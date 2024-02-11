#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Serialization/ReflectionSerializer.h>


const plPlatformProfile* plAssetCurator::GetDevelopmentAssetProfile() const
{
  return m_AssetProfiles[0];
}

const plPlatformProfile* plAssetCurator::GetActiveAssetProfile() const
{
  return m_AssetProfiles[m_uiActiveAssetProfile];
}

plUInt32 plAssetCurator::GetActiveAssetProfileIndex() const
{
  return m_uiActiveAssetProfile;
}

plUInt32 plAssetCurator::FindAssetProfileByName(const char* szPlatform)
{
  PL_LOCK(m_CuratorMutex);

  PL_ASSERT_DEV(!m_AssetProfiles.IsEmpty(), "Need to have a valid asset platform config");

  for (plUInt32 i = 0; i < m_AssetProfiles.GetCount(); ++i)
  {
    if (m_AssetProfiles[i]->GetConfigName().IsEqual_NoCase(szPlatform))
    {
      return i;
    }
  }

  return plInvalidIndex;
}

plUInt32 plAssetCurator::GetNumAssetProfiles() const
{
  return m_AssetProfiles.GetCount();
}

const plPlatformProfile* plAssetCurator::GetAssetProfile(plUInt32 uiIndex) const
{
  if (uiIndex >= m_AssetProfiles.GetCount())
    return m_AssetProfiles[0]; // fall back to default platform

  return m_AssetProfiles[uiIndex];
}

plPlatformProfile* plAssetCurator::GetAssetProfile(plUInt32 uiIndex)
{
  if (uiIndex >= m_AssetProfiles.GetCount())
    return m_AssetProfiles[0]; // fall back to default platform

  return m_AssetProfiles[uiIndex];
}

plPlatformProfile* plAssetCurator::CreateAssetProfile()
{
  plPlatformProfile* pProfile = PL_DEFAULT_NEW(plPlatformProfile);
  m_AssetProfiles.PushBack(pProfile);

  return pProfile;
}

plResult plAssetCurator::DeleteAssetProfile(plPlatformProfile* pProfile)
{
  if (m_AssetProfiles.GetCount() <= 1)
    return PL_FAILURE;

  // do not allow to delete element 0 !

  for (plUInt32 i = 1; i < m_AssetProfiles.GetCount(); ++i)
  {
    if (m_AssetProfiles[i] == pProfile)
    {
      if (m_uiActiveAssetProfile == i)
        return PL_FAILURE;

      if (i < m_uiActiveAssetProfile)
        --m_uiActiveAssetProfile;

      PL_DEFAULT_DELETE(pProfile);
      m_AssetProfiles.RemoveAtAndCopy(i);

      return PL_SUCCESS;
    }
  }

  return PL_FAILURE;
}

void plAssetCurator::SetActiveAssetProfileByIndex(plUInt32 uiIndex, bool bForceReevaluation /*= false*/)
{
  if (uiIndex >= m_AssetProfiles.GetCount())
    uiIndex = 0; // fall back to default platform

  if (!bForceReevaluation && m_uiActiveAssetProfile == uiIndex)
    return;

  PL_LOG_BLOCK("Switch Active Asset Platform", m_AssetProfiles[uiIndex]->GetConfigName());

  m_uiActiveAssetProfile = uiIndex;

  CheckFileSystem();

  {
    plAssetCuratorEvent e;
    e.m_Type = plAssetCuratorEvent::Type::ActivePlatformChanged;
    m_Events.Broadcast(e);
  }

  {
    plSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ChangeActivePlatform";
    msg.m_sPayload = GetActiveAssetProfile()->GetConfigName();
    plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
}

void plAssetCurator::SaveRuntimeProfiles()
{
  for (plUInt32 i = 0; i < GetNumAssetProfiles(); ++i)
  {
    plStringBuilder sProfileRuntimeDataFile;

    plPlatformProfile* pProfile = GetAssetProfile(i);

    sProfileRuntimeDataFile.Set(":project/RuntimeConfigs/", pProfile->GetConfigName(), ".plProfile");

    pProfile->SaveForRuntime(sProfileRuntimeDataFile).IgnoreResult();
  }
}

plResult plAssetCurator::SaveAssetProfiles()
{
  plDeferredFileWriter file;
  file.SetOutput(":project/Editor/AssetProfiles.ddl");

  plOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  ddl.BeginObject("AssetProfiles");

  for (const auto* pCfg : m_AssetProfiles)
  {
    ddl.BeginObject("Config", pCfg->GetConfigName());

    // make sure to create the same GUID every time, otherwise the serialized file changes all the time
    const plUuid guid = plUuid::MakeStableUuidFromString(pCfg->GetConfigName());

    plReflectionSerializer::WriteObjectToDDL(ddl, pCfg->GetDynamicRTTI(), pCfg, guid);

    ddl.EndObject();
  }

  ddl.EndObject();

  return file.Close();
}

plResult plAssetCurator::LoadAssetProfiles()
{
  PL_LOG_BLOCK("LoadAssetProfiles", ":project/Editor/PlatformProfiles.ddl");

  plFileReader file;
  if (file.Open(":project/Editor/AssetProfiles.ddl").Failed())
    return PL_FAILURE;

  plOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return PL_FAILURE;

  const plOpenDdlReaderElement* pRootElement = ddl.GetRootElement()->FindChildOfType("AssetProfiles");

  if (!pRootElement)
    return PL_FAILURE;

  if (pRootElement->FindChildOfType("Config") == nullptr)
    return PL_FAILURE;

  ClearAssetProfiles();

  for (auto pChild = pRootElement->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Config"))
    {
      const plRTTI* pRtti = nullptr;
      void* pConfigObj = plReflectionSerializer::ReadObjectFromDDL(pChild, pRtti);

      auto pProfile = static_cast<plPlatformProfile*>(pConfigObj);

      pProfile->AddMissingConfigs();

      m_AssetProfiles.PushBack(pProfile);
    }
  }

  if (m_AssetProfiles.IsEmpty() || m_AssetProfiles[0]->GetConfigName() != "Default")
  {
    plPlatformProfile* pCfg = PL_DEFAULT_NEW(plPlatformProfile);
    pCfg->SetConfigName("Default");

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
    pCfg->SetTargetPlatform("Windows");
#elif PL_ENABLED(PL_PLATFORM_LINUX)
    pCfg->SetTargetPlatform("Linux");
#elif PL_ENABLED(PL_PLATFORM_OSX)
    pCfg->SetTargetPlatform("OSX");
#endif

    pCfg->AddMissingConfigs();
    m_AssetProfiles.Insert(pCfg, 0);
  }

  return PL_SUCCESS;
}

void plAssetCurator::ClearAssetProfiles()
{
  for (auto pCfg : m_AssetProfiles)
  {
    pCfg->GetDynamicRTTI()->GetAllocator()->Deallocate(pCfg);
  }

  m_AssetProfiles.Clear();
}

void plAssetCurator::SetupDefaultAssetProfiles()
{
  ClearAssetProfiles();

  {
    plPlatformProfile* pCfg = PL_DEFAULT_NEW(plPlatformProfile);
    pCfg->SetConfigName("Default");

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
    pCfg->SetTargetPlatform("Windows");
#elif PL_ENABLED(PL_PLATFORM_LINUX)
    pCfg->SetTargetPlatform("Linux");
#elif PL_ENABLED(PL_PLATFORM_OSX)
    pCfg->SetTargetPlatform("OSX");
#endif

    pCfg->AddMissingConfigs();
    m_AssetProfiles.PushBack(pCfg);
  }
}

void plAssetCurator::ComputeAllDocumentManagerAssetProfileHashes()
{
  for (auto pMan : plDocumentManager::GetAllDocumentManagers())
  {
    if (auto pAssMan = plDynamicCast<plAssetDocumentManager*>(pMan))
    {
      pAssMan->ComputeAssetProfileHash(GetActiveAssetProfile());
    }
  }
}

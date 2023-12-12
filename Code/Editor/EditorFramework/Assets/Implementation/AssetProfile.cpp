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
  PLASMA_LOCK(m_CuratorMutex);

  PLASMA_ASSERT_DEV(!m_AssetProfiles.IsEmpty(), "Need to have a valid asset platform config");

  for (plUInt32 i = 0; i < m_AssetProfiles.GetCount(); ++i)
  {
    if (m_AssetProfiles[i]->m_sName.IsEqual_NoCase(szPlatform))
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

const plPlatformProfile* plAssetCurator::GetAssetProfile(plUInt32 index) const
{
  if (index >= m_AssetProfiles.GetCount())
    return m_AssetProfiles[0]; // fall back to default platform

  return m_AssetProfiles[index];
}

plPlatformProfile* plAssetCurator::GetAssetProfile(plUInt32 index)
{
  if (index >= m_AssetProfiles.GetCount())
    return m_AssetProfiles[0]; // fall back to default platform

  return m_AssetProfiles[index];
}

plPlatformProfile* plAssetCurator::CreateAssetProfile()
{
  plPlatformProfile* pProfile = PLASMA_DEFAULT_NEW(plPlatformProfile);
  m_AssetProfiles.PushBack(pProfile);

  return pProfile;
}

plResult plAssetCurator::DeleteAssetProfile(plPlatformProfile* pProfile)
{
  if (m_AssetProfiles.GetCount() <= 1)
    return PLASMA_FAILURE;

  // do not allow to delete element 0 !

  for (plUInt32 i = 1; i < m_AssetProfiles.GetCount(); ++i)
  {
    if (m_AssetProfiles[i] == pProfile)
    {
      if (m_uiActiveAssetProfile == i)
        return PLASMA_FAILURE;

      if (i < m_uiActiveAssetProfile)
        --m_uiActiveAssetProfile;

      PLASMA_DEFAULT_DELETE(pProfile);
      m_AssetProfiles.RemoveAtAndCopy(i);

      return PLASMA_SUCCESS;
    }
  }

  return PLASMA_FAILURE;
}

void plAssetCurator::SetActiveAssetProfileByIndex(plUInt32 index, bool bForceReevaluation /*= false*/)
{
  if (index >= m_AssetProfiles.GetCount())
    index = 0; // fall back to default platform

  if (!bForceReevaluation && m_uiActiveAssetProfile == index)
    return;

  PLASMA_LOG_BLOCK("Switch Active Asset Platform", m_AssetProfiles[index]->GetConfigName());

  m_uiActiveAssetProfile = index;

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
    PlasmaEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
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
    ddl.BeginObject("Config", pCfg->m_sName);

    // make sure to create the same GUID every time, otherwise the serialized file changes all the time
    const plUuid guid = plUuid::StableUuidForString(pCfg->GetConfigName());

    plReflectionSerializer::WriteObjectToDDL(ddl, pCfg->GetDynamicRTTI(), pCfg, guid);

    ddl.EndObject();
  }

  ddl.EndObject();

  return file.Close();
}

plResult plAssetCurator::LoadAssetProfiles()
{
  PLASMA_LOG_BLOCK("LoadAssetProfiles", ":project/Editor/PlatformProfiles.ddl");

  plFileReader file;
  if (file.Open(":project/Editor/AssetProfiles.ddl").Failed())
    return PLASMA_FAILURE;

  plOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return PLASMA_FAILURE;

  const plOpenDdlReaderElement* pRootElement = ddl.GetRootElement()->FindChildOfType("AssetProfiles");

  if (!pRootElement)
    return PLASMA_FAILURE;

  if (pRootElement->FindChildOfType("Config") == nullptr)
    return PLASMA_FAILURE;

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

  return PLASMA_SUCCESS;
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
    plPlatformProfile* pCfg = PLASMA_DEFAULT_NEW(plPlatformProfile);
    pCfg->m_sName = "PC";
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

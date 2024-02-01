#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Profiling/Profiling.h>

void plPluginBundle::WriteStateToDDL(plOpenDdlWriter& ref_ddl, const char* szOwnName) const
{
  ref_ddl.BeginObject("PluginState");
  plOpenDdlUtils::StoreString(ref_ddl, szOwnName, "ID");
  plOpenDdlUtils::StoreBool(ref_ddl, m_bSelected, "Selected");
  plOpenDdlUtils::StoreBool(ref_ddl, m_bLoadCopy, "LoadCopy");
  ref_ddl.EndObject();
}

void plPluginBundle::ReadStateFromDDL(plOpenDdlReader& ref_ddl, const char* szOwnName)
{
  m_bSelected = false;

  auto pState = ref_ddl.GetRootElement()->FindChildOfType("PluginState");
  while (pState)
  {
    auto pName = pState->FindChildOfType(plOpenDdlPrimitiveType::String, "ID");
    if (!pName || pName->GetPrimitivesString()[0] != szOwnName)
    {
      pState = pState->GetSibling();
      continue;
    }

    if (auto pVal = pState->FindChildOfType(plOpenDdlPrimitiveType::Bool, "Selected"))
      m_bSelected = pVal->GetPrimitivesBool()[0];
    if (auto pVal = pState->FindChildOfType(plOpenDdlPrimitiveType::Bool, "LoadCopy"))
      m_bLoadCopy = pVal->GetPrimitivesBool()[0];

    break;
  }
}

void plPluginBundleSet::WriteStateToDDL(plOpenDdlWriter& ref_ddl) const
{
  for (const auto& it : m_Plugins)
  {
    if (it.Value().m_bSelected)
    {
      it.Value().WriteStateToDDL(ref_ddl, it.Key());
    }
  }
}

void plPluginBundleSet::ReadStateFromDDL(plOpenDdlReader& ref_ddl)
{
  for (auto& it : m_Plugins)
  {
    it.Value().ReadStateFromDDL(ref_ddl, it.Key());
  }
}

bool plPluginBundleSet::IsStateEqual(const plPluginBundleSet& rhs) const
{
  if (m_Plugins.GetCount() != rhs.m_Plugins.GetCount())
    return false;

  for (auto it : m_Plugins)
  {
    auto it2 = rhs.m_Plugins.Find(it.Key());

    if (!it2.IsValid())
      return false;

    if (!it.Value().IsStateEqual(it2.Value()))
      return false;
  }

  return true;
}

plResult plPluginBundle::ReadBundleFromDDL(plOpenDdlReader& ref_ddl)
{
  PL_LOG_BLOCK("Reading plugin info file");

  auto pInfo = ref_ddl.GetRootElement()->FindChildOfType("PluginInfo");

  if (pInfo == nullptr)
  {
    plLog::Error("'PluginInfo' root object is missing");
    return PL_FAILURE;
  }

  if (auto pElement = pInfo->FindChildOfType(plOpenDdlPrimitiveType::Bool, "Mandatory"))
    m_bMandatory = pElement->GetPrimitivesBool()[0];

  if (auto pElement = pInfo->FindChildOfType(plOpenDdlPrimitiveType::String, "DisplayName"))
    m_sDisplayName = pElement->GetPrimitivesString()[0];

  if (auto pElement = pInfo->FindChildOfType(plOpenDdlPrimitiveType::String, "Description"))
    m_sDescription = pElement->GetPrimitivesString()[0];

  if (auto pElement = pInfo->FindChildOfType(plOpenDdlPrimitiveType::String, "EditorPlugins"))
  {
    m_EditorPlugins.SetCount(pElement->GetNumPrimitives());
    for (plUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_EditorPlugins[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(plOpenDdlPrimitiveType::String, "EditorEnginePlugins"))
  {
    m_EditorEnginePlugins.SetCount(pElement->GetNumPrimitives());
    for (plUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_EditorEnginePlugins[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(plOpenDdlPrimitiveType::String, "RuntimePlugins"))
  {
    m_RuntimePlugins.SetCount(pElement->GetNumPrimitives());
    for (plUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_RuntimePlugins[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(plOpenDdlPrimitiveType::String, "PackageDependencies"))
  {
    m_PackageDependencies.SetCount(pElement->GetNumPrimitives());
    for (plUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_PackageDependencies[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(plOpenDdlPrimitiveType::String, "RequiredPlugins"))
  {
    m_RequiredBundles.SetCount(pElement->GetNumPrimitives());
    for (plUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_RequiredBundles[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(plOpenDdlPrimitiveType::String, "ExclusiveFeatures"))
  {
    m_ExclusiveFeatures.SetCount(pElement->GetNumPrimitives());
    for (plUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_ExclusiveFeatures[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(plOpenDdlPrimitiveType::String, "EnabledInTemplates"))
  {
    m_EnabledInTemplates.SetCount(pElement->GetNumPrimitives());
    for (plUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_EnabledInTemplates[i] = pElement->GetPrimitivesString()[i];
  }

  m_bMissing = false;

  return PL_SUCCESS;
}

void plQtEditorApp::DetectAvailablePluginBundles(plStringView sSearchDirectory)
{
#if PL_ENABLED(PL_SUPPORTS_FILE_ITERATORS)
  // find all plPluginBundle files
  {
    plStringBuilder sSearch = sSearchDirectory;

    sSearch.AppendPath("*.plPluginBundle");

    plStringBuilder sPath, sPlugin;

    plFileSystemIterator fsit;
    for (fsit.StartSearch(sSearch.GetData(), plFileSystemIteratorFlags::ReportFiles); fsit.IsValid(); fsit.Next())
    {
      sPlugin = fsit.GetStats().m_sName;
      sPlugin.RemoveFileExtension();

      fsit.GetStats().GetFullPath(sPath);

      plFileReader file;
      if (file.Open(sPath).Succeeded())
      {
        plOpenDdlReader ddl;
        if (ddl.ParseDocument(file).Failed())
        {
          plLog::Error("Failed to parse plugin bundle file: '{}'", sPath);
        }
        else
        {
          m_PluginBundles.m_Plugins[sPlugin].ReadBundleFromDDL(ddl).IgnoreResult();
        }
      }
    }
  }

  // additionally, find all *Plugin.dll files that are not mentioned in any plPluginBundle and treat them as fake plugin bundles
  if (false) // sometimes useful, but not how it's supposed to be
  {
    plStringBuilder sSearch = plOSFile::GetApplicationDirectory();

    sSearch.AppendPath("*Plugin.dll");

    plStringBuilder sPlugin;

    auto isUsedInBundle = [this](const plStringBuilder& sPlugin) -> bool {
      for (auto pit : m_PluginBundles.m_Plugins)
      {
        if (pit.Key().IsEqual_NoCase(sPlugin))
          return true;

        const plPluginBundle& val = pit.Value();

        for (const auto& rt : val.m_RuntimePlugins)
        {
          if (rt.IsEqual_NoCase(sPlugin))
            return true;
        }
      }

      return false;
    };

    plFileSystemIterator fsit;
    for (fsit.StartSearch(sSearch.GetData(), plFileSystemIteratorFlags::ReportFiles); fsit.IsValid(); fsit.Next())
    {
      sPlugin = fsit.GetStats().m_sName;
      sPlugin.RemoveFileExtension();

      if (isUsedInBundle(sPlugin))
        continue;

      auto& newp = m_PluginBundles.m_Plugins[sPlugin];
      newp.m_RuntimePlugins.PushBack(sPlugin);
      newp.m_sDescription = "No plPluginBundle file is present for this plugin.";

      sPlugin.Shrink(0, 6);
      newp.m_sDisplayName = sPlugin;
    }
  }
#else
  PL_ASSERT_NOT_IMPLEMENTED;
#endif
}

void plQtEditorApp::LoadEditorPlugins()
{
  PL_PROFILE_SCOPE("LoadEditorPlugins");
  DetectAvailablePluginBundles(plOSFile::GetApplicationDirectory());

  plPlugin::InitializeStaticallyLinkedPlugins();
}

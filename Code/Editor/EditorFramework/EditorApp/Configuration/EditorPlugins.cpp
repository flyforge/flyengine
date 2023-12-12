#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Profiling/Profiling.h>

void plPluginBundle::WriteStateToDDL(plOpenDdlWriter& ddl, const char* szOwnName) const
{
  ddl.BeginObject("PluginState");
  plOpenDdlUtils::StoreString(ddl, szOwnName, "ID");
  plOpenDdlUtils::StoreBool(ddl, m_bSelected, "Selected");
  plOpenDdlUtils::StoreBool(ddl, m_bLoadCopy, "LoadCopy");
  ddl.EndObject();
}

void plPluginBundle::ReadStateFromDDL(plOpenDdlReader& ddl, const char* szOwnName)
{
  m_bSelected = false;

  auto pState = ddl.GetRootElement()->FindChildOfType("PluginState");
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

void plPluginBundleSet::WriteStateToDDL(plOpenDdlWriter& ddl) const
{
  for (const auto& it : m_Plugins)
  {
    if (it.Value().m_bSelected)
    {
      it.Value().WriteStateToDDL(ddl, it.Key());
    }
  }
}

void plPluginBundleSet::ReadStateFromDDL(plOpenDdlReader& ddl)
{
  for (auto& it : m_Plugins)
  {
    it.Value().ReadStateFromDDL(ddl, it.Key());
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

plResult plPluginBundle::ReadBundleFromDDL(plOpenDdlReader& ddl)
{
  PLASMA_LOG_BLOCK("Reading plugin info file");

  auto pInfo = ddl.GetRootElement()->FindChildOfType("PluginInfo");

  if (pInfo == nullptr)
  {
    plLog::Error("'PluginInfo' root object is missing");
    return PLASMA_FAILURE;
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

  return PLASMA_SUCCESS;
}

void plQtEditorApp::DetectAvailablePluginBundles(plStringView sSearchDirectory)
{
#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS)
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
  PLASMA_ASSERT_NOT_IMPLEMENTED;
#endif
}

void plQtEditorApp::LoadEditorPlugins()
{
  PLASMA_PROFILE_SCOPE("LoadEditorPlugins");
  DetectAvailablePluginBundles(plOSFile::GetApplicationDirectory());

  plPlugin::InitializeStaticallyLinkedPlugins();
}

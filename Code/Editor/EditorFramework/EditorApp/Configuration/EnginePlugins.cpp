#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>

void plQtEditorApp::StoreEnginePluginModificationTimes()
{
  for (auto it : m_PluginBundles.m_Plugins)
  {
    plPluginBundle& plugin = it.Value();

    for (const plString& rt : plugin.m_RuntimePlugins)
    {
      plStringBuilder sPath, sCopy;
      plPlugin::GetPluginPaths(rt, sPath, sCopy, 0);

      plFileStats stats;
      if (plOSFile::GetFileStats(sPath, stats).Succeeded())
      {
        if (!plugin.m_LastModificationTime.IsValid() || stats.m_LastModificationTime.Compare(plugin.m_LastModificationTime, plTimestamp::CompareMode::Newer))
        {
          // store the maximum (latest) modification timestamp
          plugin.m_LastModificationTime = stats.m_LastModificationTime;
        }
      }
    }
  }
}

bool plQtEditorApp::CheckForEnginePluginModifications()
{
  for (auto it : m_PluginBundles.m_Plugins)
  {
    plPluginBundle& plugin = it.Value();

    if (plugin.m_bMissing)
    {
      DetectAvailablePluginBundles(plOSFile::GetApplicationDirectory());

      plCppSettings cppSettings;
      if (cppSettings.Load().Succeeded())
      {
        plQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(plCppProject::GetPluginSourceDir(cppSettings));
      }

      break;
    }
  }

  for (auto it : m_PluginBundles.m_Plugins)
  {
    plPluginBundle& plugin = it.Value();

    if (!plugin.m_bSelected || !plugin.m_bLoadCopy)
      continue;

    for (const plString& rt : plugin.m_RuntimePlugins)
    {
      plStringBuilder sPath, sCopy;
      plPlugin::GetPluginPaths(rt, sPath, sCopy, 0);

      plFileStats stats;
      if (plOSFile::GetFileStats(sPath, stats).Succeeded())
      {
        if (!plugin.m_LastModificationTime.IsValid() || stats.m_LastModificationTime.Compare(plugin.m_LastModificationTime, plTimestamp::CompareMode::Newer))
        {
          return true;
        }
      }
    }
  }

  return false;
}

void plQtEditorApp::RestartEngineProcessIfPluginsChanged(bool bForce)
{
  if (!bForce)
  {
    if (m_LastPluginModificationCheck + plTime::Seconds(2) > plTime::Now())
      return;
  }

  m_LastPluginModificationCheck = plTime::Now();

  for (auto pMan : plDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : pMan->plDocumentManager::GetAllOpenDocuments())
    {
      if (!pDoc->CanEngineProcessBeRestarted())
      {
        // not allowed to restart at the moment
        return;
      }
    }
  }

  if (!CheckForEnginePluginModifications())
    return;

  plLog::Info("Engine plugins have changed, restarting engine process.");

  StoreEnginePluginModificationTimes();
  PlasmaEditorEngineProcessConnection::GetSingleton()->SetPluginConfig(GetRuntimePluginConfig(true));
  PlasmaEditorEngineProcessConnection::GetSingleton()->RestartProcess().IgnoreResult();
}

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

void plQtEditorApp::SaveRecentFiles()
{
  m_RecentProjects.Save(":appdata/Settings/RecentProjects.txt");
  m_RecentDocuments.Save(":appdata/Settings/RecentDocuments.txt");
}

void plQtEditorApp::LoadRecentFiles()
{
  m_RecentProjects.Load(":appdata/Settings/RecentProjects.txt");
  m_RecentDocuments.Load(":appdata/Settings/RecentDocuments.txt");
}

void plQtEditorApp::SaveOpenDocumentsList()
{
  plQtContainerWindow::GetContainerWindow()->SaveWindowLayout();
  const plDynamicArray<plQtDocumentWindow*>& windows = plQtDocumentWindow::GetAllDocumentWindows();

  if (windows.IsEmpty())
    return;

  plRecentFilesList allDocs(windows.GetCount());

  plDynamicArray<plQtDocumentWindow*> allWindows;
  allWindows.Reserve(windows.GetCount());
  {
    auto* container = plQtContainerWindow::GetContainerWindow();
    plHybridArray<plQtDocumentWindow*, 16> docWindows;
    container->GetDocumentWindows(docWindows);
    for (auto* pWindow : docWindows)
    {
      allWindows.PushBack(pWindow);
    }
  }
  for (plInt32 w = (plInt32)allWindows.GetCount() - 1; w >= 0; --w)
  {
    if (allWindows[w]->GetDocument())
    {
      allDocs.Insert(allWindows[w]->GetDocument()->GetDocumentPath(), 0);
    }
  }

  plStringBuilder sFile = plApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  sFile.AppendPath("LastDocuments.txt");

  allDocs.Save(sFile);
}

plRecentFilesList plQtEditorApp::LoadOpenDocumentsList()
{
  plRecentFilesList allDocs(15);

  plStringBuilder sFile = plApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  sFile.AppendPath("LastDocuments.txt");

  allDocs.Load(sFile);

  return allDocs;
}

void plQtEditorApp::SaveSettings()
{
  // headless mode should never store any settings on disk
  if (m_StartupFlags.IsAnySet(StartupFlags::Headless | StartupFlags::UnitTest))
    return;

  SaveRecentFiles();

  plPreferences::SaveApplicationPreferences();

  // this setting is needed before we have loaded the preferences, so we duplicate it in the QSettings (registry)
  {
    PlasmaEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<PlasmaEditorPreferencesUser>();

    QSettings s;
    s.beginGroup("EditorPreferences");
    s.setValue("ShowSplashscreen", pPreferences->m_bShowSplashscreen);
    s.endGroup();
  }

  if (plToolsProject::IsProjectOpen())
  {
    plPreferences::SaveProjectPreferences();
    SaveOpenDocumentsList();

    m_FileSystemConfig.Save().IgnoreResult();
    GetRuntimePluginConfig(false).Save().IgnoreResult();
  }
}

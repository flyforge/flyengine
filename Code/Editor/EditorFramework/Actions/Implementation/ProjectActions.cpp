#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/Dialogs/AssetProfilesDlg.moc.h>
#include <EditorFramework/Dialogs/CppProjectDlg.moc.h>
#include <EditorFramework/Dialogs/DataDirsDlg.moc.h>
#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/Dialogs/InputConfigDlg.moc.h>
#include <EditorFramework/Dialogs/LaunchFileserveDlg.moc.h>
#include <EditorFramework/Dialogs/PluginSelectionDlg.moc.h>
#include <EditorFramework/Dialogs/PreferencesDlg.moc.h>
#include <EditorFramework/Dialogs/TagsDlg.moc.h>
#include <EditorFramework/Dialogs/WindowCfgDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <GuiFoundation/Dialogs/ShortcutEditorDlg.moc.h>

plActionDescriptorHandle plProjectActions::s_hCatProjectGeneral;
plActionDescriptorHandle plProjectActions::s_hCatProjectAssets;
plActionDescriptorHandle plProjectActions::s_hCatProjectConfig;
plActionDescriptorHandle plProjectActions::s_hCatProjectExternal;

plActionDescriptorHandle plProjectActions::s_hCatFilesGeneral;
plActionDescriptorHandle plProjectActions::s_hCatFileCommon;
plActionDescriptorHandle plProjectActions::s_hCatFileSpecial;
plActionDescriptorHandle plProjectActions::s_hCatAssetDoc;

plActionDescriptorHandle plProjectActions::s_hCreateDocument;
plActionDescriptorHandle plProjectActions::s_hOpenDocument;
plActionDescriptorHandle plProjectActions::s_hRecentDocuments;

plActionDescriptorHandle plProjectActions::s_hOpenDashboard;
plActionDescriptorHandle plProjectActions::s_hCreateProject;
plActionDescriptorHandle plProjectActions::s_hOpenProject;
plActionDescriptorHandle plProjectActions::s_hRecentProjects;
plActionDescriptorHandle plProjectActions::s_hCloseProject;
plActionDescriptorHandle plProjectActions::s_hDocsAndCommunity;

plActionDescriptorHandle plProjectActions::s_hCatProjectSettings;
plActionDescriptorHandle plProjectActions::s_hCatPluginSettings;
plActionDescriptorHandle plProjectActions::s_hShortcutEditor;
plActionDescriptorHandle plProjectActions::s_hDataDirectories;
plActionDescriptorHandle plProjectActions::s_hWindowConfig;
plActionDescriptorHandle plProjectActions::s_hInputConfig;
plActionDescriptorHandle plProjectActions::s_hPreferencesDlg;
plActionDescriptorHandle plProjectActions::s_hTagsConfig;
plActionDescriptorHandle plProjectActions::s_hImportAsset;
plActionDescriptorHandle plProjectActions::s_hAssetProfiles;
plActionDescriptorHandle plProjectActions::s_hExportProject;
plActionDescriptorHandle plProjectActions::s_hPluginSelection;
plActionDescriptorHandle plProjectActions::s_hClearAssetCaches;

plActionDescriptorHandle plProjectActions::s_hCatToolsExternal;
plActionDescriptorHandle plProjectActions::s_hCatToolsEditor;
plActionDescriptorHandle plProjectActions::s_hCatToolsDocument;
plActionDescriptorHandle plProjectActions::s_hCatEditorSettings;
plActionDescriptorHandle plProjectActions::s_hReloadResources;
plActionDescriptorHandle plProjectActions::s_hReloadEngine;
plActionDescriptorHandle plProjectActions::s_hLaunchFileserve;
plActionDescriptorHandle plProjectActions::s_hLaunchInspector;
plActionDescriptorHandle plProjectActions::s_hSaveProfiling;
plActionDescriptorHandle plProjectActions::s_hOpenVsCode;

plActionDescriptorHandle plProjectActions::s_hCppProjectMenu;
plActionDescriptorHandle plProjectActions::s_hSetupCppProject;
plActionDescriptorHandle plProjectActions::s_hOpenCppProject;
plActionDescriptorHandle plProjectActions::s_hCompileCppProject;
plActionDescriptorHandle plProjectActions::s_hRegenerateCppSolution;

void plProjectActions::RegisterActions()
{
  s_hCatProjectGeneral = PL_REGISTER_CATEGORY("G.Project.General");
  s_hCatProjectAssets = PL_REGISTER_CATEGORY("G.Project.Assets");
  s_hCatProjectExternal = PL_REGISTER_CATEGORY("G.Project.External");
  s_hCatProjectConfig = PL_REGISTER_CATEGORY("G.Project.Config");
  s_hCatEditorSettings = PL_REGISTER_CATEGORY("G.Editor.Settings");

  s_hCatFilesGeneral = PL_REGISTER_CATEGORY("G.Files.General");
  s_hCatFileCommon = PL_REGISTER_CATEGORY("G.File.Common");
  s_hCatFileSpecial = PL_REGISTER_CATEGORY("G.File.Special");
  s_hCatAssetDoc = PL_REGISTER_CATEGORY("G.AssetDoc");


  s_hOpenDashboard = PL_REGISTER_ACTION_1("Editor.OpenDashboard", plActionScope::Global, "Editor", "Ctrl+Shift+D", plProjectAction, plProjectAction::ButtonType::OpenDashboard);

  s_hCreateProject = PL_REGISTER_ACTION_1("Project.Create", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::CreateProject);

  s_hOpenProject = PL_REGISTER_ACTION_1("Project.Open", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::OpenProject);

  s_hRecentProjects = PL_REGISTER_DYNAMIC_MENU("Project.RecentProjects.Menu", plRecentProjectsMenuAction, "");
  s_hCloseProject = PL_REGISTER_ACTION_1("Project.Close", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::CloseProject);

  s_hImportAsset = PL_REGISTER_ACTION_1("Project.ImportAsset", plActionScope::Global, "Project", "Ctrl+I", plProjectAction, plProjectAction::ButtonType::ImportAsset);
  s_hClearAssetCaches = PL_REGISTER_ACTION_1("Project.ClearAssetCaches", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::ClearAssetCaches);

  s_hExportProject = PL_REGISTER_ACTION_1("Project.ExportProject", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::ExportProject);

  s_hCppProjectMenu = PL_REGISTER_MENU("G.Project.Cpp");
  {
    s_hSetupCppProject = PL_REGISTER_ACTION_1("Project.SetupCppProject", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::SetupCppProject);
    s_hOpenCppProject = PL_REGISTER_ACTION_1("Project.OpenCppProject", plActionScope::Global, "Project", "Ctrl+Shift+O", plProjectAction, plProjectAction::ButtonType::OpenCppProject);
    s_hCompileCppProject = PL_REGISTER_ACTION_1("Project.CompileCppProject", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::CompileCppProject);
    s_hRegenerateCppSolution = PL_REGISTER_ACTION_1("Project.RegenerateCppSolution", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::RegenerateCppSolution);
  }

  s_hCatProjectSettings = PL_REGISTER_MENU("G.Project.Settings");

  s_hPluginSelection = PL_REGISTER_ACTION_1("Project.PluginSelection", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::PluginSelection);
  s_hDataDirectories = PL_REGISTER_ACTION_1("Project.DataDirectories", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::DataDirectories);
  s_hTagsConfig = PL_REGISTER_ACTION_1("Engine.Tags", plActionScope::Global, "Editor", "", plProjectAction, plProjectAction::ButtonType::TagsDialog);
  s_hInputConfig = PL_REGISTER_ACTION_1("Project.InputConfig", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::InputConfig);
  s_hWindowConfig = PL_REGISTER_ACTION_1("Project.WindowConfig", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::WindowConfig);
  s_hAssetProfiles = PL_REGISTER_ACTION_1("Project.AssetProfiles", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::AssetProfiles);

  s_hCatPluginSettings = PL_REGISTER_MENU("G.Plugins.Settings");

  //////////////////////////////////////////////////////////////////////////

  s_hCreateDocument = PL_REGISTER_ACTION_1("Document.Create", plActionScope::Global, "Project", "Ctrl+N", plProjectAction, plProjectAction::ButtonType::CreateDocument);
  s_hOpenDocument = PL_REGISTER_ACTION_1("Document.Open", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::OpenDocument);
  s_hRecentDocuments = PL_REGISTER_DYNAMIC_MENU("Project.RecentDocuments.Menu", plRecentDocumentsMenuAction, "");

  s_hShortcutEditor = PL_REGISTER_ACTION_1("Editor.Shortcuts", plActionScope::Global, "Editor", "", plProjectAction, plProjectAction::ButtonType::Shortcuts);
  s_hPreferencesDlg = PL_REGISTER_ACTION_1("Editor.Preferences", plActionScope::Global, "Editor", "", plProjectAction, plProjectAction::ButtonType::PreferencesDialog);

  s_hCatToolsExternal = PL_REGISTER_CATEGORY("G.Tools.External");
  s_hCatToolsEditor = PL_REGISTER_CATEGORY("G.Tools.Editor");
  s_hCatToolsDocument = PL_REGISTER_CATEGORY("G.Tools.Document");

  s_hReloadResources = PL_REGISTER_ACTION_1("Engine.ReloadResources", plActionScope::Global, "Engine", "F4", plProjectAction, plProjectAction::ButtonType::ReloadResources);
  s_hReloadEngine = PL_REGISTER_ACTION_1("Engine.ReloadEngine", plActionScope::Global, "Engine", "Ctrl+Shift+F4", plProjectAction, plProjectAction::ButtonType::ReloadEngine);
  s_hLaunchFileserve = PL_REGISTER_ACTION_1("Editor.LaunchFileserve", plActionScope::Global, "Engine", "", plProjectAction, plProjectAction::ButtonType::LaunchFileserve);
  s_hLaunchInspector = PL_REGISTER_ACTION_1("Editor.LaunchInspector", plActionScope::Global, "Engine", "", plProjectAction, plProjectAction::ButtonType::LaunchInspector);
  s_hSaveProfiling = PL_REGISTER_ACTION_1("Editor.SaveProfiling", plActionScope::Global, "Engine", "Ctrl+Alt+P", plProjectAction, plProjectAction::ButtonType::SaveProfiling);
  s_hOpenVsCode = PL_REGISTER_ACTION_1("Editor.OpenVsCode", plActionScope::Global, "Project", "Ctrl+Alt+O", plProjectAction, plProjectAction::ButtonType::OpenVsCode);



  s_hDocsAndCommunity = PL_REGISTER_ACTION_1("Editor.DocsAndCommunity", plActionScope::Global, "Editor", "", plProjectAction, plProjectAction::ButtonType::ShowDocsAndCommunity);
}

void plProjectActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCatProjectGeneral);
  plActionManager::UnregisterAction(s_hCatProjectAssets);
  plActionManager::UnregisterAction(s_hCatProjectConfig);
  plActionManager::UnregisterAction(s_hCatProjectExternal);

  plActionManager::UnregisterAction(s_hCatFilesGeneral);
  plActionManager::UnregisterAction(s_hCatFileCommon);
  plActionManager::UnregisterAction(s_hCatFileSpecial);
  plActionManager::UnregisterAction(s_hCatAssetDoc);

  plActionManager::UnregisterAction(s_hCreateDocument);
  plActionManager::UnregisterAction(s_hOpenDocument);
  plActionManager::UnregisterAction(s_hRecentDocuments);
  plActionManager::UnregisterAction(s_hOpenDashboard);
  plActionManager::UnregisterAction(s_hDocsAndCommunity);
  plActionManager::UnregisterAction(s_hCreateProject);
  plActionManager::UnregisterAction(s_hOpenProject);
  plActionManager::UnregisterAction(s_hRecentProjects);
  plActionManager::UnregisterAction(s_hCloseProject);
  plActionManager::UnregisterAction(s_hCatProjectSettings);
  plActionManager::UnregisterAction(s_hCatPluginSettings);
  plActionManager::UnregisterAction(s_hCatToolsExternal);
  plActionManager::UnregisterAction(s_hCatToolsEditor);
  plActionManager::UnregisterAction(s_hCatToolsDocument);
  plActionManager::UnregisterAction(s_hCatEditorSettings);
  plActionManager::UnregisterAction(s_hReloadResources);
  plActionManager::UnregisterAction(s_hReloadEngine);
  plActionManager::UnregisterAction(s_hLaunchFileserve);
  plActionManager::UnregisterAction(s_hLaunchInspector);
  plActionManager::UnregisterAction(s_hSaveProfiling);
  plActionManager::UnregisterAction(s_hOpenVsCode);
  plActionManager::UnregisterAction(s_hShortcutEditor);
  plActionManager::UnregisterAction(s_hPreferencesDlg);
  plActionManager::UnregisterAction(s_hTagsConfig);
  plActionManager::UnregisterAction(s_hDataDirectories);
  plActionManager::UnregisterAction(s_hWindowConfig);
  plActionManager::UnregisterAction(s_hImportAsset);
  plActionManager::UnregisterAction(s_hClearAssetCaches);
  plActionManager::UnregisterAction(s_hInputConfig);
  plActionManager::UnregisterAction(s_hAssetProfiles);
  plActionManager::UnregisterAction(s_hCppProjectMenu);
  plActionManager::UnregisterAction(s_hSetupCppProject);
  plActionManager::UnregisterAction(s_hOpenCppProject);
  plActionManager::UnregisterAction(s_hCompileCppProject);
  plActionManager::UnregisterAction(s_hRegenerateCppSolution);
  plActionManager::UnregisterAction(s_hExportProject);
  plActionManager::UnregisterAction(s_hPluginSelection);
}

void plProjectActions::MapActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  plStringBuilder sPath;

  // Add categories
  pMap->MapAction(s_hCatProjectGeneral, "G.Project", 1.0f);
  pMap->MapAction(s_hCatProjectConfig, "G.Project", 2.0f);
  pMap->MapAction(s_hCatProjectAssets, "G.Project", 4.0f);
  pMap->MapAction(s_hCatProjectExternal, "G.Project", 5.0f);

  pMap->MapAction(s_hCatToolsExternal, "G.Tools", 1.0f);
  pMap->MapAction(s_hCatToolsEditor, "G.Tools", 2.0f);
  pMap->MapAction(s_hCatToolsDocument, "G.Tools", 3.0f);
  pMap->MapAction(s_hCatEditorSettings, "G.Tools", 1000.0f);

  pMap->MapAction(s_hCatProjectSettings, "G.Project.Config", 1.0f);
  pMap->MapAction(s_hCatPluginSettings, "G.Project.Config", 1.0f);

  if (pMap->SearchPathForAction("G.File", sPath).Succeeded())
  {
    pMap->MapAction(s_hCatFilesGeneral, sPath, 1.0f);
    pMap->MapAction(s_hCatFileCommon, sPath, 2.0f);
    pMap->MapAction(s_hCatAssetDoc, sPath, 3.0f);
    pMap->MapAction(s_hCatFileSpecial, sPath, 4.0f);
  }

  // Add actions
  pMap->MapAction(s_hOpenDashboard, "G.Project.General", 1.0f);
  // pMap->MapAction(s_hCreateProject, "G.Project.General", 2.0f); // use dashboard
  // pMap->MapAction(s_hOpenProject, "G.Project.General", 3.0f);   // use dashboard
  // pMap->MapAction(s_hRecentProjects, "G.Project.General", 4.0f);// use dashboard
  pMap->MapAction(s_hCloseProject, "G.Project.General", 5.0f);

  pMap->MapAction(s_hImportAsset, "G.Project.Assets", 1.0f);
  pMap->MapAction(s_hClearAssetCaches, "G.Project.Assets", 5.0f);

  pMap->MapAction(s_hDataDirectories, "G.Project.Settings", 1.0f);
  pMap->MapAction(s_hInputConfig, "G.Project.Settings", 2.0f);
  pMap->MapAction(s_hWindowConfig, "G.Project.Settings", 3.0f);
  pMap->MapAction(s_hTagsConfig, "G.Project.Settings", 4.0f);
  pMap->MapAction(s_hAssetProfiles, "G.Project.Settings", 5.0f);

  pMap->MapAction(s_hPluginSelection, "G.Plugins.Settings", -1000.0f);

  pMap->MapAction(s_hCppProjectMenu, "G.Project.External", 1.0f);
  pMap->MapAction(s_hSetupCppProject, "G.Project.Cpp", 1.0f);
  pMap->MapAction(s_hOpenCppProject, "G.Project.Cpp", 2.0f);
  pMap->MapAction(s_hCompileCppProject, "G.Project.Cpp", 3.0f);
  pMap->MapAction(s_hRegenerateCppSolution, "G.Project.Cpp", 4.0f);
  pMap->MapAction(s_hExportProject, "G.Project.External", 10.0f);

  pMap->MapAction(s_hOpenVsCode, "G.Tools.External", 1.0f);
  pMap->MapAction(s_hLaunchInspector, "G.Tools.External", 2.0f);
  pMap->MapAction(s_hLaunchFileserve, "G.Tools.External", 3.0f);

  pMap->MapAction(s_hReloadResources, "G.Tools.Editor", 1.0f);
  pMap->MapAction(s_hReloadEngine, "G.Tools.Editor", 2.0f);
  pMap->MapAction(s_hSaveProfiling, "G.Tools.Editor", 3.0f);

  pMap->MapAction(s_hShortcutEditor, "G.Editor.Settings", 1.0f);
  pMap->MapAction(s_hPreferencesDlg, "G.Editor.Settings", 2.0f);

  if (pMap->SearchPathForAction("G.Help", sPath).Succeeded())
  {
    pMap->MapAction(s_hDocsAndCommunity, sPath, 0.0f);
  }

  if (pMap->SearchPathForAction("G.File.Common", sPath).Succeeded())
  {
    pMap->MapAction(s_hCreateDocument, sPath, 1.0f);
    pMap->MapAction(s_hOpenDocument, sPath, 2.0f);
    pMap->MapAction(s_hRecentDocuments, sPath, 3.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// plRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRecentDocumentsMenuAction, 0, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

void plRecentDocumentsMenuAction::GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_entries)
{
  out_entries.Clear();

  if (plQtEditorApp::GetSingleton()->GetRecentDocumentsList().GetFileList().IsEmpty())
    return;

  plInt32 iMaxDocumentsToAdd = 10;
  for (auto file : plQtEditorApp::GetSingleton()->GetRecentDocumentsList().GetFileList())
  {
    QAction* pAction = nullptr;

    if (!plOSFile::ExistsFile(file.m_File))
      continue;

    plDynamicMenuAction::Item item;

    const plDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (plDocumentManager::FindDocumentTypeFromPath(file.m_File, false, pTypeDesc).Failed())
      continue;

    item.m_UserValue = file.m_File;
    item.m_Icon = plQtUiServices::GetCachedIconResource(pTypeDesc->m_sIcon, plColorScheme::GetCategoryColor(pTypeDesc->m_sAssetCategory, plColorScheme::CategoryColorUsage::MenuEntryIcon));

    if (plToolsProject::IsProjectOpen())
    {
      plString sRelativePath;
      if (!plToolsProject::GetSingleton()->IsDocumentInAllowedRoot(file.m_File, &sRelativePath))
        continue;

      item.m_sDisplay = sRelativePath;

      out_entries.PushBack(item);
    }
    else
    {
      item.m_sDisplay = file.m_File;

      out_entries.PushBack(item);
    }

    --iMaxDocumentsToAdd;

    if (iMaxDocumentsToAdd <= 0)
      break;
  }
}

void plRecentDocumentsMenuAction::Execute(const plVariant& value)
{
  plQtEditorApp::GetSingleton()->OpenDocumentQueued(value.ConvertTo<plString>());
}


////////////////////////////////////////////////////////////////////////
// plRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRecentProjectsMenuAction, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

void plRecentProjectsMenuAction::GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_entries)
{
  out_entries.Clear();

  plStringBuilder sTemp;

  for (auto file : plQtEditorApp::GetSingleton()->GetRecentProjectsList().GetFileList())
  {
    if (!plOSFile::ExistsFile(file.m_File))
      continue;

    sTemp = file.m_File;
    sTemp.PathParentDirectory();
    sTemp.Trim("/");

    plDynamicMenuAction::Item item;
    item.m_sDisplay = sTemp;
    item.m_UserValue = file.m_File;

    out_entries.PushBack(item);
  }
}

void plRecentProjectsMenuAction::Execute(const plVariant& value)
{
  plQtEditorApp::GetSingleton()->OpenProject(value.ConvertTo<plString>()).IgnoreResult();
}

////////////////////////////////////////////////////////////////////////
// plProjectAction
////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProjectAction, 1, plRTTINoAllocator)
  ;
PL_END_DYNAMIC_REFLECTED_TYPE;

plProjectAction::plProjectAction(const plActionContext& context, const char* szName, ButtonType button)
  : plButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case plProjectAction::ButtonType::CreateDocument:
      SetIconPath(":/GuiFoundation/Icons/DocumentAdd.svg");
      break;
    case plProjectAction::ButtonType::OpenDocument:
      SetIconPath(":/GuiFoundation/Icons/Document.svg");
      break;
    case plProjectAction::ButtonType::OpenDashboard:
      SetIconPath(":/GuiFoundation/Icons/Project.svg");
      break;
    case plProjectAction::ButtonType::CreateProject:
      SetIconPath(":/GuiFoundation/Icons/ProjectAdd.svg");
      break;
    case plProjectAction::ButtonType::OpenProject:
      SetIconPath(":/GuiFoundation/Icons/Project.svg");
      break;
    case plProjectAction::ButtonType::CloseProject:
      SetIconPath(":/GuiFoundation/Icons/ProjectClose.svg");
      break;
    case plProjectAction::ButtonType::ReloadResources:
      SetIconPath(":/GuiFoundation/Icons/ReloadResources.svg");
      break;
    case plProjectAction::ButtonType::LaunchFileserve:
      SetIconPath(":/EditorFramework/Icons/Fileserve.svg");
      break;
    case plProjectAction::ButtonType::LaunchInspector:
      SetIconPath(":/EditorFramework/Icons/Inspector.svg");
      break;
    case plProjectAction::ButtonType::ReloadEngine:
      SetIconPath(":/GuiFoundation/Icons/ReloadEngine.svg");
      break;
    case plProjectAction::ButtonType::DataDirectories:
      SetIconPath(":/EditorFramework/Icons/DataDirectory.svg");
      break;
    case plProjectAction::ButtonType::WindowConfig:
      SetIconPath(":/EditorFramework/Icons/WindowConfig.svg");
      break;
    case plProjectAction::ButtonType::ImportAsset:
      SetIconPath(":/GuiFoundation/Icons/Import.svg");
      break;
    case plProjectAction::ButtonType::InputConfig:
      SetIconPath(":/EditorFramework/Icons/Input.svg");
      break;
    case plProjectAction::ButtonType::PluginSelection:
      SetIconPath(":/EditorFramework/Icons/Plugins.svg");
      break;
    case plProjectAction::ButtonType::PreferencesDialog:
      SetIconPath(":/EditorFramework/Icons/StoredSettings.svg");
      break;
    case plProjectAction::ButtonType::TagsDialog:
      SetIconPath(":/EditorFramework/Icons/Tag.svg");
      break;
    case plProjectAction::ButtonType::ExportProject:
      // TODO: SetIconPath(":/EditorFramework/Icons/Tag.svg");
      break;
    case plProjectAction::ButtonType::Shortcuts:
      SetIconPath(":/GuiFoundation/Icons/Shortcuts.svg");
      break;
    case plProjectAction::ButtonType::AssetProfiles:
      SetIconPath(":/EditorFramework/Icons/AssetProfile.svg");
      break;
    case plProjectAction::ButtonType::OpenVsCode:
      SetIconPath(":/GuiFoundation/Icons/vscode.svg");
      break;
    case plProjectAction::ButtonType::SaveProfiling:
      // no icon
      break;
    case plProjectAction::ButtonType::SetupCppProject:
      SetIconPath(":/EditorFramework/Icons/VisualStudio.svg");
      break;
    case plProjectAction::ButtonType::OpenCppProject:
      // SetIconPath(":/EditorFramework/Icons/VisualStudio.svg"); // TODO
      break;
    case plProjectAction::ButtonType::CompileCppProject:
      // SetIconPath(":/EditorFramework/Icons/VisualStudio.svg"); // TODO
      break;
    case plProjectAction::ButtonType::RegenerateCppSolution:
      // SetIconPath(":/EditorFramework/Icons/VisualStudio.svg"); // TODO
      break;
    case plProjectAction::ButtonType::ShowDocsAndCommunity:
      // SetIconPath(":/GuiFoundation/Icons/Project.svg"); // TODO
      break;
    case plProjectAction::ButtonType::ClearAssetCaches:
      // SetIconPath(":/GuiFoundation/Icons/Project.svg"); // TODO
      break;
  }

  if (m_ButtonType == ButtonType::CloseProject ||
      m_ButtonType == ButtonType::DataDirectories ||
      m_ButtonType == ButtonType::WindowConfig ||
      m_ButtonType == ButtonType::ImportAsset ||
      m_ButtonType == ButtonType::TagsDialog ||
      m_ButtonType == ButtonType::ReloadEngine ||
      m_ButtonType == ButtonType::ReloadResources ||
      m_ButtonType == ButtonType::LaunchFileserve ||
      m_ButtonType == ButtonType::LaunchInspector ||
      m_ButtonType == ButtonType::OpenVsCode ||
      m_ButtonType == ButtonType::InputConfig ||
      m_ButtonType == ButtonType::AssetProfiles ||
      m_ButtonType == ButtonType::SetupCppProject ||
      m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject ||
      m_ButtonType == ButtonType::ExportProject ||
      m_ButtonType == ButtonType::ClearAssetCaches ||
      m_ButtonType == ButtonType::PluginSelection)
  {
    SetEnabled(plToolsProject::IsProjectOpen());

    plToolsProject::s_Events.AddEventHandler(plMakeDelegate(&plProjectAction::ProjectEventHandler, this));
  }

  if (m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject)
  {
    SetEnabled(plCppProject::ExistsProjectCMakeListsTxt());

    plCppProject::s_ChangeEvents.AddEventHandler(plMakeDelegate(&plProjectAction::CppEventHandler, this));
  }
}

plProjectAction::~plProjectAction()
{
  if (m_ButtonType == ButtonType::CloseProject ||
      m_ButtonType == ButtonType::DataDirectories ||
      m_ButtonType == ButtonType::WindowConfig ||
      m_ButtonType == ButtonType::ImportAsset ||
      m_ButtonType == ButtonType::TagsDialog ||
      m_ButtonType == ButtonType::ReloadEngine ||
      m_ButtonType == ButtonType::ReloadResources ||
      m_ButtonType == ButtonType::LaunchFileserve ||
      m_ButtonType == ButtonType::LaunchInspector ||
      m_ButtonType == ButtonType::OpenVsCode ||
      m_ButtonType == ButtonType::InputConfig ||
      m_ButtonType == ButtonType::AssetProfiles ||
      m_ButtonType == ButtonType::SetupCppProject ||
      m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject ||
      m_ButtonType == ButtonType::ExportProject ||
      m_ButtonType == ButtonType::ClearAssetCaches ||
      m_ButtonType == ButtonType::PluginSelection)
  {
    plToolsProject::s_Events.RemoveEventHandler(plMakeDelegate(&plProjectAction::ProjectEventHandler, this));
  }

  if (m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject)
  {
    plCppProject::s_ChangeEvents.RemoveEventHandler(plMakeDelegate(&plProjectAction::CppEventHandler, this));
  }
}

void plProjectAction::ProjectEventHandler(const plToolsProjectEvent& e)
{
  if (m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject)
  {
    SetEnabled(plCppProject::ExistsProjectCMakeListsTxt());
  }
  else
  {
    SetEnabled(plToolsProject::IsProjectOpen());
  }
}

void plProjectAction::CppEventHandler(const plCppSettings& e)
{
  if (m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject)
  {
    SetEnabled(plCppProject::ExistsProjectCMakeListsTxt());
  }
}

void plProjectAction::Execute(const plVariant& value)
{
  switch (m_ButtonType)
  {
    case plProjectAction::ButtonType::CreateDocument:
      plQtEditorApp::GetSingleton()->GuiCreateDocument();
      break;

    case plProjectAction::ButtonType::OpenDocument:
      plQtEditorApp::GetSingleton()->GuiOpenDocument();
      break;

    case plProjectAction::ButtonType::OpenDashboard:
      plQtEditorApp::GetSingleton()->GuiOpenDashboard();
      break;

    case plProjectAction::ButtonType::CreateProject:
      plQtEditorApp::GetSingleton()->GuiCreateProject();
      break;

    case plProjectAction::ButtonType::OpenProject:
      plQtEditorApp::GetSingleton()->GuiOpenProject();
      break;

    case plProjectAction::ButtonType::CloseProject:
    {
      if (plToolsProject::CanCloseProject())
        plQtEditorApp::GetSingleton()->CloseProject();
    }
    break;

    case plProjectAction::ButtonType::DataDirectories:
    {
      plQtDataDirsDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case plProjectAction::ButtonType::WindowConfig:
    {
      plQtWindowCfgDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case plProjectAction::ButtonType::ImportAsset:
    {
      plAssetDocumentGenerator::ImportAssets();
    }
    break;

    case plProjectAction::ButtonType::InputConfig:
    {
      plQtInputConfigDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        plToolsProject::BroadcastConfigChanged();
      }
    }
    break;

    case plProjectAction::ButtonType::PluginSelection:
    {
      plQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(plOSFile::GetApplicationDirectory());

      plCppSettings cppSettings;
      if (cppSettings.Load().Succeeded())
      {
        plQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(plCppProject::GetPluginSourceDir(cppSettings));
      }

      plQtPluginSelectionDlg dlg(&plQtEditorApp::GetSingleton()->GetPluginBundles());
      dlg.exec();

      plToolsProject::SaveProjectState();
    }
    break;

    case plProjectAction::ButtonType::PreferencesDialog:
    {
      plQtPreferencesDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        // save modified preferences right away
        plToolsProject::SaveProjectState();

        plToolsProject::BroadcastConfigChanged();
      }
    }
    break;

    case plProjectAction::ButtonType::TagsDialog:
    {
      plQtTagsDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        plToolsProject::BroadcastConfigChanged();
      }
    }
    break;

    case plProjectAction::ButtonType::ExportProject:
    {
      plQtExportProjectDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case plProjectAction::ButtonType::ClearAssetCaches:
    {
      auto res = plQtUiServices::GetSingleton()->MessageBoxQuestion("Delete ALL cached asset files?\n\n* 'Yes All' deletes everything and takes a long time to re-process. This is rarely needed.\n* 'No All' only deletes assets that are likely to make problems.", QMessageBox::StandardButton::YesAll | QMessageBox::StandardButton::NoAll | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel);

      if (res == QMessageBox::StandardButton::Cancel)
        break;

      if (res == QMessageBox::StandardButton::YesAll)
        plAssetCurator::GetSingleton()->ClearAssetCaches(plAssetDocumentManager::Perfect);
      else
        plAssetCurator::GetSingleton()->ClearAssetCaches(plAssetDocumentManager::Unknown);
    }
    break;

    case plProjectAction::ButtonType::Shortcuts:
    {
      plQtShortcutEditorDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case plProjectAction::ButtonType::ReloadResources:
    {
      plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Reloading Resources...", plTime::MakeFromSeconds(5));

      plSimpleConfigMsgToEngine msg;
      msg.m_sWhatToDo = "ReloadResources";
      msg.m_sPayload = "ReloadAllResources";
      plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);

      plEditorAppEvent e;
      e.m_Type = plEditorAppEvent::Type::ReloadResources;
      plQtEditorApp::GetSingleton()->m_Events.Broadcast(e);

      // keep this here to make live color palette editing available, when needed
      if (false)
      {
        QTimer::singleShot(1, [this]() { plQtEditorApp::GetSingleton()->SetStyleSheet(); });
        QTimer::singleShot(500, [this]() { plQtEditorApp::GetSingleton()->SetStyleSheet(); });
      }

      if (m_Context.m_pDocument)
      {
        m_Context.m_pDocument->ShowDocumentStatus("Reloading Resources");
      }

      plTranslator::ReloadAllTranslators();
    }
    break;

    case plProjectAction::ButtonType::LaunchFileserve:
    {
      plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching FileServe...", plTime::MakeFromSeconds(5));

      plQtLaunchFileserveDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case plProjectAction::ButtonType::LaunchInspector:
    {
      plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching plInspector...", plTime::MakeFromSeconds(5));

      plQtEditorApp::GetSingleton()->RunInspector();
    }
    break;

    case plProjectAction::ButtonType::ReloadEngine:
    {
      plEditorEngineProcessConnection::GetSingleton()->RestartProcess().IgnoreResult();
    }
    break;

    case plProjectAction::ButtonType::SaveProfiling:
    {
      const char* szEditorProfilingFile = ":appdata/profilingEditor.json";
      {
        // Start capturing profiling data on engine process
        plSimpleConfigMsgToEngine msg;
        msg.m_sWhatToDo = "SaveProfiling";
        msg.m_sPayload = ":appdata/profilingEngine.json";
        plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
      }
      if (plProfilingUtils::SaveProfilingCapture(szEditorProfilingFile).Failed())
        return;

      plStringBuilder sEngineProfilingFile;
      {
        // Wait for engine process response
        auto callback = [&](plProcessMessage* pMsg) -> bool {
          auto pSimpleCfg = static_cast<plSaveProfilingResponseToEditor*>(pMsg);
          sEngineProfilingFile = pSimpleCfg->m_sProfilingFile;
          return true;
        };
        plProcessCommunicationChannel::WaitForMessageCallback cb = callback;

        if (plEditorEngineProcessConnection::GetSingleton()->WaitForMessage(plGetStaticRTTI<plSaveProfilingResponseToEditor>(), plTime::MakeFromSeconds(15), &cb).Failed())
        {
          plLog::Error("Timeout while waiting for engine process to create profiling capture. Captures will not be merged.");
          return;
        }
        if (sEngineProfilingFile.IsEmpty())
        {
          plLog::Error("Engine process failed to create profiling file.");
          return;
        }
      }

      plStringBuilder sMergedFile;
      const plDateTime dt = plDateTime::MakeFromTimestamp(plTimestamp::CurrentTimestamp());
      sMergedFile.AppendFormat(":appdata/profiling_{0}-{1}-{2}_{3}-{4}-{5}-{6}.json", dt.GetYear(), plArgU(dt.GetMonth(), 2, true), plArgU(dt.GetDay(), 2, true), plArgU(dt.GetHour(), 2, true), plArgU(dt.GetMinute(), 2, true), plArgU(dt.GetSecond(), 2, true), plArgU(dt.GetMicroseconds() / 1000, 3, true));

      plStringBuilder sAbsPath;
      if (plProfilingUtils::MergeProfilingCaptures(sEngineProfilingFile, szEditorProfilingFile, sMergedFile).Succeeded() && plFileSystem::ResolvePath(sMergedFile, &sAbsPath, nullptr).Succeeded())
      {
        plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(plFmt("Merged profiling capture saved to '{0}'.", sAbsPath), plTime::MakeFromSeconds(5.0));
      }
    }
    break;

    case plProjectAction::ButtonType::OpenVsCode:
    {
      QStringList args;

      for (const auto& dd : plQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
      {
        plStringBuilder path;
        plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path).IgnoreResult();

        args.append(QString::fromUtf8(path, path.GetElementCount()));
      }

      const plStatus res = plQtUiServices::OpenInVsCode(args);

      plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Failed to open VS Code");
    }
    break;

    case plProjectAction::ButtonType::AssetProfiles:
    {
      plQtAssetProfilesDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        // we need to force the asset status reevaluation because when the profile settings have changed,
        // we need to figure out which assets are now out of date
        plAssetCurator::GetSingleton()->SetActiveAssetProfileByIndex(dlg.m_uiActiveConfig, true);

        // makes the scene re-select the current objects, which updates which enum values are shown in the property grid
        plToolsProject::BroadcastConfigChanged();
      }
    }
    break;

    case plProjectAction::ButtonType::SetupCppProject:
    {
      plQtCppProjectDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case plProjectAction::ButtonType::OpenCppProject:
    {
      plCppSettings cpp;
      cpp.Load().IgnoreResult();

      if (plCppProject::ExistsProjectCMakeListsTxt())
      {
        if (plCppProject::RunCMakeIfNecessary(cpp).Failed())
        {
          plQtUiServices::GetSingleton()->MessageBoxWarning("Generating the C++ solution failed.");
        }
        else
        {
          if(auto status = plCppProject::OpenSolution(cpp); status.Failed())
          {
            plQtUiServices::GetSingleton()->MessageBoxWarning(status.m_sMessage.GetView());
          }
        }
      }
      else
      {
        plQtUiServices::GetSingleton()->MessageBoxInformation("C++ code has not been set up, opening a solution is not possible.");
      }
    }
    break;

    case plProjectAction::ButtonType::CompileCppProject:
    {
      plCppSettings cpp;
      cpp.Load().IgnoreResult();

      if (plCppProject::ExistsProjectCMakeListsTxt())
      {
        if (plCppProject::BuildCodeIfNecessary(cpp).Succeeded())
        {
          plQtUiServices::GetSingleton()->MessageBoxInformation("Successfully compiled the C++ code.");
        }
        else
        {
          plQtUiServices::GetSingleton()->MessageBoxWarning("Compiling the code failed. See log for details.");
        }
      }
      else
      {
        plQtUiServices::GetSingleton()->MessageBoxInformation("C++ code has not been set up, compilation is not possible (or necessary).");
      }
    }
    break;

    case plProjectAction::ButtonType::RegenerateCppSolution:
    {
      plCppSettings cpp;
      cpp.Load().IgnoreResult();

      if (!plCppProject::ExistsProjectCMakeListsTxt() || !plCppProject::ExistsSolution(cpp))
      {
        plQtCppProjectDlg dlg(nullptr);
        dlg.exec();
      }
      else
      {
        if (plCppProject::RunCMake(cpp).Succeeded())
        {
          plQtUiServices::GetSingleton()->MessageBoxInformation("Successfully regenerated the C++ solution.");
        }
        else
        {
          plQtUiServices::GetSingleton()->MessageBoxWarning("Regenerating the solution failed. See log for details.");
        }
      }
    }
    break;

    case plProjectAction::ButtonType::ShowDocsAndCommunity:
      plQtEditorApp::GetSingleton()->GuiOpenDocsAndCommunity();
      break;
  }
}

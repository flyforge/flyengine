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
#include <GuiFoundation/Dialogs/ShortcutEditorDlg.moc.h>

plActionDescriptorHandle plProjectActions::s_hProjectMenu;
plActionDescriptorHandle plProjectActions::s_hCategoryGeneral;
plActionDescriptorHandle plProjectActions::s_hCategoryAssets;
plActionDescriptorHandle plProjectActions::s_hCategoryConfig;


plActionDescriptorHandle plProjectActions::s_hDocumentCategory;
plActionDescriptorHandle plProjectActions::s_hCreateDocument;
plActionDescriptorHandle plProjectActions::s_hOpenDocument;
plActionDescriptorHandle plProjectActions::s_hRecentDocuments;

plActionDescriptorHandle plProjectActions::s_hProjectCategory;
plActionDescriptorHandle plProjectActions::s_hOpenDashboard;
plActionDescriptorHandle plProjectActions::s_hCreateProject;
plActionDescriptorHandle plProjectActions::s_hOpenProject;
plActionDescriptorHandle plProjectActions::s_hRecentProjects;
plActionDescriptorHandle plProjectActions::s_hCloseProject;
plActionDescriptorHandle plProjectActions::s_hDocsAndCommunity;

plActionDescriptorHandle plProjectActions::s_hSettingsCategory;
//plActionDescriptorHandle plProjectActions::s_hEditorSettingsMenu;
plActionDescriptorHandle plProjectActions::s_hProjectSettingsMenu;
plActionDescriptorHandle plProjectActions::s_hShortcutEditor;
plActionDescriptorHandle plProjectActions::s_hDataDirectories;
plActionDescriptorHandle plProjectActions::s_hWindowConfig;
plActionDescriptorHandle plProjectActions::s_hInputConfig;
plActionDescriptorHandle plProjectActions::s_hPreferencesDlg;
plActionDescriptorHandle plProjectActions::s_hTagsDlg;
plActionDescriptorHandle plProjectActions::s_hImportAsset;
plActionDescriptorHandle plProjectActions::s_hAssetProfiles;
plActionDescriptorHandle plProjectActions::s_hExportProject;
plActionDescriptorHandle plProjectActions::s_hPluginSelection;

plActionDescriptorHandle plProjectActions::s_hToolsMenu;
plActionDescriptorHandle plProjectActions::s_hToolsCategory;
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
  s_hProjectMenu = PLASMA_REGISTER_MENU("Menu.Editor");
  s_hCategoryGeneral = PLASMA_REGISTER_CATEGORY("General");
  s_hCategoryAssets = PLASMA_REGISTER_CATEGORY("Assets");
  s_hCategoryConfig = PLASMA_REGISTER_CATEGORY("Config");

  s_hDocumentCategory = PLASMA_REGISTER_CATEGORY("DocumentCategory");
  s_hCreateDocument = PLASMA_REGISTER_ACTION_1("Document.Create", plActionScope::Global, "Project", "Ctrl+N", plProjectAction, plProjectAction::ButtonType::CreateDocument);
  s_hOpenDocument = PLASMA_REGISTER_ACTION_1("Document.Open", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::OpenDocument);
  s_hRecentDocuments = PLASMA_REGISTER_DYNAMIC_MENU("Project.RecentDocuments.Menu", plRecentDocumentsMenuAction, "");

  s_hProjectCategory = PLASMA_REGISTER_CATEGORY("ProjectCategory");
  s_hOpenDashboard = PLASMA_REGISTER_ACTION_1("Editor.OpenDashboard", plActionScope::Global, "Editor", "Ctrl+Shift+D", plProjectAction, plProjectAction::ButtonType::OpenDashboard);
  s_hCreateProject = PLASMA_REGISTER_ACTION_1("Project.Create", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::CreateProject);
  s_hOpenProject = PLASMA_REGISTER_ACTION_1("Project.Open", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::OpenProject);
  s_hRecentProjects = PLASMA_REGISTER_DYNAMIC_MENU("Project.RecentProjects.Menu", plRecentProjectsMenuAction, "");
  s_hCloseProject = PLASMA_REGISTER_ACTION_1("Project.Close", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::CloseProject);

  s_hSettingsCategory = PLASMA_REGISTER_CATEGORY("SettingsCategory");
  //s_hEditorSettingsMenu = PLASMA_REGISTER_MENU_WITH_ICON("Menu.EditorSettings", ":/GuiFoundation/Icons/Settings.svg");
  s_hProjectSettingsMenu = PLASMA_REGISTER_MENU("Menu.ProjectSettings");

  s_hShortcutEditor = PLASMA_REGISTER_ACTION_1("Editor.Shortcuts", plActionScope::Global, "Editor", "", plProjectAction, plProjectAction::ButtonType::Shortcuts);
  s_hPreferencesDlg = PLASMA_REGISTER_ACTION_1("Editor.Preferences", plActionScope::Global, "Editor", "", plProjectAction, plProjectAction::ButtonType::PreferencesDialog);
  s_hTagsDlg = PLASMA_REGISTER_ACTION_1("Engine.Tags", plActionScope::Global, "Editor", "", plProjectAction, plProjectAction::ButtonType::TagsDialog);
  s_hPluginSelection = PLASMA_REGISTER_ACTION_1("Project.PluginSelection", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::PluginSelection);

  s_hDataDirectories = PLASMA_REGISTER_ACTION_1("Project.DataDirectories", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::DataDirectories);
  s_hInputConfig = PLASMA_REGISTER_ACTION_1("Project.InputConfig", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::InputConfig);
  s_hWindowConfig = PLASMA_REGISTER_ACTION_1("Project.WindowConfig", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::WindowConfig);
  s_hImportAsset = PLASMA_REGISTER_ACTION_1("Project.ImportAsset", plActionScope::Global, "Project", "Ctrl+I", plProjectAction, plProjectAction::ButtonType::ImportAsset);
  s_hAssetProfiles = PLASMA_REGISTER_ACTION_1("Project.AssetProfiles", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::AssetProfiles);
  s_hExportProject = PLASMA_REGISTER_ACTION_1("Project.ExportProject", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::ExportProject);

  s_hToolsMenu = PLASMA_REGISTER_MENU("Menu.Tools");
  s_hToolsCategory = PLASMA_REGISTER_CATEGORY("ToolsCategory");
  s_hReloadResources = PLASMA_REGISTER_ACTION_1("Engine.ReloadResources", plActionScope::Global, "Engine", "F4", plProjectAction, plProjectAction::ButtonType::ReloadResources);
  s_hReloadEngine = PLASMA_REGISTER_ACTION_1("Engine.ReloadEngine", plActionScope::Global, "Engine", "Ctrl+Shift+F4", plProjectAction, plProjectAction::ButtonType::ReloadEngine);
  s_hLaunchFileserve = PLASMA_REGISTER_ACTION_1("Editor.LaunchFileserve", plActionScope::Global, "Engine", "", plProjectAction, plProjectAction::ButtonType::LaunchFileserve);
  s_hLaunchInspector = PLASMA_REGISTER_ACTION_1("Editor.LaunchInspector", plActionScope::Global, "Engine", "", plProjectAction, plProjectAction::ButtonType::LaunchInspector);
  s_hSaveProfiling = PLASMA_REGISTER_ACTION_1("Editor.SaveProfiling", plActionScope::Global, "Engine", "Ctrl+Alt+P", plProjectAction, plProjectAction::ButtonType::SaveProfiling);
  s_hOpenVsCode = PLASMA_REGISTER_ACTION_1("Editor.OpenVsCode", plActionScope::Global, "Project", "Ctrl+Alt+O", plProjectAction, plProjectAction::ButtonType::OpenVsCode);


  s_hCppProjectMenu = PLASMA_REGISTER_MENU("Project.Cpp");
  s_hSetupCppProject = PLASMA_REGISTER_ACTION_1("Project.SetupCppProject", plActionScope::Global, "Project", "", plProjectAction, plProjectAction::ButtonType::SetupCppProject);
  s_hOpenCppProject = PLASMA_REGISTER_ACTION_1("Project.OpenCppProject", plActionScope::Global, "Project", "Ctrl+Shift+O", plProjectAction, plProjectAction::ButtonType::OpenCppProject);
  s_hCompileCppProject = PLASMA_REGISTER_ACTION_1("Project.CompileCppProject", plActionScope::Global, "Project", "Ctrl+Shift+B", plProjectAction, plProjectAction::ButtonType::CompileCppProject);
  s_hRegenerateCppSolution = PLASMA_REGISTER_ACTION_1("Project.RegenerateCppSolution", plActionScope::Global, "Project", "Ctrl+Shift+G", plProjectAction, plProjectAction::ButtonType::RegenerateCppSolution);

  s_hDocsAndCommunity = PLASMA_REGISTER_ACTION_1("Editor.DocsAndCommunity", plActionScope::Global, "Editor", "", plProjectAction, plProjectAction::ButtonType::ShowDocsAndCommunity);
}

void plProjectActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hProjectMenu);
  plActionManager::UnregisterAction(s_hCategoryGeneral);
  plActionManager::UnregisterAction(s_hCategoryAssets);
  plActionManager::UnregisterAction(s_hCategoryConfig);
  plActionManager::UnregisterAction(s_hDocumentCategory);
  plActionManager::UnregisterAction(s_hCreateDocument);
  plActionManager::UnregisterAction(s_hOpenDocument);
  plActionManager::UnregisterAction(s_hRecentDocuments);
  plActionManager::UnregisterAction(s_hProjectCategory);
  plActionManager::UnregisterAction(s_hOpenDashboard);
  plActionManager::UnregisterAction(s_hDocsAndCommunity);
  plActionManager::UnregisterAction(s_hCreateProject);
  plActionManager::UnregisterAction(s_hOpenProject);
  plActionManager::UnregisterAction(s_hRecentProjects);
  plActionManager::UnregisterAction(s_hCloseProject);
  plActionManager::UnregisterAction(s_hSettingsCategory);
  //plActionManager::UnregisterAction(s_hEditorSettingsMenu);
  plActionManager::UnregisterAction(s_hProjectSettingsMenu);
  plActionManager::UnregisterAction(s_hToolsMenu);
  plActionManager::UnregisterAction(s_hToolsCategory);
  plActionManager::UnregisterAction(s_hReloadResources);
  plActionManager::UnregisterAction(s_hReloadEngine);
  plActionManager::UnregisterAction(s_hLaunchFileserve);
  plActionManager::UnregisterAction(s_hLaunchInspector);
  plActionManager::UnregisterAction(s_hSaveProfiling);
  plActionManager::UnregisterAction(s_hOpenVsCode);
  plActionManager::UnregisterAction(s_hShortcutEditor);
  plActionManager::UnregisterAction(s_hPreferencesDlg);
  plActionManager::UnregisterAction(s_hTagsDlg);
  plActionManager::UnregisterAction(s_hDataDirectories);
  plActionManager::UnregisterAction(s_hWindowConfig);
  plActionManager::UnregisterAction(s_hImportAsset);
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

void plProjectActions::MapActions(const char* szMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hProjectMenu, "", -1000000000.0f);
  pMap->MapAction(s_hCategoryGeneral, "Menu.Editor", 1.0f);
  pMap->MapAction(s_hCategoryAssets, "Menu.Editor", 2.0f);
  pMap->MapAction(s_hCategoryConfig, "Menu.Editor", 3.0f);

  pMap->MapAction(s_hDocumentCategory, "Menu.File", 1.0f);
  pMap->MapAction(s_hCreateDocument, "Menu.File/DocumentCategory", 1.0f);
  pMap->MapAction(s_hOpenDocument, "Menu.File/DocumentCategory", 2.0f);
  pMap->MapAction(s_hRecentDocuments, "Menu.File/DocumentCategory", 4.0f);

  pMap->MapAction(s_hProjectCategory, "Menu.Editor", 2.0f);
  pMap->MapAction(s_hOpenDashboard, "Menu.Editor/ProjectCategory", 0.5f);
  pMap->MapAction(s_hImportAsset, "Menu.Editor/ProjectCategory", 1.0f);
  // pMap->MapAction(s_hCreateProject, "Menu.Editor/ProjectCategory", 1.0f); // use dashboard
  // pMap->MapAction(s_hOpenProject, "Menu.Editor/ProjectCategory", 2.0f);   // use dashboard
  // pMap->MapAction(s_hRecentProjects, "Menu.Editor/ProjectCategory", 3.0f);// use dashboard
  pMap->MapAction(s_hCloseProject, "Menu.Editor/ProjectCategory", 4.0f);
  pMap->MapAction(s_hExportProject, "Menu.Editor/ProjectCategory", 6.0f);
  pMap->MapAction(s_hProjectSettingsMenu, "Menu.Editor/ProjectCategory", 1000.0f);

  pMap->MapAction(s_hCppProjectMenu, "Menu.Editor/ProjectCategory", 5.0f);
  pMap->MapAction(s_hSetupCppProject, "Menu.Editor/ProjectCategory/Project.Cpp", 1.0f);
  pMap->MapAction(s_hOpenCppProject, "Menu.Editor/ProjectCategory/Project.Cpp", 2.0f);
  pMap->MapAction(s_hCompileCppProject, "Menu.Editor/ProjectCategory/Project.Cpp", 3.0f);
  pMap->MapAction(s_hRegenerateCppSolution, "Menu.Editor/ProjectCategory/Project.Cpp", 4.0f);

  pMap->MapAction(s_hSettingsCategory, "Menu.Editor", 3.0f);
  //pMap->MapAction(s_hEditorSettingsMenu, "Menu.Editor/SettingsCategory", 1.0f);

  pMap->MapAction(s_hToolsMenu, "", 4.5f);
  pMap->MapAction(s_hToolsCategory, "Menu.Tools", 1.0f);
  pMap->MapAction(s_hReloadResources, "Menu.Tools/ToolsCategory", 1.0f);
  pMap->MapAction(s_hReloadEngine, "Menu.Tools/ToolsCategory", 2.0f);
  pMap->MapAction(s_hLaunchFileserve, "Menu.Tools/ToolsCategory", 3.0f);
  pMap->MapAction(s_hLaunchInspector, "Menu.Tools/ToolsCategory", 3.5f);
  pMap->MapAction(s_hSaveProfiling, "Menu.Tools/ToolsCategory", 4.0f);
  pMap->MapAction(s_hOpenVsCode, "Menu.Tools/ToolsCategory", 5.0f);

  pMap->MapAction(s_hShortcutEditor, "Menu.Tools/ToolsCategory", 1000.0f);
  pMap->MapAction(s_hPreferencesDlg, "Menu.Tools/ToolsCategory", 10001.0f);

  pMap->MapAction(s_hPluginSelection, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 0.5f);
  pMap->MapAction(s_hDataDirectories, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 1.0f);
  pMap->MapAction(s_hInputConfig, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 3.0f);
  pMap->MapAction(s_hTagsDlg, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 4.0f);
  pMap->MapAction(s_hWindowConfig, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 5.0f);
  pMap->MapAction(s_hAssetProfiles, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 6.0f);

  pMap->MapAction(s_hDocsAndCommunity, "Menu.Help", 0.0f);
}

////////////////////////////////////////////////////////////////////////
// plRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRecentDocumentsMenuAction, 0, plRTTINoAllocator)
  ;
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


void plRecentDocumentsMenuAction::GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

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

      out_Entries.PushBack(item);
    }
    else
    {
      item.m_sDisplay = file.m_File;

      out_Entries.PushBack(item);
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

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRecentProjectsMenuAction, 1, plRTTINoAllocator)
  ;
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


void plRecentProjectsMenuAction::GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

  if (plQtEditorApp::GetSingleton()->GetRecentProjectsList().GetFileList().IsEmpty())
    return;

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

    out_Entries.PushBack(item);
  }
}

void plRecentProjectsMenuAction::Execute(const plVariant& value)
{
  plQtEditorApp::GetSingleton()->OpenProject(value.ConvertTo<plString>()).IgnoreResult();
}

////////////////////////////////////////////////////////////////////////
// plProjectAction
////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProjectAction, 1, plRTTINoAllocator)
  ;
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

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
    case plProjectAction::ButtonType::OpenCppProject:
      SetIconPath(":/EditorFramework/Icons/VisualStudio.svg");
      break;
    case plProjectAction::ButtonType::CompileCppProject:
      SetIconPath(":/EditorFramework/Icons/VisualStudio.svg");
      break;
    case plProjectAction::ButtonType::RegenerateCppSolution:
      SetIconPath(":/EditorFramework/Icons/VisualStudio.svg");
    break;
    case plProjectAction::ButtonType::SaveProfiling:
      // no icon
      break;
    case plProjectAction::ButtonType::SetupCppProject:
      SetIconPath(":/EditorFramework/Icons/VisualStudio.svg");
      break;
    case plProjectAction::ButtonType::ShowDocsAndCommunity:
      //SetIconPath(":/GuiFoundation/Icons/Project.svg"); // TODO
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
  SetEnabled(plCppProject::ExistsProjectCMakeListsTxt());
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
      if (dlg.exec() == QDialog::Accepted)
      {
        // TODO
      }
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
      plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Reloading Resources...", plTime::Seconds(5));

      plSimpleConfigMsgToEngine msg;
      msg.m_sWhatToDo = "ReloadResources";
      PlasmaEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);

      PlasmaEditorAppEvent e;
      e.m_Type = PlasmaEditorAppEvent::Type::ReloadResources;
      plQtEditorApp::GetSingleton()->m_Events.Broadcast(e);

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
      plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching FileServe...", plTime::Seconds(5));

      plQtLaunchFileserveDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case plProjectAction::ButtonType::LaunchInspector:
    {
      plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching Plasma Inspector...", plTime::Seconds(5));

      plQtEditorApp::GetSingleton()->RunInspector();
    }
    break;

    case plProjectAction::ButtonType::ReloadEngine:
    {
      PlasmaEditorEngineProcessConnection::GetSingleton()->RestartProcess().IgnoreResult();
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
        PlasmaEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
      }
      {
        // Capture profiling data on editor process
        plFileWriter fileWriter;
        if (fileWriter.Open(szEditorProfilingFile) == PLASMA_SUCCESS)
        {
          plProfilingSystem::ProfilingData profilingData;
          plProfilingSystem::Capture(profilingData);
          // Set sort index to -1 so that the editor is always on top when opening the trace.
          profilingData.m_uiProcessSortIndex = -1;
          if (profilingData.Write(fileWriter).Failed())
          {
            plLog::Error("Failed to write editor profiling capture: {}.", szEditorProfilingFile);
            return;
          }

          plLog::Info("Editor profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
        }
        else
        {
          plLog::Error("Could not write profiling capture to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
        }
      }
      plStringBuilder sEngineProfilingFile;
      {
        // Wait for engine process response
        auto callback = [&](plProcessMessage* pMsg) -> bool
        {
          auto pSimpleCfg = static_cast<plSaveProfilingResponseToEditor*>(pMsg);
          sEngineProfilingFile = pSimpleCfg->m_sProfilingFile;
          return true;
        };
        plProcessCommunicationChannel::WaitForMessageCallback cb = callback;

        if (PlasmaEditorEngineProcessConnection::GetSingleton()->WaitForMessage(plGetStaticRTTI<plSaveProfilingResponseToEditor>(), plTime::Seconds(15), &cb).Failed())
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

      // Merge editor and engine profiling files by simply merging the arrays inside
      {
        plString sEngineProfilingJson;
        {
          plFileReader reader;
          if (reader.Open(sEngineProfilingFile).Failed())
          {
            plLog::Error("Failed to read engine profiling capture: {}.", sEngineProfilingFile);
            return;
          }
          sEngineProfilingJson.ReadAll(reader);
        }
        plString sEditorProfilingJson;
        {
          plFileReader reader;
          if (reader.Open(szEditorProfilingFile).Failed())
          {
            plLog::Error("Failed to read editor profiling capture: {}.", sEngineProfilingFile);
            return;
          }
          sEditorProfilingJson.ReadAll(reader);
        }

        plStringBuilder sMergedProfilingJson;
        {
          // Just glue the array together
          sMergedProfilingJson.Reserve(sEngineProfilingJson.GetElementCount() + 1 + sEditorProfilingJson.GetElementCount());
          const char* szEndArray = sEngineProfilingJson.FindLastSubString("]");
          sMergedProfilingJson.Append(plStringView(sEngineProfilingJson.GetData(), szEndArray - sEngineProfilingJson.GetData()));
          sMergedProfilingJson.Append(",");
          const char* szStartArray = sEditorProfilingJson.FindSubString("[") + 1;
          sMergedProfilingJson.Append(plStringView(szStartArray, sEditorProfilingJson.GetElementCount() - (szStartArray - sEditorProfilingJson.GetData())));
        }
        plStringBuilder sMergedFile;
        const plDateTime dt = plTimestamp::CurrentTimestamp();
        sMergedFile.AppendFormat(":appdata/profiling_{0}-{1}-{2}_{3}-{4}-{5}-{6}.json", dt.GetYear(), plArgU(dt.GetMonth(), 2, true), plArgU(dt.GetDay(), 2, true), plArgU(dt.GetHour(), 2, true), plArgU(dt.GetMinute(), 2, true), plArgU(dt.GetSecond(), 2, true), plArgU(dt.GetMicroseconds() / 1000, 3, true));
        plFileWriter fileWriter;
        if (fileWriter.Open(sMergedFile).Failed() || fileWriter.WriteBytes(sMergedProfilingJson.GetData(), sMergedProfilingJson.GetElementCount()).Failed())
        {
          plLog::Error("Failed to write merged profiling capture: {}.", sMergedFile);
          return;
        }
        plLog::Info("Merged profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
        plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(plFmt("Merged profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData()), plTime::Seconds(5.0));
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
        plQtUiServices::GetSingleton()->MessageBoxInformation("C++ code has not been set up, compilation is not necessary.");
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

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class plCppSettings;

///
class PLASMA_EDITORFRAMEWORK_DLL plProjectActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping);

  static plActionDescriptorHandle s_hProjectMenu;
  static plActionDescriptorHandle s_hCategoryGeneral;
  static plActionDescriptorHandle s_hCategoryAssets;
  static plActionDescriptorHandle s_hCategoryConfig;


  static plActionDescriptorHandle s_hDocumentCategory;
  static plActionDescriptorHandle s_hCreateDocument;
  static plActionDescriptorHandle s_hOpenDocument;
  static plActionDescriptorHandle s_hRecentDocuments;

  static plActionDescriptorHandle s_hProjectCategory;
  static plActionDescriptorHandle s_hOpenDashboard;
  static plActionDescriptorHandle s_hCreateProject;
  static plActionDescriptorHandle s_hOpenProject;
  static plActionDescriptorHandle s_hRecentProjects;
  static plActionDescriptorHandle s_hCloseProject;

  static plActionDescriptorHandle s_hDocsAndCommunity;

  static plActionDescriptorHandle s_hSettingsCategory;
  //static plActionDescriptorHandle s_hEditorSettingsMenu;
  static plActionDescriptorHandle s_hProjectSettingsMenu;
  static plActionDescriptorHandle s_hShortcutEditor;
  static plActionDescriptorHandle s_hDataDirectories;
  static plActionDescriptorHandle s_hWindowConfig;
  static plActionDescriptorHandle s_hInputConfig;
  static plActionDescriptorHandle s_hPreferencesDlg;
  static plActionDescriptorHandle s_hTagsDlg;
  static plActionDescriptorHandle s_hAssetProfiles;
  static plActionDescriptorHandle s_hExportProject;
  static plActionDescriptorHandle s_hPluginSelection;

  static plActionDescriptorHandle s_hToolsMenu;
  static plActionDescriptorHandle s_hToolsCategory;
  static plActionDescriptorHandle s_hReloadResources;
  static plActionDescriptorHandle s_hReloadEngine;
  static plActionDescriptorHandle s_hLaunchFileserve;
  static plActionDescriptorHandle s_hLaunchInspector;
  static plActionDescriptorHandle s_hSaveProfiling;
  static plActionDescriptorHandle s_hOpenVsCode;
  static plActionDescriptorHandle s_hImportAsset;

  static plActionDescriptorHandle s_hCppProjectMenu;
  static plActionDescriptorHandle s_hSetupCppProject;
  static plActionDescriptorHandle s_hOpenCppProject;
  static plActionDescriptorHandle s_hCompileCppProject;
};

///
class PLASMA_EDITORFRAMEWORK_DLL plRecentDocumentsMenuAction : public plDynamicMenuAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRecentDocumentsMenuAction, plDynamicMenuAction);

public:
  plRecentDocumentsMenuAction(const plActionContext& context, const char* szName, const char* szIconPath)
    : plDynamicMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_Entries) override;
  virtual void Execute(const plVariant& value) override;
};

///
class PLASMA_EDITORFRAMEWORK_DLL plRecentProjectsMenuAction : public plDynamicMenuAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRecentProjectsMenuAction, plDynamicMenuAction);

public:
  plRecentProjectsMenuAction(const plActionContext& context, const char* szName, const char* szIconPath)
    : plDynamicMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_Entries) override;
  virtual void Execute(const plVariant& value) override;
};

///
class PLASMA_EDITORFRAMEWORK_DLL plProjectAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProjectAction, plButtonAction);

public:
  enum class ButtonType
  {
    CreateDocument,
    OpenDocument,
    OpenDashboard,
    CreateProject,
    OpenProject,
    CloseProject,
    ReloadResources,
    ReloadEngine,
    LaunchFileserve,
    LaunchInspector,
    SaveProfiling,
    OpenVsCode,
    Shortcuts,
    DataDirectories,
    WindowConfig,
    InputConfig,
    PreferencesDialog,
    TagsDialog,
    ImportAsset,
    AssetProfiles,
    SetupCppProject,
    OpenCppProject,
    CompileCppProject,
    ShowDocsAndCommunity,
    ExportProject,
    PluginSelection,
  };

  plProjectAction(const plActionContext& context, const char* szName, ButtonType button);
  ~plProjectAction();

  virtual void Execute(const plVariant& value) override;

private:
  void ProjectEventHandler(const plToolsProjectEvent& e);
  void CppEventHandler(const plCppSettings& e);

  ButtonType m_ButtonType;
};

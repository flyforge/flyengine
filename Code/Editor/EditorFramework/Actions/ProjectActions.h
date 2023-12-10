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

  static void MapActions(plStringView sMapping);

  static plActionDescriptorHandle s_hCatProjectGeneral;
  static plActionDescriptorHandle s_hCatProjectAssets;
  static plActionDescriptorHandle s_hCatProjectConfig;
  static plActionDescriptorHandle s_hCatProjectExternal;

  static plActionDescriptorHandle s_hCatFilesGeneral;
  static plActionDescriptorHandle s_hCatFileCommon;
  static plActionDescriptorHandle s_hCatFileSpecial;
  static plActionDescriptorHandle s_hCatAssetDoc;

  static plActionDescriptorHandle s_hCreateDocument;
  static plActionDescriptorHandle s_hOpenDocument;
  static plActionDescriptorHandle s_hRecentDocuments;

  static plActionDescriptorHandle s_hOpenDashboard;
  static plActionDescriptorHandle s_hCreateProject;
  static plActionDescriptorHandle s_hOpenProject;
  static plActionDescriptorHandle s_hRecentProjects;
  static plActionDescriptorHandle s_hCloseProject;

  static plActionDescriptorHandle s_hDocsAndCommunity;

  static plActionDescriptorHandle s_hCatProjectSettings;
  static plActionDescriptorHandle s_hCatPluginSettings;
  static plActionDescriptorHandle s_hShortcutEditor;
  static plActionDescriptorHandle s_hDataDirectories;
  static plActionDescriptorHandle s_hWindowConfig;
  static plActionDescriptorHandle s_hInputConfig;
  static plActionDescriptorHandle s_hPreferencesDlg;
  static plActionDescriptorHandle s_hTagsConfig;
  static plActionDescriptorHandle s_hAssetProfiles;
  static plActionDescriptorHandle s_hExportProject;
  static plActionDescriptorHandle s_hPluginSelection;

  static plActionDescriptorHandle s_hCatToolsExternal;
  static plActionDescriptorHandle s_hCatToolsEditor;
  static plActionDescriptorHandle s_hCatToolsDocument;
  static plActionDescriptorHandle s_hCatEditorSettings;
  static plActionDescriptorHandle s_hReloadResources;
  static plActionDescriptorHandle s_hReloadEngine;
  static plActionDescriptorHandle s_hLaunchFileserve;
  static plActionDescriptorHandle s_hLaunchInspector;
  static plActionDescriptorHandle s_hSaveProfiling;
  static plActionDescriptorHandle s_hOpenVsCode;
  static plActionDescriptorHandle s_hImportAsset;
  static plActionDescriptorHandle s_hClearAssetCaches;

  static plActionDescriptorHandle s_hCppProjectMenu;
  static plActionDescriptorHandle s_hSetupCppProject;
  static plActionDescriptorHandle s_hOpenCppProject;
  static plActionDescriptorHandle s_hCompileCppProject;
  static plActionDescriptorHandle s_hRegenerateCppSolution;
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
  virtual void GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_entries) override;
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
  virtual void GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_entries) override;
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
    RegenerateCppSolution,
    ShowDocsAndCommunity,
    ExportProject,
    PluginSelection,
    ClearAssetCaches,
  };

  plProjectAction(const plActionContext& context, const char* szName, ButtonType button);
  ~plProjectAction();

  virtual void Execute(const plVariant& value) override;

private:
  void ProjectEventHandler(const plToolsProjectEvent& e);
  void CppEventHandler(const plCppSettings& e);

  ButtonType m_ButtonType;
};

#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plPreferences;

///
class PL_EDITORPLUGINSCENE_DLL plSceneActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(plStringView sMapping);
  static void MapToolbarActions(plStringView sMapping);
  static void MapViewContextMenuActions(plStringView sMapping);

  static plActionDescriptorHandle s_hSceneCategory;
  static plActionDescriptorHandle s_hSceneUtilsMenu;
  static plActionDescriptorHandle s_hExportScene;
  static plActionDescriptorHandle s_hGameModeSimulate;
  static plActionDescriptorHandle s_hGameModePlay;
  static plActionDescriptorHandle s_hGameModePlayFromHere;
  static plActionDescriptorHandle s_hGameModeStop;
  static plActionDescriptorHandle s_hUtilExportSceneToOBJ;
  static plActionDescriptorHandle s_hKeepSimulationChanges;
  static plActionDescriptorHandle s_hCreateThumbnail;
  static plActionDescriptorHandle s_hFavoriteCamsMenu;
  static plActionDescriptorHandle s_hStoreEditorCamera[10];
  static plActionDescriptorHandle s_hRestoreEditorCamera[10];
  static plActionDescriptorHandle s_hJumpToCamera[10];
  static plActionDescriptorHandle s_hCreateLevelCamera[10];
};

///
class PL_EDITORPLUGINSCENE_DLL plSceneAction : public plButtonAction
{
  PL_ADD_DYNAMIC_REFLECTION(plSceneAction, plButtonAction);

public:
  enum class ActionType
  {
    ExportAndRunScene,
    StartGameModeSimulate,
    StartGameModePlay,
    StartGameModePlayFromHere,
    StopGameMode,
    ExportSceneToOBJ,
    KeepSimulationChanges,
    CreateThumbnail,

    StoreEditorCamera0,
    StoreEditorCamera1,
    StoreEditorCamera2,
    StoreEditorCamera3,
    StoreEditorCamera4,
    StoreEditorCamera5,
    StoreEditorCamera6,
    StoreEditorCamera7,
    StoreEditorCamera8,
    StoreEditorCamera9,

    RestoreEditorCamera0,
    RestoreEditorCamera1,
    RestoreEditorCamera2,
    RestoreEditorCamera3,
    RestoreEditorCamera4,
    RestoreEditorCamera5,
    RestoreEditorCamera6,
    RestoreEditorCamera7,
    RestoreEditorCamera8,
    RestoreEditorCamera9,

    JumpToCamera0,
    JumpToCamera1,
    JumpToCamera2,
    JumpToCamera3,
    JumpToCamera4,
    JumpToCamera5,
    JumpToCamera6,
    JumpToCamera7,
    JumpToCamera8,
    JumpToCamera9,

    CreateLevelCamera0,
    CreateLevelCamera1,
    CreateLevelCamera2,
    CreateLevelCamera3,
    CreateLevelCamera4,
    CreateLevelCamera5,
    CreateLevelCamera6,
    CreateLevelCamera7,
    CreateLevelCamera8,
    CreateLevelCamera9,
  };

  plSceneAction(const plActionContext& context, const char* szName, ActionType type);
  ~plSceneAction();

  virtual void Execute(const plVariant& value) override;

  void LaunchPlayer(const char* szPlayerApp);
  QStringList GetPlayerCommandLine(plStringBuilder& out_sSingleLine) const;

private:
  void SceneEventHandler(const plGameObjectEvent& e);
  void UpdateState();

  plSceneDocument* m_pSceneDocument;
  ActionType m_Type;
};

#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

///
class PLASMA_EDITORPLUGINSCENE_DLL plSelectionActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(plStringView sMapping);
  static void MapPrefabActions(plStringView sMapping, float fPriority);
  static void MapContextMenuActions(plStringView sMapping);
  static void MapViewContextMenuActions(plStringView sMapping);

  static plActionDescriptorHandle s_hGroupSelectedItems;
  static plActionDescriptorHandle s_hCreateEmptyChildObject;
  static plActionDescriptorHandle s_hCreateEmptyObjectAtPosition;
  static plActionDescriptorHandle s_hHideSelectedObjects;
  static plActionDescriptorHandle s_hHideUnselectedObjects;
  static plActionDescriptorHandle s_hShowHiddenObjects;
  static plActionDescriptorHandle s_hPrefabMenu;
  static plActionDescriptorHandle s_hCreatePrefab;
  static plActionDescriptorHandle s_hRevertPrefab;
  static plActionDescriptorHandle s_hUnlinkFromPrefab;
  static plActionDescriptorHandle s_hOpenPrefabDocument;
  static plActionDescriptorHandle s_hDuplicateSpecial;
  static plActionDescriptorHandle s_hDeltaTransform;
  static plActionDescriptorHandle s_hSnapObjectToCamera;
  static plActionDescriptorHandle s_hAttachToObject;
  static plActionDescriptorHandle s_hDetachFromParent;
  static plActionDescriptorHandle s_hConvertToEnginePrefab;
  static plActionDescriptorHandle s_hConvertToEditorPrefab;
  static plActionDescriptorHandle s_hCopyReference;
};

///
class PLASMA_EDITORPLUGINSCENE_DLL plSelectionAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSelectionAction, plButtonAction);

public:
  enum class ActionType
  {
    GroupSelectedItems,
    CreateEmptyChildObject,
    CreateEmptyObjectAtPosition,
    HideSelectedObjects,
    HideUnselectedObjects,
    ShowHiddenObjects,

    CreatePrefab,
    RevertPrefab,
    UnlinkFromPrefab,
    OpenPrefabDocument,
    ConvertToEnginePrefab,
    ConvertToEditorPrefab,

    DuplicateSpecial,
    DeltaTransform,
    SnapObjectToCamera,
    AttachToObject,
    DetachFromParent,
    CopyReference,
  };

  plSelectionAction(const plActionContext& context, const char* szName, ActionType type);
  ~plSelectionAction();

  virtual void Execute(const plVariant& value) override;

  void OpenPrefabDocument();

  void CreatePrefab();

private:
  void SelectionEventHandler(const plSelectionManagerEvent& e);

  void UpdateEnableState();

  plSceneDocument* m_pSceneDocument;
  ActionType m_Type;
};

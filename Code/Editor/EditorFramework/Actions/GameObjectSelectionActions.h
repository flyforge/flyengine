#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plGameObjectDocument;

///
class PLASMA_EDITORFRAMEWORK_DLL plGameObjectSelectionActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);
  static void MapContextMenuActions(const char* szMapping, const char* szPath);
  static void MapViewContextMenuActions(const char* szMapping, const char* szPath);

  static plActionDescriptorHandle s_hSelectionCategory;
  static plActionDescriptorHandle s_hShowInScenegraph;
  static plActionDescriptorHandle s_hFocusOnSelection;
  static plActionDescriptorHandle s_hFocusOnSelectionAllViews;
  static plActionDescriptorHandle s_hSnapCameraToObject;
  static plActionDescriptorHandle s_hMoveCameraHere;
  static plActionDescriptorHandle s_hCreateEmptyGameObjectHere;
};

///
class PLASMA_EDITORFRAMEWORK_DLL plGameObjectSelectionAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameObjectSelectionAction, plButtonAction);

public:
  enum class ActionType
  {
    ShowInScenegraph,
    FocusOnSelection,
    FocusOnSelectionAllViews,
    SnapCameraToObject,
    MoveCameraHere,
    CreateGameObjectHere,
  };

  plGameObjectSelectionAction(const plActionContext& context, const char* szName, ActionType type);
  ~plGameObjectSelectionAction();

  virtual void Execute(const plVariant& value) override;

private:
  void SelectionEventHandler(const plSelectionManagerEvent& e);

  void UpdateEnableState();

  plGameObjectDocument* m_pSceneDocument;
  ActionType m_Type;
};

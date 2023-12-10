#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

enum class ActiveGizmo;
class plGameObjectDocument;
struct plSnapProviderEvent;
struct plGameObjectEvent;

///
class PLASMA_EDITORFRAMEWORK_DLL plTransformGizmoActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(plStringView sMapping);
  static void MapToolbarActions(plStringView sMapping);

  static plActionDescriptorHandle s_hGizmoCategory;
  static plActionDescriptorHandle s_hGizmoMenu;
  static plActionDescriptorHandle s_hNoGizmo;
  static plActionDescriptorHandle s_hTranslateGizmo;
  static plActionDescriptorHandle s_hRotateGizmo;
  static plActionDescriptorHandle s_hScaleGizmo;
  static plActionDescriptorHandle s_hDragToPositionGizmo;
  static plActionDescriptorHandle s_hWorldSpace;
  static plActionDescriptorHandle s_hMoveParentOnly;
  static plActionDescriptorHandle s_SnapSettings;
};

///
class PLASMA_EDITORFRAMEWORK_DLL plGizmoAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGizmoAction, plButtonAction);

public:
  plGizmoAction(const plActionContext& context, const char* szName, const plRTTI* pGizmoType);
  ~plGizmoAction();

  virtual void Execute(const plVariant& value) override;

protected:
  void UpdateState();
  void GameObjectEventHandler(const plGameObjectEvent& e);

  plGameObjectDocument* m_pGameObjectDocument = nullptr;
  const plRTTI* m_pGizmoType = nullptr;
};

///
class PLASMA_EDITORFRAMEWORK_DLL plToggleWorldSpaceGizmo : public plGizmoAction
{
public:
  plToggleWorldSpaceGizmo(const plActionContext& context, const char* szName, const plRTTI* pGizmoType);
  virtual void Execute(const plVariant& value) override;
};

///
class PLASMA_EDITORFRAMEWORK_DLL plTransformGizmoAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTransformGizmoAction, plButtonAction);

public:
  enum class ActionType
  {
    GizmoToggleWorldSpace,
    GizmoToggleMoveParentOnly,
    GizmoSnapSettings,
  };

  plTransformGizmoAction(const plActionContext& context, const char* szName, ActionType type);
  ~plTransformGizmoAction();

  virtual void Execute(const plVariant& value) override;
  void GameObjectEventHandler(const plGameObjectEvent& e);

private:
  void UpdateState();

  plGameObjectDocument* m_pGameObjectDocument;
  ActionType m_Type;
};

///
class PLASMA_EDITORFRAMEWORK_DLL plTranslateGizmoAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTranslateGizmoAction, plButtonAction);

public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(plStringView sMapping);

private:
  static plActionDescriptorHandle s_hSnappingValueMenu;
  static plActionDescriptorHandle s_hSnapPivotToGrid;
  static plActionDescriptorHandle s_hSnapObjectsToGrid;

public:
  enum class ActionType
  {
    SnapSelectionPivotToGrid,
    SnapEachSelectedObjectToGrid,
  };

  plTranslateGizmoAction(const plActionContext& context, const char* szName, ActionType type);

  virtual void Execute(const plVariant& value) override;

private:
  const plGameObjectDocument* m_pSceneDocument;
  ActionType m_Type;
};

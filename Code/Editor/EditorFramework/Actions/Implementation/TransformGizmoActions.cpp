#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Dialogs/SnapSettingsDlg.moc.h>
#include <EditorFramework/EditTools/StandardGizmoEditTools.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGizmoAction, 0, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plGizmoAction::plGizmoAction(const plActionContext& context, const char* szName, const plRTTI* pGizmoType)
  : plButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_pGizmoType = pGizmoType;
  m_pGameObjectDocument = static_cast<plGameObjectDocument*>(context.m_pDocument);
  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(plMakeDelegate(&plGizmoAction::GameObjectEventHandler, this));

  if (m_pGizmoType)
  {
    plStringBuilder sIcon(":/TypeIcons/", m_pGizmoType->GetTypeName(), ".svg");
    SetIconPath(sIcon);
  }
  else
  {
    SetIconPath(":/EditorFramework/Icons/GizmoNone.svg");
  }

  UpdateState();
}

plGizmoAction::~plGizmoAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(plMakeDelegate(&plGizmoAction::GameObjectEventHandler, this));
}

void plGizmoAction::Execute(const plVariant& value)
{
  m_pGameObjectDocument->SetActiveEditTool(m_pGizmoType);
  UpdateState();
}

void plGizmoAction::UpdateState()
{
  SetChecked(m_pGameObjectDocument->IsActiveEditTool(m_pGizmoType));
}

void plGizmoAction::GameObjectEventHandler(const plGameObjectEvent& e)
{
  if (e.m_Type == plGameObjectEvent::Type::ActiveEditToolChanged)
    UpdateState();
}

//////////////////////////////////////////////////////////////////////////

plToggleWorldSpaceGizmo::plToggleWorldSpaceGizmo(const plActionContext& context, const char* szName, const plRTTI* pGizmoType)
  : plGizmoAction(context, szName, pGizmoType)
{
}

void plToggleWorldSpaceGizmo::Execute(const plVariant& value)
{
  if (m_pGameObjectDocument->IsActiveEditTool(m_pGizmoType))
  {
    // toggle local/world space if the same tool is selected again
    m_pGameObjectDocument->SetGizmoWorldSpace(!m_pGameObjectDocument->GetGizmoWorldSpace());
  }
  else
  {
    plGizmoAction::Execute(value);
  }
}

//////////////////////////////////////////////////////////////////////////

plActionDescriptorHandle plTransformGizmoActions::s_hGizmoCategory;
plActionDescriptorHandle plTransformGizmoActions::s_hGizmoMenu;
plActionDescriptorHandle plTransformGizmoActions::s_hNoGizmo;
plActionDescriptorHandle plTransformGizmoActions::s_hTranslateGizmo;
plActionDescriptorHandle plTransformGizmoActions::s_hRotateGizmo;
plActionDescriptorHandle plTransformGizmoActions::s_hScaleGizmo;
plActionDescriptorHandle plTransformGizmoActions::s_hDragToPositionGizmo;
plActionDescriptorHandle plTransformGizmoActions::s_hWorldSpace;
plActionDescriptorHandle plTransformGizmoActions::s_hMoveParentOnly;
plActionDescriptorHandle plTransformGizmoActions::s_SnapSettings;

void plTransformGizmoActions::RegisterActions()
{
  s_hGizmoCategory = PLASMA_REGISTER_CATEGORY("GizmoCategory");
  s_hGizmoMenu = PLASMA_REGISTER_MENU("G.Gizmos");
  s_hNoGizmo = PLASMA_REGISTER_ACTION_1("Gizmo.Mode.Select", plActionScope::Document, "Gizmo", "Q", plGizmoAction, nullptr);
  s_hTranslateGizmo = PLASMA_REGISTER_ACTION_1(
    "Gizmo.Mode.Translate", plActionScope::Document, "Gizmo", "W", plToggleWorldSpaceGizmo, plGetStaticRTTI<plTranslateGizmoEditTool>());
  s_hRotateGizmo = PLASMA_REGISTER_ACTION_1(
    "Gizmo.Mode.Rotate", plActionScope::Document, "Gizmo", "E", plToggleWorldSpaceGizmo, plGetStaticRTTI<plRotateGizmoEditTool>());
  s_hScaleGizmo =
    PLASMA_REGISTER_ACTION_1("Gizmo.Mode.Scale", plActionScope::Document, "Gizmo", "R", plGizmoAction, plGetStaticRTTI<plScaleGizmoEditTool>());
  s_hDragToPositionGizmo = PLASMA_REGISTER_ACTION_1(
    "Gizmo.Mode.DragToPosition", plActionScope::Document, "Gizmo", "T", plGizmoAction, plGetStaticRTTI<plDragToPositionGizmoEditTool>());
  s_hWorldSpace = PLASMA_REGISTER_ACTION_1(
    "Gizmo.TransformSpace", plActionScope::Document, "Gizmo", "", plTransformGizmoAction, plTransformGizmoAction::ActionType::GizmoToggleWorldSpace);
  s_hMoveParentOnly = PLASMA_REGISTER_ACTION_1("Gizmo.MoveParentOnly", plActionScope::Document, "Gizmo", "", plTransformGizmoAction,
    plTransformGizmoAction::ActionType::GizmoToggleMoveParentOnly);
  s_SnapSettings = PLASMA_REGISTER_ACTION_1(
    "Gizmo.SnapSettings", plActionScope::Document, "Gizmo", "End", plTransformGizmoAction, plTransformGizmoAction::ActionType::GizmoSnapSettings);
}

void plTransformGizmoActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hGizmoCategory);
  plActionManager::UnregisterAction(s_hGizmoMenu);
  plActionManager::UnregisterAction(s_hNoGizmo);
  plActionManager::UnregisterAction(s_hTranslateGizmo);
  plActionManager::UnregisterAction(s_hRotateGizmo);
  plActionManager::UnregisterAction(s_hScaleGizmo);
  plActionManager::UnregisterAction(s_hDragToPositionGizmo);
  plActionManager::UnregisterAction(s_hWorldSpace);
  plActionManager::UnregisterAction(s_hMoveParentOnly);
  plActionManager::UnregisterAction(s_SnapSettings);
}

void plTransformGizmoActions::MapMenuActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  const plStringView sTarget = "G.Gizmos";

  pMap->MapAction(s_hGizmoMenu, "G.Edit", 4.0f);
  pMap->MapAction(s_hNoGizmo, sTarget, 0.0f);
  pMap->MapAction(s_hTranslateGizmo, sTarget, 1.0f);
  pMap->MapAction(s_hRotateGizmo, sTarget, 2.0f);
  pMap->MapAction(s_hScaleGizmo, sTarget, 3.0f);
  pMap->MapAction(s_hDragToPositionGizmo, sTarget, 4.0f);
  pMap->MapAction(s_hWorldSpace, sTarget, 6.0f);
  pMap->MapAction(s_hMoveParentOnly, sTarget, 7.0f);
  pMap->MapAction(s_SnapSettings, sTarget, 8.0f);
}

void plTransformGizmoActions::MapToolbarActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  const plStringView sSubPath = "GizmoCategory";

  pMap->MapAction(s_hGizmoCategory, "", 4.0f);
  pMap->MapAction(s_hNoGizmo, sSubPath, 0.0f);
  pMap->MapAction(s_hTranslateGizmo, sSubPath, 1.0f);
  pMap->MapAction(s_hRotateGizmo, sSubPath, 2.0f);
  pMap->MapAction(s_hScaleGizmo, sSubPath, 3.0f);
  pMap->MapAction(s_hDragToPositionGizmo, sSubPath, 4.0f);
  pMap->MapAction(s_hWorldSpace, sSubPath, 6.0f);
  pMap->MapAction(s_SnapSettings, sSubPath, 7.0f);
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTransformGizmoAction, 0, plRTTINoAllocator)
  ;
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plTransformGizmoAction::plTransformGizmoAction(const plActionContext& context, const char* szName, ActionType type)
  : plButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_Type = type;
  m_pGameObjectDocument = static_cast<plGameObjectDocument*>(context.m_pDocument);

  switch (m_Type)
  {
    case ActionType::GizmoToggleWorldSpace:
      SetIconPath(":/EditorFramework/Icons/WorldSpace.svg");
      break;
    case ActionType::GizmoToggleMoveParentOnly:
      SetIconPath(":/EditorFramework/Icons/TransformParent.svg");
      break;
    case ActionType::GizmoSnapSettings:
      SetCheckable(false);
      SetIconPath(":/EditorFramework/Icons/SnapSettings.svg");
      break;
  }

  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(plMakeDelegate(&plTransformGizmoAction::GameObjectEventHandler, this));
  UpdateState();
}

plTransformGizmoAction::~plTransformGizmoAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(plMakeDelegate(&plTransformGizmoAction::GameObjectEventHandler, this));
}

void plTransformGizmoAction::Execute(const plVariant& value)
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    m_pGameObjectDocument->SetGizmoWorldSpace(value.ConvertTo<bool>());
  }
  else if (m_Type == ActionType::GizmoToggleMoveParentOnly)
  {
    m_pGameObjectDocument->SetGizmoMoveParentOnly(value.ConvertTo<bool>());
  }
  else if (m_Type == ActionType::GizmoSnapSettings)
  {
    plQtSnapSettingsDlg dlg(nullptr);
    dlg.exec();
  }

  UpdateState();
}

void plTransformGizmoAction::GameObjectEventHandler(const plGameObjectEvent& e)
{
  if (e.m_Type == plGameObjectEvent::Type::ActiveEditToolChanged)
    UpdateState();
}

void plTransformGizmoAction::UpdateState()
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    plGameObjectEditTool* pTool = m_pGameObjectDocument->GetActiveEditTool();
    SetEnabled(pTool != nullptr && pTool->GetSupportedSpaces() == plEditToolSupportedSpaces::LocalAndWorldSpace);

    if (pTool != nullptr)
    {
      switch (pTool->GetSupportedSpaces())
      {
        case plEditToolSupportedSpaces::LocalSpaceOnly:
          SetChecked(false);
          break;
        case plEditToolSupportedSpaces::WorldSpaceOnly:
          SetChecked(true);
          break;
        case plEditToolSupportedSpaces::LocalAndWorldSpace:
          SetChecked(m_pGameObjectDocument->GetGizmoWorldSpace());
          break;
      }
    }
  }
  else if (m_Type == ActionType::GizmoToggleMoveParentOnly)
  {
    plGameObjectEditTool* pTool = m_pGameObjectDocument->GetActiveEditTool();
    const bool bSupported = pTool != nullptr && pTool->GetSupportsMoveParentOnly();

    SetEnabled(bSupported);
    SetChecked(bSupported && m_pGameObjectDocument->GetGizmoMoveParentOnly());
  }
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTranslateGizmoAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plActionDescriptorHandle plTranslateGizmoAction::s_hSnappingValueMenu;
plActionDescriptorHandle plTranslateGizmoAction::s_hSnapPivotToGrid;
plActionDescriptorHandle plTranslateGizmoAction::s_hSnapObjectsToGrid;

void plTranslateGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = PLASMA_REGISTER_CATEGORY("Gizmo.Translate.Snap.Menu");
  s_hSnapPivotToGrid = PLASMA_REGISTER_ACTION_1("Gizmo.Translate.Snap.PivotToGrid", plActionScope::Document, "Gizmo - Position Snap", "Ctrl+End", plTranslateGizmoAction, plTranslateGizmoAction::ActionType::SnapSelectionPivotToGrid);
  s_hSnapObjectsToGrid = PLASMA_REGISTER_ACTION_1("Gizmo.Translate.Snap.ObjectsToGrid", plActionScope::Document, "Gizmo - Position Snap", "", plTranslateGizmoAction, plTranslateGizmoAction::ActionType::SnapEachSelectedObjectToGrid);
}

void plTranslateGizmoAction::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hSnappingValueMenu);
  plActionManager::UnregisterAction(s_hSnapPivotToGrid);
  plActionManager::UnregisterAction(s_hSnapObjectsToGrid);
}

void plTranslateGizmoAction::MapActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSnappingValueMenu, "G.Gizmos", 8.0f);

  pMap->MapAction(s_hSnapPivotToGrid, "G.Gizmos", "Gizmo.Translate.Snap.Menu", 0.0f);
  pMap->MapAction(s_hSnapObjectsToGrid, "G.Gizmos", "Gizmo.Translate.Snap.Menu", 1.0f);
}

plTranslateGizmoAction::plTranslateGizmoAction(const plActionContext& context, const char* szName, ActionType type)
  : plButtonAction(context, szName, false, "")
{
  m_pSceneDocument = static_cast<const plGameObjectDocument*>(context.m_pDocument);
  m_Type = type;
}

void plTranslateGizmoAction::Execute(const plVariant& value)
{
  if (m_Type == ActionType::SnapSelectionPivotToGrid)
    m_pSceneDocument->TriggerSnapPivotToGrid();

  if (m_Type == ActionType::SnapEachSelectedObjectToGrid)
    m_pSceneDocument->TriggerSnapEachObjectToGrid();
}

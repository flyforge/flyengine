#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Document/GameObjectDocument.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameObjectSelectionAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plActionDescriptorHandle plGameObjectSelectionActions::s_hSelectionCategory;
plActionDescriptorHandle plGameObjectSelectionActions::s_hShowInScenegraph;
plActionDescriptorHandle plGameObjectSelectionActions::s_hFocusOnSelection;
plActionDescriptorHandle plGameObjectSelectionActions::s_hFocusOnSelectionAllViews;
plActionDescriptorHandle plGameObjectSelectionActions::s_hSnapCameraToObject;
plActionDescriptorHandle plGameObjectSelectionActions::s_hMoveCameraHere;
plActionDescriptorHandle plGameObjectSelectionActions::s_hCreateEmptyGameObjectHere;

void plGameObjectSelectionActions::RegisterActions()
{
  s_hSelectionCategory = PLASMA_REGISTER_CATEGORY("SelectionCategory");
  s_hShowInScenegraph = PLASMA_REGISTER_ACTION_1("Selection.ShowInScenegraph", plActionScope::Document, "Scene - Selection", "Ctrl+T",
    plGameObjectSelectionAction, plGameObjectSelectionAction::ActionType::ShowInScenegraph);
  s_hFocusOnSelection = PLASMA_REGISTER_ACTION_1("Selection.FocusSingleView", plActionScope::Document, "Scene - Selection", "F",
    plGameObjectSelectionAction, plGameObjectSelectionAction::ActionType::FocusOnSelection);
  s_hFocusOnSelectionAllViews = PLASMA_REGISTER_ACTION_1("Selection.FocusAllViews", plActionScope::Document, "Scene - Selection", "Shift+F",
    plGameObjectSelectionAction, plGameObjectSelectionAction::ActionType::FocusOnSelectionAllViews);
  s_hSnapCameraToObject = PLASMA_REGISTER_ACTION_1("Scene.Camera.SnapCameraToObject", plActionScope::Document, "Camera", "", plGameObjectSelectionAction,
    plGameObjectSelectionAction::ActionType::SnapCameraToObject);
  s_hMoveCameraHere = PLASMA_REGISTER_ACTION_1("Scene.Camera.MoveCameraHere", plActionScope::Document, "Camera", "C", plGameObjectSelectionAction,
    plGameObjectSelectionAction::ActionType::MoveCameraHere);

  s_hCreateEmptyGameObjectHere = PLASMA_REGISTER_ACTION_1("Scene.GameObject.CreateEmptyHere", plActionScope::Document, "Scene", "",
    plGameObjectSelectionAction, plGameObjectSelectionAction::ActionType::CreateGameObjectHere);
}

void plGameObjectSelectionActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hSelectionCategory);
  plActionManager::UnregisterAction(s_hShowInScenegraph);
  plActionManager::UnregisterAction(s_hFocusOnSelection);
  plActionManager::UnregisterAction(s_hFocusOnSelectionAllViews);
  plActionManager::UnregisterAction(s_hSnapCameraToObject);
  plActionManager::UnregisterAction(s_hMoveCameraHere);
  plActionManager::UnregisterAction(s_hCreateEmptyGameObjectHere);
}

void plGameObjectSelectionActions::MapActions(const char* szMapping, const char* szPath)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  plStringBuilder sSubPath(szPath, "/SelectionCategory");

  pMap->MapAction(s_hSelectionCategory, szPath, 5.0f);

  pMap->MapAction(s_hShowInScenegraph, sSubPath, 2.0f);
  pMap->MapAction(s_hFocusOnSelection, sSubPath, 3.0f);
  pMap->MapAction(s_hFocusOnSelectionAllViews, sSubPath, 3.5f);
  pMap->MapAction(s_hSnapCameraToObject, sSubPath, 8.0f);
  pMap->MapAction(s_hMoveCameraHere, sSubPath, 10.0f);
}

void plGameObjectSelectionActions::MapContextMenuActions(const char* szMapping, const char* szPath)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  plStringBuilder sSubPath(szPath, "/SelectionCategory");

  pMap->MapAction(s_hSelectionCategory, szPath, 5.0f);
  pMap->MapAction(s_hFocusOnSelectionAllViews, sSubPath, 1.0f);
}


void plGameObjectSelectionActions::MapViewContextMenuActions(const char* szMapping, const char* szPath)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  plStringBuilder sSubPath(szPath, "/SelectionCategory");

  pMap->MapAction(s_hSelectionCategory, szPath, 5.0f);

  pMap->MapAction(s_hFocusOnSelectionAllViews, sSubPath, 1.0f);
  pMap->MapAction(s_hSnapCameraToObject, sSubPath, 4.0f);
  pMap->MapAction(s_hMoveCameraHere, sSubPath, 6.0f);
  pMap->MapAction(s_hCreateEmptyGameObjectHere, sSubPath, 1.0f);
}

plGameObjectSelectionAction::plGameObjectSelectionAction(
  const plActionContext& context, const char* szName, plGameObjectSelectionAction::ActionType type)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pSceneDocument = const_cast<plGameObjectDocument*>(static_cast<const plGameObjectDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::ShowInScenegraph:
      SetIconPath(":/EditorFramework/Icons/Scenegraph.svg");
      break;
    case ActionType::FocusOnSelection:
      SetIconPath(":/EditorFramework/Icons/FocusOnSelection.svg");
      break;
    case ActionType::FocusOnSelectionAllViews:
      SetIconPath(":/EditorFramework/Icons/FocusOnSelectionAllViews.svg");
      break;
    case ActionType::SnapCameraToObject:
      // SetIconPath(":/EditorFramework/Icons/Duplicate.svg"); // TODO Icon
      break;
    case ActionType::MoveCameraHere:
      // SetIconPath(":/EditorFramework/Icons/Duplicate.svg"); // TODO Icon
      break;
    case ActionType::CreateGameObjectHere:
      SetIconPath(":/EditorFramework/Icons/CreateEmpty.svg");
      break;
  }

  UpdateEnableState();

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plGameObjectSelectionAction::SelectionEventHandler, this));
}


plGameObjectSelectionAction::~plGameObjectSelectionAction()
{
  m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(
    plMakeDelegate(&plGameObjectSelectionAction::SelectionEventHandler, this));
}

void plGameObjectSelectionAction::Execute(const plVariant& value)
{
  switch (m_Type)
  {
    case ActionType::ShowInScenegraph:
      m_pSceneDocument->TriggerShowSelectionInScenegraph();
      return;
    case ActionType::FocusOnSelection:
      m_pSceneDocument->TriggerFocusOnSelection(false);
      return;
    case ActionType::FocusOnSelectionAllViews:
      m_pSceneDocument->TriggerFocusOnSelection(true);
      return;
    case ActionType::SnapCameraToObject:
      m_pSceneDocument->SnapCameraToObject();
      break;
    case ActionType::MoveCameraHere:
      m_pSceneDocument->MoveCameraHere();
      break;
    case ActionType::CreateGameObjectHere:
    {
      auto res = m_pSceneDocument->CreateGameObjectHere();
      plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Create empty object at picked position failed.");
    }
    break;
  }
}

void plGameObjectSelectionAction::SelectionEventHandler(const plSelectionManagerEvent& e)
{
  UpdateEnableState();
}

void plGameObjectSelectionAction::UpdateEnableState()
{
  if (m_Type == ActionType::FocusOnSelection || m_Type == ActionType::FocusOnSelectionAllViews || m_Type == ActionType::ShowInScenegraph)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }

  if (m_Type == ActionType::SnapCameraToObject)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() == 1);
  }
}

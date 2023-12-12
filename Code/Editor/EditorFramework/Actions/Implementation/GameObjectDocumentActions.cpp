#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/Preferences/ScenePreferences.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameObjectDocumentAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCameraSpeedSliderAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plActionDescriptorHandle plGameObjectDocumentActions::s_hGameObjectCategory;
plActionDescriptorHandle plGameObjectDocumentActions::s_hRenderSelectionOverlay;
plActionDescriptorHandle plGameObjectDocumentActions::s_hRenderVisualizers;
plActionDescriptorHandle plGameObjectDocumentActions::s_hRenderShapeIcons;
plActionDescriptorHandle plGameObjectDocumentActions::s_hRenderGrid;
plActionDescriptorHandle plGameObjectDocumentActions::s_hAddAmbientLight;
plActionDescriptorHandle plGameObjectDocumentActions::s_hSimulationSpeedMenu;
plActionDescriptorHandle plGameObjectDocumentActions::s_hSimulationSpeed[10];
plActionDescriptorHandle plGameObjectDocumentActions::s_hCameraSpeed;
plActionDescriptorHandle plGameObjectDocumentActions::s_hPickTransparent;

void plGameObjectDocumentActions::RegisterActions()
{
  s_hGameObjectCategory = PLASMA_REGISTER_CATEGORY("GameObjectCategory");
  s_hRenderSelectionOverlay = PLASMA_REGISTER_ACTION_1("Scene.Render.SelectionOverlay", plActionScope::Document, "Scene", "S", plGameObjectDocumentAction,
    plGameObjectDocumentAction::ActionType::RenderSelectionOverlay);
  s_hRenderVisualizers = PLASMA_REGISTER_ACTION_1("Scene.Render.Visualizers", plActionScope::Document, "Scene", "V", plGameObjectDocumentAction,
    plGameObjectDocumentAction::ActionType::RenderVisualizers);
  s_hRenderShapeIcons = PLASMA_REGISTER_ACTION_1("Scene.Render.ShapeIcons", plActionScope::Document, "Scene", "I", plGameObjectDocumentAction,
    plGameObjectDocumentAction::ActionType::RenderShapeIcons);
  s_hRenderGrid = PLASMA_REGISTER_ACTION_1(
    "Scene.Render.Grid", plActionScope::Document, "Scene", "G", plGameObjectDocumentAction, plGameObjectDocumentAction::ActionType::RenderGrid);
  s_hAddAmbientLight = PLASMA_REGISTER_ACTION_1("Scene.Render.AddAmbient", plActionScope::Document, "Scene", "", plGameObjectDocumentAction,
    plGameObjectDocumentAction::ActionType::AddAmbientLight);

  s_hSimulationSpeedMenu = PLASMA_REGISTER_MENU_WITH_ICON("Scene.Simulation.Speed.Menu", "");
  s_hSimulationSpeed[0] = PLASMA_REGISTER_ACTION_2("Scene.Simulation.Speed.01", plActionScope::Document, "Simulation - Speed", "",
    plGameObjectDocumentAction, plGameObjectDocumentAction::ActionType::SimulationSpeed, 0.1f);
  s_hSimulationSpeed[1] = PLASMA_REGISTER_ACTION_2("Scene.Simulation.Speed.025", plActionScope::Document, "Simulation - Speed", "",
    plGameObjectDocumentAction, plGameObjectDocumentAction::ActionType::SimulationSpeed, 0.25f);
  s_hSimulationSpeed[2] = PLASMA_REGISTER_ACTION_2("Scene.Simulation.Speed.05", plActionScope::Document, "Simulation - Speed", "",
    plGameObjectDocumentAction, plGameObjectDocumentAction::ActionType::SimulationSpeed, 0.5f);
  s_hSimulationSpeed[3] = PLASMA_REGISTER_ACTION_2("Scene.Simulation.Speed.1", plActionScope::Document, "Simulation - Speed", "",
    plGameObjectDocumentAction, plGameObjectDocumentAction::ActionType::SimulationSpeed, 1.0f);
  s_hSimulationSpeed[4] = PLASMA_REGISTER_ACTION_2("Scene.Simulation.Speed.15", plActionScope::Document, "Simulation - Speed", "",
    plGameObjectDocumentAction, plGameObjectDocumentAction::ActionType::SimulationSpeed, 1.5f);
  s_hSimulationSpeed[5] = PLASMA_REGISTER_ACTION_2("Scene.Simulation.Speed.2", plActionScope::Document, "Simulation - Speed", "",
    plGameObjectDocumentAction, plGameObjectDocumentAction::ActionType::SimulationSpeed, 2.0f);
  s_hSimulationSpeed[6] = PLASMA_REGISTER_ACTION_2("Scene.Simulation.Speed.3", plActionScope::Document, "Simulation - Speed", "",
    plGameObjectDocumentAction, plGameObjectDocumentAction::ActionType::SimulationSpeed, 3.0f);
  s_hSimulationSpeed[7] = PLASMA_REGISTER_ACTION_2("Scene.Simulation.Speed.4", plActionScope::Document, "Simulation - Speed", "",
    plGameObjectDocumentAction, plGameObjectDocumentAction::ActionType::SimulationSpeed, 4.0f);
  s_hSimulationSpeed[8] = PLASMA_REGISTER_ACTION_2("Scene.Simulation.Speed.5", plActionScope::Document, "Simulation - Speed", "",
    plGameObjectDocumentAction, plGameObjectDocumentAction::ActionType::SimulationSpeed, 5.0f);
  s_hSimulationSpeed[9] = PLASMA_REGISTER_ACTION_2("Scene.Simulation.Speed.10", plActionScope::Document, "Simulation - Speed", "",
    plGameObjectDocumentAction, plGameObjectDocumentAction::ActionType::SimulationSpeed, 10.0f);

  s_hCameraSpeed = PLASMA_REGISTER_ACTION_1(
    "Scene.Camera.Speed", plActionScope::Document, "Camera", "", plCameraSpeedSliderAction, plCameraSpeedSliderAction::ActionType::CameraSpeed);

  s_hPickTransparent = PLASMA_REGISTER_ACTION_1("Scene.Render.PickTransparent", plActionScope::Document, "Scene", "U", plGameObjectDocumentAction,
    plGameObjectDocumentAction::ActionType::PickTransparent);
}

void plGameObjectDocumentActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hGameObjectCategory);
  plActionManager::UnregisterAction(s_hRenderSelectionOverlay);
  plActionManager::UnregisterAction(s_hRenderVisualizers);
  plActionManager::UnregisterAction(s_hRenderShapeIcons);
  plActionManager::UnregisterAction(s_hRenderGrid);
  plActionManager::UnregisterAction(s_hAddAmbientLight);

  plActionManager::UnregisterAction(s_hSimulationSpeedMenu);
  for (int i = 0; i < PLASMA_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    plActionManager::UnregisterAction(s_hSimulationSpeed[i]);

  plActionManager::UnregisterAction(s_hCameraSpeed);
  plActionManager::UnregisterAction(s_hPickTransparent);
}

void plGameObjectDocumentActions::MapMenuActions(const char* szMapping, const char* szPath)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    pMap->MapAction(s_hGameObjectCategory, szPath, 0.9f);

    plStringBuilder sSubPath(szPath, "/GameObjectCategory");
    pMap->MapAction(s_hRenderSelectionOverlay, sSubPath, 1.0f);
    pMap->MapAction(s_hRenderVisualizers, sSubPath, 2.0f);
    pMap->MapAction(s_hRenderShapeIcons, sSubPath, 3.0f);
    pMap->MapAction(s_hRenderGrid, sSubPath, 4.0f);
    pMap->MapAction(s_hPickTransparent, sSubPath, 5.0f);
    pMap->MapAction(s_hAddAmbientLight, sSubPath, 6.0f);
    pMap->MapAction(s_hCameraSpeed, sSubPath, 7.0f);
  }
}

void plGameObjectDocumentActions::MapMenuSimulationSpeed(const char* szMapping, const char* szPath)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    plStringBuilder sSubPath(szPath, "/GameObjectCategory");

    pMap->MapAction(s_hGameObjectCategory, szPath, 1.0f);
    pMap->MapAction(s_hSimulationSpeedMenu, sSubPath, 3.0f);

    plStringBuilder sSubPathSim(sSubPath, "/Scene.Simulation.Speed.Menu");
    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(s_hSimulationSpeed); ++i)
      pMap->MapAction(s_hSimulationSpeed[i], sSubPathSim, i + 1.0f);
  }
}

void plGameObjectDocumentActions::MapToolbarActions(const char* szMapping, const char* szPath)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    pMap->MapAction(s_hGameObjectCategory, szPath, 12.0f);

    plStringBuilder sSubPath(szPath, "/GameObjectCategory");
    pMap->MapAction(s_hRenderSelectionOverlay, sSubPath, 4.0f);
    pMap->MapAction(s_hRenderVisualizers, sSubPath, 5.0f);
    pMap->MapAction(s_hRenderShapeIcons, sSubPath, 6.0f);
    pMap->MapAction(s_hRenderGrid, sSubPath, 6.5f);
    pMap->MapAction(s_hCameraSpeed, sSubPath, 7.0f);
  }
}

plGameObjectDocumentAction::plGameObjectDocumentAction(
  const plActionContext& context, const char* szName, plGameObjectDocumentAction::ActionType type, float fSimSpeed)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pGameObjectDocument = const_cast<plGameObjectDocument*>(static_cast<const plGameObjectDocument*>(context.m_pDocument));
  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(plMakeDelegate(&plGameObjectDocumentAction::SceneEventHandler, this));
  m_fSimSpeed = fSimSpeed;

  switch (m_Type)
  {
    case ActionType::RenderSelectionOverlay:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Selection.svg");
      SetChecked(m_pGameObjectDocument->GetRenderSelectionOverlay());
      break;

    case ActionType::RenderVisualizers:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Visualizers.svg");
      SetChecked(m_pGameObjectDocument->GetRenderVisualizers());
      break;

    case ActionType::RenderShapeIcons:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/ShapeIcons.svg");
      SetChecked(m_pGameObjectDocument->GetRenderShapeIcons());
      break;

    case ActionType::RenderGrid:
    {
      plScenePreferencesUser* pPreferences = plPreferences::QueryPreferences<plScenePreferencesUser>(m_pGameObjectDocument);
      pPreferences->m_ChangedEvent.AddEventHandler(plMakeDelegate(&plGameObjectDocumentAction::OnPreferenceChange, this));

      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Grid.svg");
      SetChecked(pPreferences->GetShowGrid());
    }
    break;

    case ActionType::AddAmbientLight:
      SetCheckable(true);
      // SetIconPath(":/EditorPluginScene/Icons/ShapeIcons.svg"); // TODO icon
      SetChecked(m_pGameObjectDocument->GetAddAmbientLight());
      break;

    case ActionType::SimulationSpeed:
      SetCheckable(true);
      SetChecked(m_pGameObjectDocument->GetSimulationSpeed() == m_fSimSpeed);
      break;

    case ActionType::PickTransparent:
      SetCheckable(true);
      // SetIconPath(":/EditorFramework/Icons/Visualizers.svg"); // TODO icon
      SetChecked(m_pGameObjectDocument->GetPickTransparent());
      break;
  }
}

plGameObjectDocumentAction::~plGameObjectDocumentAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(plMakeDelegate(&plGameObjectDocumentAction::SceneEventHandler, this));

  switch (m_Type)
  {
    case ActionType::RenderGrid:
    {
      plScenePreferencesUser* pPreferences = plPreferences::QueryPreferences<plScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->m_ChangedEvent.RemoveEventHandler(plMakeDelegate(&plGameObjectDocumentAction::OnPreferenceChange, this));
    }
    break;
    default:
      break;
  }
}

void plGameObjectDocumentAction::Execute(const plVariant& value)
{
  switch (m_Type)
  {
    case ActionType::RenderSelectionOverlay:
      m_pGameObjectDocument->SetRenderSelectionOverlay(!m_pGameObjectDocument->GetRenderSelectionOverlay());
      return;

    case ActionType::RenderVisualizers:
      m_pGameObjectDocument->SetRenderVisualizers(!m_pGameObjectDocument->GetRenderVisualizers());
      return;

    case ActionType::RenderShapeIcons:
      m_pGameObjectDocument->SetRenderShapeIcons(!m_pGameObjectDocument->GetRenderShapeIcons());
      return;

    case ActionType::RenderGrid:
    {
      auto pPref = plPreferences::QueryPreferences<plScenePreferencesUser>(m_pGameObjectDocument);
      pPref->SetShowGrid(!pPref->GetShowGrid());
      m_pGameObjectDocument->ShowDocumentStatus(plFmt("Show Grid: {}", pPref->GetShowGrid() ? "ON" : "OFF"));
      return;
    }

    case ActionType::AddAmbientLight:
      m_pGameObjectDocument->SetAddAmbientLight(!m_pGameObjectDocument->GetAddAmbientLight());
      return;

    case ActionType::SimulationSpeed:
      m_pGameObjectDocument->SetSimulationSpeed(m_fSimSpeed);
      return;

    case ActionType::PickTransparent:
      m_pGameObjectDocument->SetPickTransparent(!m_pGameObjectDocument->GetPickTransparent());
      return;

    default:
      break;
  }
}

void plGameObjectDocumentAction::SceneEventHandler(const plGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case plGameObjectEvent::Type::RenderSelectionOverlayChanged:
    {
      if (m_Type == ActionType::RenderSelectionOverlay)
      {
        SetChecked(m_pGameObjectDocument->GetRenderSelectionOverlay());
      }
    }
    break;

    case plGameObjectEvent::Type::RenderVisualizersChanged:
    {
      if (m_Type == ActionType::RenderVisualizers)
      {
        SetChecked(m_pGameObjectDocument->GetRenderVisualizers());
      }
    }
    break;

    case plGameObjectEvent::Type::RenderShapeIconsChanged:
    {
      if (m_Type == ActionType::RenderShapeIcons)
      {
        SetChecked(m_pGameObjectDocument->GetRenderShapeIcons());
      }
    }
    break;

    case plGameObjectEvent::Type::AddAmbientLightChanged:
    {
      if (m_Type == ActionType::AddAmbientLight)
      {
        SetChecked(m_pGameObjectDocument->GetAddAmbientLight());
      }
    }
    break;

    case plGameObjectEvent::Type::SimulationSpeedChanged:
    {
      if (m_Type == ActionType::SimulationSpeed)
      {
        SetChecked(m_pGameObjectDocument->GetSimulationSpeed() == m_fSimSpeed);
      }
    }
    break;

    case plGameObjectEvent::Type::PickTransparentChanged:
    {
      if (m_Type == ActionType::PickTransparent)
      {
        SetChecked(m_pGameObjectDocument->GetPickTransparent());
      }
    }
    break;

    default:
      break;
  }
}

void plGameObjectDocumentAction::OnPreferenceChange(plPreferences* pref)
{
  plScenePreferencesUser* pPreferences = plPreferences::QueryPreferences<plScenePreferencesUser>(m_pGameObjectDocument);

  switch (m_Type)
  {
    case ActionType::RenderGrid:
    {
      SetChecked(pPreferences->GetShowGrid());
    }
    break;

    default:
      break;
  }
}

plCameraSpeedSliderAction::plCameraSpeedSliderAction(const plActionContext& context, const char* szName, ActionType type)
  : plSliderAction(context, szName)
{
  m_Type = type;
  m_pGameObjectDocument = const_cast<plGameObjectDocument*>(static_cast<const plGameObjectDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      plScenePreferencesUser* pPreferences = plPreferences::QueryPreferences<plScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->m_ChangedEvent.AddEventHandler(plMakeDelegate(&plCameraSpeedSliderAction::OnPreferenceChange, this));

      SetRange(0, 24);
    }
    break;
  }

  UpdateState();
}

plCameraSpeedSliderAction::~plCameraSpeedSliderAction()
{
  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      plScenePreferencesUser* pPreferences = plPreferences::QueryPreferences<plScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->m_ChangedEvent.RemoveEventHandler(plMakeDelegate(&plCameraSpeedSliderAction::OnPreferenceChange, this));
    }
    break;
  }
}

void plCameraSpeedSliderAction::Execute(const plVariant& value)
{
  const plInt32 iValue = value.Get<plInt32>();

  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      plScenePreferencesUser* pPreferences = plPreferences::QueryPreferences<plScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->SetCameraSpeed(value.Get<plInt32>());
    }
    break;
  }
}

void plCameraSpeedSliderAction::OnPreferenceChange(plPreferences* pref)
{
  UpdateState();
}

void plCameraSpeedSliderAction::UpdateState()
{
  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      plScenePreferencesUser* pPreferences = plPreferences::QueryPreferences<plScenePreferencesUser>(m_pGameObjectDocument);

      SetValue(pPreferences->GetCameraSpeed());
    }
    break;
  }
}

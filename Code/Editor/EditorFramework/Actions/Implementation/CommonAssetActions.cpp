#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/CommonAssetActions.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCommonAssetAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plActionDescriptorHandle plCommonAssetActions::s_hCategory;
plActionDescriptorHandle plCommonAssetActions::s_hPause;
plActionDescriptorHandle plCommonAssetActions::s_hRestart;
plActionDescriptorHandle plCommonAssetActions::s_hLoop;
plActionDescriptorHandle plCommonAssetActions::s_hSimulationSpeedMenu;
plActionDescriptorHandle plCommonAssetActions::s_hSimulationSpeed[10];
plActionDescriptorHandle plCommonAssetActions::s_hGrid;
plActionDescriptorHandle plCommonAssetActions::s_hVisualizers;


void plCommonAssetActions::RegisterActions()
{
  s_hCategory = PLASMA_REGISTER_CATEGORY("CommonAssetCategory");
  s_hPause = PLASMA_REGISTER_ACTION_1("Common.Pause", plActionScope::Document, "Animations", "Pause", plCommonAssetAction, plCommonAssetAction::ActionType::Pause);
  s_hRestart = PLASMA_REGISTER_ACTION_1("Common.Restart", plActionScope::Document, "Animations", "F5", plCommonAssetAction, plCommonAssetAction::ActionType::Restart);
  s_hLoop = PLASMA_REGISTER_ACTION_1("Common.Loop", plActionScope::Document, "Animations", "", plCommonAssetAction, plCommonAssetAction::ActionType::Loop);
  s_hGrid = PLASMA_REGISTER_ACTION_1("Common.Grid", plActionScope::Document, "Misc", "G", plCommonAssetAction, plCommonAssetAction::ActionType::Grid);
  s_hVisualizers = PLASMA_REGISTER_ACTION_1("Common.Visualizers", plActionScope::Document, "Misc", "V", plCommonAssetAction, plCommonAssetAction::ActionType::Visualizers);

  s_hSimulationSpeedMenu = PLASMA_REGISTER_MENU_WITH_ICON("Common.Speed.Menu", ":/EditorPluginParticle/Icons/Speed.svg");
  s_hSimulationSpeed[0] = PLASMA_REGISTER_ACTION_2("Common.Speed.01", plActionScope::Document, "Animations", "Ctrl+1", plCommonAssetAction, plCommonAssetAction::ActionType::SimulationSpeed, 0.1f);
  s_hSimulationSpeed[1] = PLASMA_REGISTER_ACTION_2("Common.Speed.025", plActionScope::Document, "Animations", "Ctrl+2", plCommonAssetAction, plCommonAssetAction::ActionType::SimulationSpeed, 0.25f);
  s_hSimulationSpeed[2] = PLASMA_REGISTER_ACTION_2("Common.Speed.05", plActionScope::Document, "Animations", "Ctrl+3", plCommonAssetAction, plCommonAssetAction::ActionType::SimulationSpeed, 0.5f);
  s_hSimulationSpeed[3] = PLASMA_REGISTER_ACTION_2("Common.Speed.1", plActionScope::Document, "Animations", "Ctrl+4", plCommonAssetAction, plCommonAssetAction::ActionType::SimulationSpeed, 1.0f);
  s_hSimulationSpeed[4] = PLASMA_REGISTER_ACTION_2("Common.Speed.15", plActionScope::Document, "Animations", "Ctrl+5", plCommonAssetAction, plCommonAssetAction::ActionType::SimulationSpeed, 1.5f);
  s_hSimulationSpeed[5] = PLASMA_REGISTER_ACTION_2("Common.Speed.2", plActionScope::Document, "Animations", "Ctrl+6", plCommonAssetAction, plCommonAssetAction::ActionType::SimulationSpeed, 2.0f);
  s_hSimulationSpeed[6] = PLASMA_REGISTER_ACTION_2("Common.Speed.3", plActionScope::Document, "Animations", "Ctrl+7", plCommonAssetAction, plCommonAssetAction::ActionType::SimulationSpeed, 3.0f);
  s_hSimulationSpeed[7] = PLASMA_REGISTER_ACTION_2("Common.Speed.4", plActionScope::Document, "Animations", "Ctrl+8", plCommonAssetAction, plCommonAssetAction::ActionType::SimulationSpeed, 4.0f);
  s_hSimulationSpeed[8] = PLASMA_REGISTER_ACTION_2("Common.Speed.5", plActionScope::Document, "Animations", "Ctrl+9", plCommonAssetAction, plCommonAssetAction::ActionType::SimulationSpeed, 5.0f);
  s_hSimulationSpeed[9] = PLASMA_REGISTER_ACTION_2("Common.Speed.10", plActionScope::Document, "Animations", "Ctrl+0", plCommonAssetAction, plCommonAssetAction::ActionType::SimulationSpeed, 10.0f);
}

void plCommonAssetActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCategory);
  plActionManager::UnregisterAction(s_hPause);
  plActionManager::UnregisterAction(s_hRestart);
  plActionManager::UnregisterAction(s_hLoop);
  plActionManager::UnregisterAction(s_hSimulationSpeedMenu);
  plActionManager::UnregisterAction(s_hGrid);
  plActionManager::UnregisterAction(s_hVisualizers);

  for (int i = 0; i < PLASMA_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    plActionManager::UnregisterAction(s_hSimulationSpeed[i]);
}

void plCommonAssetActions::MapToolbarActions(plStringView sMapping, plUInt32 uiStateMask)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "CommonAssetCategory";

  if (uiStateMask & plCommonAssetUiState::Pause)
  {
    pMap->MapAction(s_hPause, szSubPath, 0.5f);
  }

  if (uiStateMask & plCommonAssetUiState::Restart)
  {
    pMap->MapAction(s_hRestart, szSubPath, 1.0f);
  }

  if (uiStateMask & plCommonAssetUiState::Loop)
  {
    pMap->MapAction(s_hLoop, szSubPath, 2.0f);
  }

  if (uiStateMask & plCommonAssetUiState::SimulationSpeed)
  {
    pMap->MapAction(s_hSimulationSpeedMenu, szSubPath, 3.0f);

    plStringBuilder sSubPath(szSubPath, "/Common.Speed.Menu");

    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    {
      pMap->MapAction(s_hSimulationSpeed[i], sSubPath, i + 1.0f);
    }
  }

  if (uiStateMask & plCommonAssetUiState::Grid)
  {
    pMap->MapAction(s_hGrid, szSubPath, 4.0f);
  }

  if (uiStateMask & plCommonAssetUiState::Visualizers)
  {
    pMap->MapAction(s_hVisualizers, szSubPath, 5.0f);
  }
}

plCommonAssetAction::plCommonAssetAction(const plActionContext& context, const char* szName, plCommonAssetAction::ActionType type, float fSimSpeed)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_fSimSpeed = fSimSpeed;

  m_pAssetDocument = const_cast<plAssetDocument*>(static_cast<const plAssetDocument*>(context.m_pDocument));
  m_pAssetDocument->m_CommonAssetUiChangeEvent.AddEventHandler(plMakeDelegate(&plCommonAssetAction::CommonUiEventHandler, this));

  switch (m_Type)
  {
    case ActionType::Pause:
      SetCheckable(true);
      SetIconPath(":/EditorPluginParticle/Icons/Pause.svg");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(plCommonAssetUiState::Pause) != 0.0f);
      break;

    case ActionType::Restart:
      SetIconPath(":/EditorPluginParticle/Icons/Restart.svg");
      break;

    case ActionType::Loop:
      SetCheckable(true);
      SetIconPath(":/EditorPluginParticle/Icons/Loop.svg");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(plCommonAssetUiState::Loop) != 0.0f);
      break;

    case ActionType::Grid:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Grid.svg");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(plCommonAssetUiState::Grid) != 0.0f);
      break;

    case ActionType::Visualizers:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Visualizers.svg");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(plCommonAssetUiState::Visualizers) != 0.0f);
      break;

    default:
      break;
  }

  UpdateState();
}


plCommonAssetAction::~plCommonAssetAction()
{
  m_pAssetDocument->m_CommonAssetUiChangeEvent.RemoveEventHandler(plMakeDelegate(&plCommonAssetAction::CommonUiEventHandler, this));
}

void plCommonAssetAction::Execute(const plVariant& value)
{
  switch (m_Type)
  {
    case ActionType::Pause:
      m_pAssetDocument->SetCommonAssetUiState(plCommonAssetUiState::Pause, m_pAssetDocument->GetCommonAssetUiState(plCommonAssetUiState::Pause) == 0.0f ? 1.0f : 0.0f);
      return;

    case ActionType::Restart:
      m_pAssetDocument->SetCommonAssetUiState(plCommonAssetUiState::Restart, 1.0f);
      return;

    case ActionType::Loop:
      m_pAssetDocument->SetCommonAssetUiState(plCommonAssetUiState::Loop, m_pAssetDocument->GetCommonAssetUiState(plCommonAssetUiState::Loop) == 0.0f ? 1.0f : 0.0f);
      return;

    case ActionType::SimulationSpeed:
      m_pAssetDocument->SetCommonAssetUiState(plCommonAssetUiState::SimulationSpeed, m_fSimSpeed);
      return;

    case ActionType::Grid:
      m_pAssetDocument->SetCommonAssetUiState(plCommonAssetUiState::Grid, m_pAssetDocument->GetCommonAssetUiState(plCommonAssetUiState::Grid) == 0.0f ? 1.0f : 0.0f);
      return;

    case ActionType::Visualizers:
      m_pAssetDocument->SetCommonAssetUiState(plCommonAssetUiState::Visualizers, m_pAssetDocument->GetCommonAssetUiState(plCommonAssetUiState::Visualizers) == 0.0f ? 1.0f : 0.0f);
      return;
  }
}

void plCommonAssetAction::CommonUiEventHandler(const plCommonAssetUiState& e)
{
  if (e.m_State == plCommonAssetUiState::Loop || e.m_State == plCommonAssetUiState::SimulationSpeed || e.m_State == plCommonAssetUiState::Grid || e.m_State == plCommonAssetUiState::Visualizers)
  {
    UpdateState();
  }
}

void plCommonAssetAction::UpdateState()
{
  if (m_Type == ActionType::Pause)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(plCommonAssetUiState::Pause) != 0.0f);
  }

  if (m_Type == ActionType::Loop)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(plCommonAssetUiState::Loop) != 0.0f);
  }

  if (m_Type == ActionType::SimulationSpeed)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(plCommonAssetUiState::SimulationSpeed) == m_fSimSpeed);
  }

  if (m_Type == ActionType::Grid)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(plCommonAssetUiState::Grid) != 0.0f);
  }

  if (m_Type == ActionType::Visualizers)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(plCommonAssetUiState::Visualizers) != 0.0f);
  }
}

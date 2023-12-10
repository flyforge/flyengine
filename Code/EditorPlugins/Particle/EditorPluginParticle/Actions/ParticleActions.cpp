#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorPluginParticle/Actions/ParticleActions.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <GuiFoundation/Action/ActionManager.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plActionDescriptorHandle plParticleActions::s_hCategory;
plActionDescriptorHandle plParticleActions::s_hPauseEffect;
plActionDescriptorHandle plParticleActions::s_hRestartEffect;
plActionDescriptorHandle plParticleActions::s_hAutoRestart;
plActionDescriptorHandle plParticleActions::s_hSimulationSpeedMenu;
plActionDescriptorHandle plParticleActions::s_hSimulationSpeed[10];
plActionDescriptorHandle plParticleActions::s_hRenderVisualizers;


void plParticleActions::RegisterActions()
{
  s_hCategory = PLASMA_REGISTER_CATEGORY("ParticleCategory");
  s_hPauseEffect =
    PLASMA_REGISTER_ACTION_1("PFX.Pause", plActionScope::Document, "Particles", "Pause", plParticleAction, plParticleAction::ActionType::PauseEffect);
  s_hRestartEffect =
    PLASMA_REGISTER_ACTION_1("PFX.Restart", plActionScope::Document, "Particles", "F5", plParticleAction, plParticleAction::ActionType::RestartEffect);
  s_hAutoRestart =
    PLASMA_REGISTER_ACTION_1("PFX.AutoRestart", plActionScope::Document, "Particles", "", plParticleAction, plParticleAction::ActionType::AutoRestart);

  s_hSimulationSpeedMenu = PLASMA_REGISTER_MENU_WITH_ICON("PFX.Speed.Menu", ":/EditorPluginParticle/Icons/Speed.svg");
  s_hSimulationSpeed[0] = PLASMA_REGISTER_ACTION_2(
    "PFX.Speed.01", plActionScope::Document, "Particles", "Ctrl+1", plParticleAction, plParticleAction::ActionType::SimulationSpeed, 0.1f);
  s_hSimulationSpeed[1] = PLASMA_REGISTER_ACTION_2(
    "PFX.Speed.025", plActionScope::Document, "Particles", "Ctrl+2", plParticleAction, plParticleAction::ActionType::SimulationSpeed, 0.25f);
  s_hSimulationSpeed[2] = PLASMA_REGISTER_ACTION_2(
    "PFX.Speed.05", plActionScope::Document, "Particles", "Ctrl+3", plParticleAction, plParticleAction::ActionType::SimulationSpeed, 0.5f);
  s_hSimulationSpeed[3] = PLASMA_REGISTER_ACTION_2(
    "PFX.Speed.1", plActionScope::Document, "Particles", "Ctrl+4", plParticleAction, plParticleAction::ActionType::SimulationSpeed, 1.0f);
  s_hSimulationSpeed[4] = PLASMA_REGISTER_ACTION_2(
    "PFX.Speed.15", plActionScope::Document, "Particles", "Ctrl+5", plParticleAction, plParticleAction::ActionType::SimulationSpeed, 1.5f);
  s_hSimulationSpeed[5] = PLASMA_REGISTER_ACTION_2(
    "PFX.Speed.2", plActionScope::Document, "Particles", "Ctrl+6", plParticleAction, plParticleAction::ActionType::SimulationSpeed, 2.0f);
  s_hSimulationSpeed[6] = PLASMA_REGISTER_ACTION_2(
    "PFX.Speed.3", plActionScope::Document, "Particles", "Ctrl+7", plParticleAction, plParticleAction::ActionType::SimulationSpeed, 3.0f);
  s_hSimulationSpeed[7] = PLASMA_REGISTER_ACTION_2(
    "PFX.Speed.4", plActionScope::Document, "Particles", "Ctrl+8", plParticleAction, plParticleAction::ActionType::SimulationSpeed, 4.0f);
  s_hSimulationSpeed[8] = PLASMA_REGISTER_ACTION_2(
    "PFX.Speed.5", plActionScope::Document, "Particles", "Ctrl+9", plParticleAction, plParticleAction::ActionType::SimulationSpeed, 5.0f);
  s_hSimulationSpeed[9] = PLASMA_REGISTER_ACTION_2(
    "PFX.Speed.10", plActionScope::Document, "Particles", "Ctrl+0", plParticleAction, plParticleAction::ActionType::SimulationSpeed, 10.0f);
  s_hRenderVisualizers = PLASMA_REGISTER_ACTION_1(
    "PFX.Render.Visualizers", plActionScope::Document, "Particles", "V", plParticleAction, plParticleAction::ActionType::RenderVisualizers);
}

void plParticleActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCategory);
  plActionManager::UnregisterAction(s_hPauseEffect);
  plActionManager::UnregisterAction(s_hRestartEffect);
  plActionManager::UnregisterAction(s_hAutoRestart);
  plActionManager::UnregisterAction(s_hSimulationSpeedMenu);
  plActionManager::UnregisterAction(s_hRenderVisualizers);

  for (int i = 0; i < PLASMA_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    plActionManager::UnregisterAction(s_hSimulationSpeed[i]);
}

void plParticleActions::MapActions(const char* szMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "ParticleCategory";

  pMap->MapAction(s_hPauseEffect, szSubPath, 0.5f);
  pMap->MapAction(s_hRestartEffect, szSubPath, 1.0f);
  pMap->MapAction(s_hAutoRestart, szSubPath, 2.0f);

  pMap->MapAction(s_hSimulationSpeedMenu, szSubPath, 3.0f);

  plStringBuilder sSubPath(szSubPath, "/PFX.Speed.Menu");

  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    pMap->MapAction(s_hSimulationSpeed[i], sSubPath, i + 1.0f);

  pMap->MapAction(s_hRenderVisualizers, szSubPath, 4.0f);
}

plParticleAction::plParticleAction(const plActionContext& context, const char* szName, plParticleAction::ActionType type, float fSimSpeed)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_fSimSpeed = fSimSpeed;

  m_pEffectDocument = const_cast<plParticleEffectAssetDocument*>(static_cast<const plParticleEffectAssetDocument*>(context.m_pDocument));
  m_pEffectDocument->m_Events.AddEventHandler(plMakeDelegate(&plParticleAction::EffectEventHandler, this));

  switch (m_Type)
  {
    case ActionType::PauseEffect:
      SetIconPath(":/EditorPluginParticle/Icons/Pause.svg");
      break;

    case ActionType::RestartEffect:
      SetIconPath(":/EditorPluginParticle/Icons/Restart.svg");
      break;

    case ActionType::AutoRestart:
      SetIconPath(":/EditorPluginParticle/Icons/Loop.svg");
      break;

    case ActionType::RenderVisualizers:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Visualizers.svg");
      SetChecked(m_pEffectDocument->GetRenderVisualizers());
      break;

    default:
      break;
  }

  UpdateState();
}


plParticleAction::~plParticleAction()
{
  m_pEffectDocument->m_Events.RemoveEventHandler(plMakeDelegate(&plParticleAction::EffectEventHandler, this));
}

void plParticleAction::Execute(const plVariant& value)
{
  switch (m_Type)
  {
    case ActionType::PauseEffect:
      m_pEffectDocument->SetSimulationPaused(!m_pEffectDocument->GetSimulationPaused());
      return;

    case ActionType::RestartEffect:
      m_pEffectDocument->TriggerRestartEffect();
      return;

    case ActionType::AutoRestart:
      m_pEffectDocument->SetAutoRestart(!m_pEffectDocument->GetAutoRestart());
      return;

    case ActionType::SimulationSpeed:
      m_pEffectDocument->SetSimulationSpeed(m_fSimSpeed);
      return;

    case ActionType::RenderVisualizers:
      m_pEffectDocument->SetRenderVisualizers(!m_pEffectDocument->GetRenderVisualizers());
      return;
  }
}

void plParticleAction::EffectEventHandler(const plParticleEffectAssetEvent& e)
{
  switch (e.m_Type)
  {
    case plParticleEffectAssetEvent::AutoRestartChanged:
    case plParticleEffectAssetEvent::SimulationSpeedChanged:
    case plParticleEffectAssetEvent::RenderVisualizersChanged:
      UpdateState();
      break;

    default:
      break;
  }
}

void plParticleAction::UpdateState()
{
  if (m_Type == ActionType::PauseEffect)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetSimulationPaused());
  }

  if (m_Type == ActionType::AutoRestart)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetAutoRestart());
  }

  if (m_Type == ActionType::SimulationSpeed)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetSimulationSpeed() == m_fSimSpeed);
  }

  if (m_Type == ActionType::RenderVisualizers)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetRenderVisualizers());
  }
}

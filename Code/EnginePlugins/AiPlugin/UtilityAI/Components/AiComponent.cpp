#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Components/AiComponent.h>
#include <AiPlugin/UtilityAI/Impl/GameAiBehaviors.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptionGenerators.h>
#include <AiPlugin/UtilityAI/Impl/GameAiSensors.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
//////////////////////////////////////////////////////////////////////////

PL_BEGIN_COMPONENT_TYPE(plAiComponent, 1, plComponentMode::Dynamic)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("DebugInfo", m_bDebugInfo),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("AI/Components"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

plAiComponent::plAiComponent() = default;
plAiComponent::~plAiComponent() = default;

void plAiComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_bDebugInfo;
}

void plAiComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_bDebugInfo;
}

void plAiComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_SensorManager.AddSensor("Sensor_See", PL_DEFAULT_NEW(plAiSensorSpatial, plTempHashedString("Sensor_POI")));
  m_PerceptionManager.AddGenerator(PL_DEFAULT_NEW(plAiPerceptionGenPOI));
  m_PerceptionManager.AddGenerator(PL_DEFAULT_NEW(plAiPerceptionGenWander));
  m_PerceptionManager.AddGenerator(PL_DEFAULT_NEW(plAiPerceptionGenCheckpoint));
  m_BehaviorManager.AddBehavior(PL_DEFAULT_NEW(plAiBehaviorGoToPOI));
  //m_BehaviorManager.AddBehavior(PL_DEFAULT_NEW(plAiBehaviorWander));
  m_BehaviorManager.AddBehavior(PL_DEFAULT_NEW(plAiBehaviorGoToCheckpoint));
  // m_BehaviorManager.AddBehavior(PL_DEFAULT_NEW(plAiBehaviorShoot));
  m_BehaviorManager.AddBehavior(PL_DEFAULT_NEW(plAiBehaviorQuip));
}

void plAiComponent::OnDeactivated()
{
  m_ActionQueue.InterruptAndClear();

  SUPER::OnDeactivated();
}

void plAiComponent::Update()
{
  const auto cs = m_BehaviorManager.ContinueActiveBehavior(*GetOwner(), m_ActionQueue);

  if (cs.m_bEndBehavior)
  {
    m_fLastScore = 0.0f;
    m_BehaviorManager.SetActiveBehavior(*GetOwner(), nullptr, nullptr, m_ActionQueue);
  }

  if (cs.m_bAllowBehaviorSwitch && GetWorld()->GetClock().GetAccumulatedTime() > m_LastAiUpdate + plTime::Seconds(0.5))
  {
    if (!m_BehaviorManager.HasActiveBehavior())
    {
      m_fLastScore = 0.0f;
    }

    m_LastAiUpdate = GetWorld()->GetClock().GetAccumulatedTime();

    m_BehaviorManager.DetermineAvailableBehaviors(m_LastAiUpdate, plMath::Floor(m_fLastScore));

    m_BehaviorManager.FlagNeededPerceptions(m_PerceptionManager);

    m_PerceptionManager.FlagNeededSensors(m_SensorManager);

    m_SensorManager.UpdateNeededSensors(*GetOwner());

    m_PerceptionManager.UpdateNeededPerceptions(*GetOwner(), m_SensorManager);

    const plAiBehaviorCandidate candidate = m_BehaviorManager.DetermineBehaviorCandidate(*GetOwner(), m_PerceptionManager);

    if (candidate.m_pBehavior == m_BehaviorManager.GetActiveBehavior())
    {
      m_fLastScore = candidate.m_fScore;
      m_BehaviorManager.KeepActiveBehavior(*GetOwner(), candidate.m_pPerception, m_ActionQueue);
    }
    else if (candidate.m_fScore > plMath::Ceil(m_fLastScore))
    {
      m_fLastScore = candidate.m_fScore;
      m_BehaviorManager.SetActiveBehavior(*GetOwner(), candidate.m_pBehavior, candidate.m_pPerception, m_ActionQueue);
    }
  }

  m_ActionQueue.Execute(*GetOwner(), GetWorld()->GetClock().GetTimeDiff(), plLog::GetThreadLocalLogSystem());

  if (m_bDebugInfo)
  {
    m_ActionQueue.PrintDebugInfo(*GetOwner());
  }
}

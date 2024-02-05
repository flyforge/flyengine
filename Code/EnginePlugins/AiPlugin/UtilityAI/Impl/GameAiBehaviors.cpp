#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiActionQueue.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionManager.h>
#include <AiPlugin/UtilityAI/Impl/GameAiBehaviors.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptions.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plAiBehaviorGoToPOI::plAiBehaviorGoToPOI() = default;
plAiBehaviorGoToPOI::~plAiBehaviorGoToPOI() = default;

void plAiBehaviorGoToPOI::FlagNeededPerceptions(plAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("plAiPerceptionPOI");
}

plAiBehaviorScore plAiBehaviorGoToPOI::DetermineBehaviorScore(plGameObject& owner, const plAiPerceptionManager& perceptionManager)
{
  if (!perceptionManager.HasPerceptionsOfType("plAiPerceptionPOI"))
    return {};

  plHybridArray<const plAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptionsOfType("plAiPerceptionPOI", perceptions);

  const plVec3 vOwnerPos = owner.GetGlobalPosition();
  float fClosestSqr = plMath::HighValue<float>();

  plAiBehaviorScore res;

  for (const plAiPerception* pPerception0 : perceptions)
  {
    const plAiPerceptionPOI* pPerception = static_cast<const plAiPerceptionPOI*>(pPerception0);

    const float fDistSqr = (pPerception->m_vGlobalPosition - vOwnerPos).GetLengthSquared();

    if (fDistSqr < fClosestSqr)
    {
      fClosestSqr = fDistSqr;
      res.m_pPerception = pPerception;
    }
  }

  if (res.m_pPerception != nullptr)
  {
    res.SetScore(plAiScoreCategory::Command, 0.5f);
  }

  return res;
}

void plAiBehaviorGoToPOI::ActivateBehavior(plGameObject& owner, const plAiPerception* pPerception0, plAiActionQueue& inout_ActionQueue)
{
  const plAiPerceptionPOI* pPerception = static_cast<const plAiPerceptionPOI*>(pPerception0);

  m_vTargetPosition = pPerception->m_vGlobalPosition;

  inout_ActionQueue.CancelCurrentActions(owner);

  //{
  //  auto pAct = plAiActionLerpRotationTowards::Create();
  //  pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_TurnAnglesPerSec = plAngle::MakeFromDegree(90);
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto* pCmd = plAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("MoveForwards");
    pCmd->m_Value = 1.0f;
    inout_ActionQueue.QueueAction(pCmd);
  }
  //{
  //  auto pAct = plAiActionCCMoveTo::Create();
  //  pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_fSpeed = 1.0f;
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto pAct = plAiActionNavigateTo::Create();
    pAct->m_pTargetPosition = &m_vTargetPosition;
    pAct->m_fSpeed = 3.0f;
    pAct->m_fReachedDist = 3.0f;
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = plAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("MoveForwards");
    pCmd->m_Value = 0.0f;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.QueueAction(pCmd);
  }
}

void plAiBehaviorGoToPOI::ReactivateBehavior(plGameObject& owner, const plAiPerception* pPerception0, plAiActionQueue& inout_ActionQueue)
{
  const plAiPerceptionPOI* pPerception = static_cast<const plAiPerceptionPOI*>(pPerception0);
  m_vTargetPosition = pPerception->m_vGlobalPosition;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plAiBehaviorWander::plAiBehaviorWander() = default;
plAiBehaviorWander::~plAiBehaviorWander() = default;

void plAiBehaviorWander::FlagNeededPerceptions(plAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("plAiPerceptionWander");
}

plAiBehaviorScore plAiBehaviorWander::DetermineBehaviorScore(plGameObject& owner, const plAiPerceptionManager& perceptionManager)
{
  if (!perceptionManager.HasPerceptionsOfType("plAiPerceptionWander"))
    return {};

  plHybridArray<const plAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptionsOfType("plAiPerceptionWander", perceptions);

  if (perceptions.IsEmpty())
    return {};

  const plUInt32 uiPerceptionIdx = owner.GetWorld()->GetRandomNumberGenerator().UIntInRange(perceptions.GetCount());

  plAiBehaviorScore res;
  res.SetScore(plAiScoreCategory::ActiveIdle, 0.1f);
  res.m_pPerception = perceptions[uiPerceptionIdx];

  return res;
}

void plAiBehaviorWander::ActivateBehavior(plGameObject& owner, const plAiPerception* pPerception0, plAiActionQueue& inout_ActionQueue)
{
  const plAiPerceptionWander* pPerception = static_cast<const plAiPerceptionWander*>(pPerception0);
  m_vTargetPosition = pPerception->m_vGlobalPosition;

  inout_ActionQueue.CancelCurrentActions(owner);

  //{
  //  auto pAct = plAiActionLerpRotationTowards::Create();
  //  pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_TurnAnglesPerSec = plAngle::MakeFromDegree(90);
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto* pCmd = plAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("MoveForwards");
    pCmd->m_Value = 0.5f;
    inout_ActionQueue.QueueAction(pCmd);
  }
  //{
  //  auto pAct = plAiActionCCMoveTo::Create();
  //  pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_fSpeed = 0.5f;
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto pAct = plAiActionNavigateTo::Create();
    pAct->m_pTargetPosition = &m_vTargetPosition;
    pAct->m_fSpeed = 2.0f;
    pAct->m_fReachedDist = 3.0f;
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = plAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("MoveForwards");
    pCmd->m_Value = 0.0f;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.QueueAction(pCmd);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plAiBehaviorGoToCheckpoint::plAiBehaviorGoToCheckpoint() = default;
plAiBehaviorGoToCheckpoint::~plAiBehaviorGoToCheckpoint() = default;

void plAiBehaviorGoToCheckpoint::FlagNeededPerceptions(plAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("plAiPerceptionCheckpoint");
}

plAiBehaviorScore plAiBehaviorGoToCheckpoint::DetermineBehaviorScore(plGameObject& owner, const plAiPerceptionManager& perceptionManager)
{
  if (!perceptionManager.HasPerceptionsOfType("plAiPerceptionCheckpoint"))
    return {};

  plHybridArray<const plAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptionsOfType("plAiPerceptionCheckpoint", perceptions);

  if (perceptions.IsEmpty())
    return {};

  const plUInt32 uiPerceptionIdx = owner.GetWorld()->GetRandomNumberGenerator().UIntInRange(perceptions.GetCount());

  plAiBehaviorScore res;
  res.SetScore(plAiScoreCategory::ActiveIdle, 0.2f);
  res.m_pPerception = perceptions[uiPerceptionIdx];

  return res;
}

void plAiBehaviorGoToCheckpoint::ActivateBehavior(plGameObject& owner, const plAiPerception* pPerception0, plAiActionQueue& inout_ActionQueue)
{
  const plAiPerceptionCheckpoint* pPerception = static_cast<const plAiPerceptionCheckpoint*>(pPerception0);
  m_vTargetPosition = pPerception->m_vGlobalPosition;

  inout_ActionQueue.CancelCurrentActions(owner);

  //{
  //  auto pAct = plAiActionLerpRotationTowards::Create();
  //  pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_TurnAnglesPerSec = plAngle::MakeFromDegree(90);
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto* pCmd = plAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("MoveForwards");
    pCmd->m_Value = 0.5f;
    inout_ActionQueue.QueueAction(pCmd);
  }
  //{
  //  auto pAct = plAiActionCCMoveTo::Create();
  //  pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_fSpeed = 0.5f;
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto pAct = plAiActionNavigateTo::Create();
    pAct->m_pTargetPosition = &m_vTargetPosition;
    pAct->m_fSpeed = 2.0f;
    pAct->m_fReachedDist = 3.0f;
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = plAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("MoveForwards");
    pCmd->m_Value = 0.0f;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.QueueAction(pCmd);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plAiBehaviorShoot::plAiBehaviorShoot() = default;
plAiBehaviorShoot::~plAiBehaviorShoot() = default;

void plAiBehaviorShoot::FlagNeededPerceptions(plAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("plAiPerceptionPOI");
}

plAiBehaviorScore plAiBehaviorShoot::DetermineBehaviorScore(plGameObject& owner, const plAiPerceptionManager& perceptionManager)
{
  if (!perceptionManager.HasPerceptionsOfType("plAiPerceptionPOI"))
    return {};

  plHybridArray<const plAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptionsOfType("plAiPerceptionPOI", perceptions);

  const plVec3 vOwnerPos = owner.GetGlobalPosition();
  const plVec3 vOwnerDir = owner.GetGlobalDirForwards();
  float fClosest = plMath::HighValue<float>();

  plAiBehaviorScore res;

  for (const plAiPerception* pPerception0 : perceptions)
  {
    const plAiPerceptionPOI* pPerception = static_cast<const plAiPerceptionPOI*>(pPerception0);

    plVec3 vDirTo = pPerception->m_vGlobalPosition - vOwnerPos;
    const float fDist = vDirTo.GetLengthAndNormalize();

    if (fDist < 2.0f || fDist > 5.0f)
      continue;

    if (fDist >= fClosest)
      continue;

    fClosest = fDist;
    res.m_pPerception = pPerception;
  }

  if (res.m_pPerception != nullptr)
  {
    res.SetScore(plAiScoreCategory::Combat, 0.1f);
  }

  return res;
}

void plAiBehaviorShoot::ActivateBehavior(plGameObject& owner, const plAiPerception* pPerception0, plAiActionQueue& inout_ActionQueue)
{
  const plAiPerceptionPOI* pPerception = static_cast<const plAiPerceptionPOI*>(pPerception0);

  inout_ActionQueue.CancelCurrentActions(owner);

  {
    auto pAct = plAiActionLerpRotationTowards::Create();
    pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
    pAct->m_TurnAnglesPerSec = plAngle::MakeFromDegree(90);
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = plAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("Aim");
    pCmd->m_Value = 1;
    inout_ActionQueue.QueueAction(pCmd);
  }
  {
    auto* pCmd = plAiActionWait::Create();
    pCmd->m_Duration = plTime::Seconds(0.5);
    inout_ActionQueue.QueueAction(pCmd);
  }
  {
    auto pAct = plAiActionSpawn::Create();
    pAct->m_sChildObjectName = plTempHashedString("Spawn");
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = plAiActionBlackboardSetAndWait::Create();
    pCmd->m_sEntryName.Assign("Shoot");
    pCmd->m_SetValue = 1;
    pCmd->m_WaitValue = 0;
    inout_ActionQueue.QueueAction(pCmd);
  }
  {
    auto* pCmd = plAiActionWait::Create();
    pCmd->m_Duration = plTime::Seconds(0.5);
    inout_ActionQueue.QueueAction(pCmd);
  }
  {
    auto* pCmd = plAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("Aim");
    pCmd->m_Value = 0;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.QueueAction(pCmd);
  }
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plAiBehaviorQuip::plAiBehaviorQuip() = default;
plAiBehaviorQuip::~plAiBehaviorQuip() = default;

void plAiBehaviorQuip::FlagNeededPerceptions(plAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("plAiPerceptionPOI");
}

plAiBehaviorScore plAiBehaviorQuip::DetermineBehaviorScore(plGameObject& owner, const plAiPerceptionManager& perceptionManager)
{
  if (!perceptionManager.HasPerceptionsOfType("plAiPerceptionPOI"))
    return {};

  plHybridArray<const plAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptionsOfType("plAiPerceptionPOI", perceptions);

  plAiBehaviorScore res;
  res.SetScore(plAiScoreCategory::Command, 0.6f);

  return res;
}

void plAiBehaviorQuip::ActivateBehavior(plGameObject& owner, const plAiPerception* pPerception0, plAiActionQueue& inout_ActionQueue)
{
  inout_ActionQueue.CancelCurrentActions(owner);

  {
    auto pAct = plAiActionQuip::Create();
    pAct->m_sLogMsg = "I SEE YOU !";
    inout_ActionQueue.QueueAction(pAct);
  }
}

plTime plAiBehaviorQuip::GetCooldownDuration()
{
  return plTime::Seconds(5);
}

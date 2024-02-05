#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/Components/NavigationComponent.h>
#include <AiPlugin/UtilityAI/Impl/GameAiActions.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Gameplay/SpawnComponent.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_AICMD(plAiActionWait);

plAiActionWait::plAiActionWait() = default;
plAiActionWait::~plAiActionWait() = default;

void plAiActionWait::Reset()
{
  m_Duration = plTime::MakeZero();
}

void plAiActionWait::GetDebugDesc(plStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Wait: {}", m_Duration);
}

plAiActionResult plAiActionWait::Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog)
{
  if (tDiff >= m_Duration)
    return plAiActionResult::Finished; // or canceled

  m_Duration -= tDiff;
  return plAiActionResult::Succeded;
}

void plAiActionWait::Cancel(plGameObject& owner)
{
  m_Duration = plTime::MakeZero();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_AICMD(plAiActionLerpRotation);

plAiActionLerpRotation::plAiActionLerpRotation() = default;
plAiActionLerpRotation::~plAiActionLerpRotation() = default;

void plAiActionLerpRotation::Reset()
{
  m_vTurnAxis = plVec3::MakeAxisZ();
  m_TurnAngle = {};
  m_TurnAnglesPerSec = {};
}

void plAiActionLerpRotation::GetDebugDesc(plStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Turn: {}/{}/{} - {} @ {}/sec", m_vTurnAxis.x, m_vTurnAxis.y, m_vTurnAxis.z, m_TurnAngle, m_TurnAnglesPerSec);
}

plAiActionResult plAiActionLerpRotation::Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog)
{
  if (m_TurnAnglesPerSec <= plAngle())
    return plAiActionResult::Finished; // or canceled

  if (m_TurnAngle < plAngle())
  {
    m_TurnAngle = -m_TurnAngle;
    m_vTurnAxis = -m_vTurnAxis;
  }

  const plAngle turnAmount = tDiff.AsFloatInSeconds() * m_TurnAnglesPerSec;
  const plAngle toTurn = plMath::Min(m_TurnAngle, turnAmount);

  plQuat qRot = plQuat::MakeFromAxisAndAngle(m_vTurnAxis, toTurn);

  const plQuat qCurRot = owner.GetGlobalRotation();

  owner.SetGlobalRotation(qRot * qCurRot);

  if (turnAmount >= m_TurnAngle)
    return plAiActionResult::Finished;

  m_TurnAngle -= toTurn;
  return plAiActionResult::Succeded;
}

void plAiActionLerpRotation::Cancel(plGameObject& owner)
{
  m_TurnAnglesPerSec = {};
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_AICMD(plAiActionLerpPosition);

plAiActionLerpPosition::plAiActionLerpPosition() = default;
plAiActionLerpPosition::~plAiActionLerpPosition() = default;

void plAiActionLerpPosition::Reset()
{
  m_fSpeed = 0.0f;
  m_vLocalSpaceSlide = plVec3::MakeZero();
}

void plAiActionLerpPosition::GetDebugDesc(plStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Slide: {}/{}/{} @ {}/sec", m_vLocalSpaceSlide.x, m_vLocalSpaceSlide.y, m_vLocalSpaceSlide.z, m_fSpeed);
}

plAiActionResult plAiActionLerpPosition::Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog)
{
  if (m_vLocalSpaceSlide.IsZero())
    return plAiActionResult::Finished; // or canceled

  if (m_fSpeed <= 0.0f)
  {
    plLog::Error(pLog, "plAiActionLerpPosition: Invalid speed '{}'", m_fSpeed);
    return plAiActionResult::Failed;
  }

  plVec3 vSlideDir = m_vLocalSpaceSlide;
  const float fMaxSlide = vSlideDir.GetLengthAndNormalize();

  const float fSlideAmount = tDiff.AsFloatInSeconds() * m_fSpeed;
  const float fSlideDist = plMath::Min(fMaxSlide, fSlideAmount);
  const plVec3 vSlide = fSlideDist * vSlideDir;

  const plVec3 vCurGlobalPos = owner.GetGlobalPosition();

  owner.SetGlobalPosition(vCurGlobalPos + owner.GetGlobalRotation() * vSlide);

  if (fSlideAmount >= fMaxSlide)
    return plAiActionResult::Finished;

  m_vLocalSpaceSlide -= vSlide;
  return plAiActionResult::Succeded;
}

void plAiActionLerpPosition::Cancel(plGameObject& owner)
{
  m_fSpeed = 0.0f;
  m_vLocalSpaceSlide.SetZero();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_AICMD(plAiActionLerpRotationTowards);

plAiActionLerpRotationTowards::plAiActionLerpRotationTowards() = default;
plAiActionLerpRotationTowards::~plAiActionLerpRotationTowards() = default;

void plAiActionLerpRotationTowards::Reset()
{
  m_vTargetPosition = plVec3::MakeZero();
  m_hTargetObject.Invalidate();
  m_TurnAnglesPerSec = {};
}

void plAiActionLerpRotationTowards::GetDebugDesc(plStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Turn Towards: {}/{}/{} - @{}/sec", m_vTargetPosition.x, m_vTargetPosition.y, m_vTargetPosition.z, m_TurnAnglesPerSec);
}

plAiActionResult plAiActionLerpRotationTowards::Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog)
{
  if (m_TurnAnglesPerSec <= plAngle())
    return plAiActionResult::Finished; // or canceled

  if (!m_hTargetObject.IsInvalidated())
  {
    plGameObject* pTarget;
    if (!owner.GetWorld()->TryGetObject(m_hTargetObject, pTarget))
    {
      plLog::Error(pLog, "plAiActionLerpRotationTowards: Target object doesn't exist (anymore).");
      return plAiActionResult::Failed;
    }

    m_vTargetPosition = pTarget->GetGlobalPosition();
  }

  const plVec3 vOwnPos = owner.GetGlobalPosition();

  plVec3 vCurDir = owner.GetGlobalDirForwards();
  vCurDir.z = 0.0f;

  plVec3 vTargetDir = m_vTargetPosition - vOwnPos;
  vTargetDir.z = 0.0f;

  m_vTargetPosition.z = vOwnPos.z;

  if (vCurDir.NormalizeIfNotZero(plVec3::MakeZero()).Failed())
  {
    plLog::Error(pLog, "plAiActionLerpRotationTowards: Invalid current direction {}/{}/{}", vCurDir.x, vCurDir.y, vCurDir.z);
    return plAiActionResult::Failed;
  }

  if (vTargetDir.NormalizeIfNotZero(plVec3::MakeZero()).Failed())
  {
    plLog::Error(pLog, "plAiActionLerpRotationTowards: Invalid target direction {}/{}/{}", vTargetDir.x, vTargetDir.y, vTargetDir.z);
    return plAiActionResult::Failed;
  }

  if (vCurDir.IsEqual(vTargetDir, 0.001f))
    return plAiActionResult::Finished;

  const plAngle turnAngle = vCurDir.GetAngleBetween(vTargetDir);

  const plVec3 vTurnAxis = vCurDir.CrossRH(vTargetDir).GetNormalized();

  const plAngle turnAmount = tDiff.AsFloatInSeconds() * m_TurnAnglesPerSec;
  const plAngle toTurn = plMath::Min(turnAngle, turnAmount);

  plQuat qRot = plQuat::MakeFromAxisAndAngle(vTurnAxis, toTurn);

  const plQuat qCurRot = owner.GetGlobalRotation();

  owner.SetGlobalRotation(qRot * qCurRot);

  if (turnAngle - toTurn <= m_TargetReachedAngle)
    return plAiActionResult::Finished;

  return plAiActionResult::Succeded;
}

void plAiActionLerpRotationTowards::Cancel(plGameObject& owner)
{
  m_TurnAnglesPerSec = {};
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// PL_IMPLEMENT_AICMD(plAiActionFollowPath);
//
// plAiActionFollowPath::plAiActionFollowPath() = default;
// plAiActionFollowPath::~plAiActionFollowPath() = default;
//
// void plAiActionFollowPath::Reset()
//{
//  m_fSpeed = 0.0f;
//  m_hPath.Invalidate();
//}
//
// void plAiActionFollowPath::GetDebugDesc(plStringBuilder& inout_sText)
//{
//  inout_sText.Format("Follow Path: @{}/sec", m_fSpeed);
//}
//
// plAiActionResult plAiActionFollowPath::Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog)
//{
//  if (m_hPath.IsInvalidated())
//    return plAiActionResult::Finished; // or canceled
//
//  plGameObject* pPath;
//  if (!owner.GetWorld()->TryGetObject(m_hPath, pPath))
//  {
//    plLog::Error(pLog, "plAiActionFollowPath: Target object doesn't exist (anymore).");
//    return plAiActionResult::Failed;
//  }
//
//
//  return plAiActionResult::Succeded;
//}
//
// void plAiActionFollowPath::Cancel(plGameObject& owner)
//{
//  m_hPath.Invalidate();
//}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_AICMD(plAiActionBlackboardSetEntry);

plAiActionBlackboardSetEntry::plAiActionBlackboardSetEntry() = default;
plAiActionBlackboardSetEntry::~plAiActionBlackboardSetEntry() = default;

void plAiActionBlackboardSetEntry::Reset()
{
  m_sEntryName.Clear();
  m_Value = {};
}

void plAiActionBlackboardSetEntry::GetDebugDesc(plStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Set Blackboard Entry '{}' to '{}'", m_sEntryName.GetHash(), m_Value);
}

plAiActionResult plAiActionBlackboardSetEntry::Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog)
{
  if (m_sEntryName.IsEmpty() || !m_Value.IsValid())
    return plAiActionResult::Finished; // or canceled

  auto pBlackboard = plBlackboardComponent::FindBlackboard(&owner);

  if (pBlackboard == nullptr)
  {
    plLog::Error(pLog, "plAiActionBlackboardSetEntry: No Blackboard available.");
    return plAiActionResult::Failed;
  }

  plLog::Debug("Setting '{}' to {}", m_sEntryName, m_Value.ConvertTo<plString>());
  pBlackboard->SetEntryValue(m_sEntryName, m_Value);
  return plAiActionResult::Finished;
}

void plAiActionBlackboardSetEntry::Cancel(plGameObject& owner)
{
  if (!m_bNoCancel)
  {
    Reset();
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_AICMD(plAiActionBlackboardWait);

plAiActionBlackboardWait::plAiActionBlackboardWait() = default;
plAiActionBlackboardWait::~plAiActionBlackboardWait() = default;

void plAiActionBlackboardWait::Reset()
{
  m_sEntryName.Clear();
  m_Value = {};
  m_bEquals = true;
}

void plAiActionBlackboardWait::GetDebugDesc(plStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Wait for Blackboard Entry '{}' {} '{}'", m_sEntryName.GetHash(), m_bEquals ? "==" : "!=", m_Value);
}

plAiActionResult plAiActionBlackboardWait::Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog)
{
  if (m_sEntryName.IsEmpty())
    return plAiActionResult::Finished; // or canceled

  auto pBlackboard = plBlackboardComponent::FindBlackboard(&owner);

  if (pBlackboard == nullptr)
  {
    plLog::Error(pLog, "plAiActionBlackboardWait: No Blackboard available.");
    return plAiActionResult::Failed;
  }

  const plVariant val = pBlackboard->GetEntryValue(m_sEntryName, m_Value);
  const bool bIsEqual = (val == m_Value);

  if (m_bEquals == bIsEqual)
    return plAiActionResult::Finished;

  return plAiActionResult::Succeded;
}

void plAiActionBlackboardWait::Cancel(plGameObject& owner)
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_AICMD(plAiActionBlackboardSetAndWait);

plAiActionBlackboardSetAndWait::plAiActionBlackboardSetAndWait() = default;
plAiActionBlackboardSetAndWait::~plAiActionBlackboardSetAndWait() = default;

void plAiActionBlackboardSetAndWait::Reset()
{
  m_sEntryName.Clear();
  m_SetValue = {};
  m_WaitValue = {};
  m_bEqualsWaitValue = true;
}

void plAiActionBlackboardSetAndWait::GetDebugDesc(plStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Set BB '{}' to {}, wait {} '{}'", m_sEntryName.GetHash(), m_SetValue, m_bEqualsWaitValue ? "==" : "!=", m_WaitValue);
}

plAiActionResult plAiActionBlackboardSetAndWait::Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog)
{
  if (m_sEntryName.IsEmpty())
    return plAiActionResult::Finished; // or canceled

  auto pBlackboard = plBlackboardComponent::FindBlackboard(&owner);

  if (pBlackboard == nullptr)
  {
    plLog::Error(pLog, "plAiActionBlackboardSetAndWait: No Blackboard available.");
    return plAiActionResult::Failed;
  }

  if (m_SetValue.IsValid())
  {
    pBlackboard->SetEntryValue(m_sEntryName, m_SetValue);
    m_SetValue = {};
  }

  const plVariant val = pBlackboard->GetEntryValue(m_sEntryName, m_WaitValue);
  const bool bIsEqual = (val == m_WaitValue);

  if (m_bEqualsWaitValue == bIsEqual)
    return plAiActionResult::Finished;

  return plAiActionResult::Succeded;
}

void plAiActionBlackboardSetAndWait::Cancel(plGameObject& owner)
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_AICMD(plAiActionCCMoveTo);

plAiActionCCMoveTo::plAiActionCCMoveTo() = default;
plAiActionCCMoveTo::~plAiActionCCMoveTo() = default;

void plAiActionCCMoveTo::Reset()
{
  m_vTargetPosition = plVec3::MakeZero();
  m_hTargetObject.Invalidate();
  m_fSpeed = 0.0f;
  m_fReachedDistSQR = 1.0f;
}

void plAiActionCCMoveTo::GetDebugDesc(plStringBuilder& inout_sText)
{
  inout_sText.SetFormat("CCMoveTo: {}/{}/{} - @{}/sec", m_vTargetPosition.x, m_vTargetPosition.y, m_vTargetPosition.z, m_fSpeed);
}

plAiActionResult plAiActionCCMoveTo::Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog)
{
  if (m_fSpeed <= 0.0f)
    return plAiActionResult::Finished; // or canceled

  if (!m_hTargetObject.IsInvalidated())
  {
    plGameObject* pTarget;
    if (!owner.GetWorld()->TryGetObject(m_hTargetObject, pTarget))
    {
      plLog::Error(pLog, "plAiActionCCMoveTo: Target object doesn't exist (anymore).");
      return plAiActionResult::Failed;
    }

    m_vTargetPosition = pTarget->GetGlobalPosition();
  }

  const plVec3 vOwnPos = owner.GetGlobalPosition();
  plVec3 vDir = m_vTargetPosition - vOwnPos;
  vDir.z = 0.0f; // TODO: not the best idea


  if (vDir.GetLengthSquared() <= m_fReachedDistSQR)
    return plAiActionResult::Finished;

  vDir.z = 0.0f;

  plQuat qRot = plQuat::MakeShortestRotation(plVec3::MakeAxisX(), vDir.GetNormalized());
  owner.SetGlobalRotation(qRot);

  // const plVec3 vLocalDir = -owner.GetGlobalTransform().m_qRotation * vDir;

  plMsgMoveCharacterController msg;
  msg.m_fMoveForwards = m_fSpeed;
  // msg.m_fMoveForwards = plMath::Clamp(vLocalDir.x, 0.0f, 1.0f);
  // msg.m_fMoveBackwards = plMath::Clamp(-vLocalDir.x, 0.0f, 1.0f);
  // msg.m_fStrafeLeft = plMath::Clamp(-vLocalDir.y, 0.0f, 1.0f);
  // msg.m_fStrafeRight = plMath::Clamp(vLocalDir.y, 0.0f, 1.0f);
  owner.SendMessage(msg);

  return plAiActionResult::Succeded;
}

void plAiActionCCMoveTo::Cancel(plGameObject& owner)
{
  m_fSpeed = 0.0f;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_AICMD(plAiActionSpawn);

plAiActionSpawn::plAiActionSpawn() = default;
plAiActionSpawn::~plAiActionSpawn() = default;

void plAiActionSpawn::Reset()
{
  m_sChildObjectName.Clear();
}

void plAiActionSpawn::GetDebugDesc(plStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Spawn: {}", m_sChildObjectName.GetHash());
}

plAiActionResult plAiActionSpawn::Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog)
{
  if (m_sChildObjectName.IsEmpty())
    return plAiActionResult::Finished;

  plGameObject* pSpawner = owner.FindChildByName(m_sChildObjectName);
  if (pSpawner == nullptr)
  {
    plLog::Error(pLog, "plAiActionSpawn: Child with name hash '{}' doesn't exist.", m_sChildObjectName.GetHash());
    return plAiActionResult::Failed;
  }

  plSpawnComponent* pSpawn = nullptr;
  if (!pSpawner->TryGetComponentOfBaseType(pSpawn))
  {
    plLog::Error(pLog, "plAiActionSpawn: plSpawnComponent does not exist on child with name hash '{}'.", m_sChildObjectName.GetHash());
    return plAiActionResult::Failed;
  }

  if (pSpawn->TriggerManualSpawn())
    return plAiActionResult::Finished;
  else
    return plAiActionResult::Succeded; // wait
}

void plAiActionSpawn::Cancel(plGameObject& owner)
{
  m_sChildObjectName.Clear();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_AICMD(plAiActionQuip);

plAiActionQuip::plAiActionQuip() = default;
plAiActionQuip::~plAiActionQuip() = default;

void plAiActionQuip::Reset()
{
  m_sLogMsg.Clear();
}

void plAiActionQuip::GetDebugDesc(plStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Log: {}", m_sLogMsg);
}

plAiActionResult plAiActionQuip::Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog)
{
  if (m_sLogMsg.IsEmpty())
    return plAiActionResult::Finished; // or canceled

  plLog::Info(m_sLogMsg);

  return plAiActionResult::Finished;
}

void plAiActionQuip::Cancel(plGameObject& owner)
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_AICMD(plAiActionNavigateTo);

plAiActionNavigateTo::plAiActionNavigateTo() = default;
plAiActionNavigateTo::~plAiActionNavigateTo() = default;

void plAiActionNavigateTo::Reset()
{
  m_pTargetPosition = nullptr;
  m_fSpeed = 0.0f;
  m_fReachedDist = 1.0f;
  m_bStarted = false;
}

void plAiActionNavigateTo::GetDebugDesc(plStringBuilder& inout_sText)
{
  inout_sText.SetFormat("NavTo: {}/{}/{} - @{}/sec", m_pTargetPosition->x, m_pTargetPosition->y, m_pTargetPosition->z, m_fSpeed);
}

plAiActionResult plAiActionNavigateTo::Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog)
{
  if (m_pTargetPosition == nullptr || m_fSpeed <= 0.0f)
    return plAiActionResult::Finished; // or canceled

  plAiNavigationComponent* pNav;
  if (!owner.TryGetComponentOfBaseType(pNav))
  {
    plLog::Error(pLog, "No plAiNavigationComponent found.");
    return plAiActionResult::Failed;
  }

  switch (pNav->GetState())
  {
    case plAiNavigationComponent::Idle:
      if (m_bStarted)
      {
        pNav->CancelNavigation();
        return plAiActionResult::Finished;
      }
      break;

    case plAiNavigationComponent::Failed:
      plLog::Error(pLog, "Path navigation failed.");
      return plAiActionResult::Failed;
  }

  pNav->m_fReachedDistance = m_fReachedDist;
  pNav->m_fSpeed = m_fSpeed;

  m_bStarted = true;
  pNav->SetDestination(*m_pTargetPosition);
  return plAiActionResult::Succeded;
}

void plAiActionNavigateTo::Cancel(plGameObject& owner)
{
  m_fSpeed = 0.0f;
}

#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <RecastPlugin/Utils/RcMath.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <DetourCrowd.h>
#include <RecastPlugin/WorldModule/DetourCrowdWorldModule.h>
#include <RecastPlugin/Components/DetourCrowdAgentComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plDetourCrowdAgentRotationMode, 1)
  PL_ENUM_CONSTANTS(plDetourCrowdAgentRotationMode::LookAtNextPathCorner, plDetourCrowdAgentRotationMode::MatchVelocityDirection)
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plDetourCrowdAgentComponent, 1, plComponentMode::Dynamic)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("AI/Recast"),
  }
  PL_END_ATTRIBUTES;

  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Radius",m_fRadius)->AddAttributes(new plDefaultValueAttribute(0.3f),new plClampValueAttribute(0.0f, plVariant())),
    PL_MEMBER_PROPERTY("Height",m_fHeight)->AddAttributes(new plDefaultValueAttribute(1.8f),new plClampValueAttribute(0.01f, plVariant())),
    PL_MEMBER_PROPERTY("MaxSpeed",m_fMaxSpeed)->AddAttributes(new plDefaultValueAttribute(3.5f),new plClampValueAttribute(0.0f, plVariant())),
    PL_MEMBER_PROPERTY("MaxAcceleration",m_fMaxAcceleration)->AddAttributes(new plDefaultValueAttribute(10.0f),new plClampValueAttribute(0.0f, plVariant())),
    PL_MEMBER_PROPERTY("StoppingDistance",m_fStoppingDistance)->AddAttributes(new plDefaultValueAttribute(1.0f),new plClampValueAttribute(0.001f, plVariant())),
    PL_MEMBER_PROPERTY("MaxAngularSpeed",m_MaxAngularSpeed)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(360.0f)),new plClampValueAttribute(0.0f, plVariant())),
    PL_ENUM_MEMBER_PROPERTY("RotationMode",plDetourCrowdAgentRotationMode,m_RotationMode),
    PL_MEMBER_PROPERTY("Pushiness",m_fPushiness)->AddAttributes(new plDefaultValueAttribute(2.0f),new plClampValueAttribute(0.0f, plVariant())),
  }
  PL_END_PROPERTIES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plDetourCrowdAgentComponent::plDetourCrowdAgentComponent()
{
  m_uiTargetDirtyBit = 0;
  m_uiSteeringFailedBit = 0;
  m_uiErrorBit = 0;
  m_uiParamsDirtyBit = 0;
}

plDetourCrowdAgentComponent::~plDetourCrowdAgentComponent() = default;

void plDetourCrowdAgentComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  plStreamWriter& s = stream.GetStream();

  s << m_fRadius;
  s << m_fHeight;
  s << m_fMaxSpeed;
  s << m_fMaxAcceleration;
  s << m_fStoppingDistance;
  s << m_MaxAngularSpeed;
  s << m_RotationMode;
  s << m_fPushiness;
}

void plDetourCrowdAgentComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = stream.GetStream();

  s >> m_fRadius;
  s >> m_fHeight;
  s >> m_fMaxSpeed;
  s >> m_fMaxAcceleration;
  s >> m_fStoppingDistance;
  s >> m_MaxAngularSpeed;
  s >> m_RotationMode;
  s >> m_fPushiness;
}

void plDetourCrowdAgentComponent::FillAgentParams(plDetourCrowdAgentParams& out_params) const
{
  out_params.m_fRadius = m_fRadius;
  out_params.m_fHeight = m_fHeight;
  out_params.m_fMaxSpeed = m_fMaxSpeed;
  out_params.m_fMaxAcceleration = m_fMaxAcceleration;
  out_params.m_fSeparationWeight = m_fPushiness;
}

void plDetourCrowdAgentComponent::SetRadius(float fRadius)
{
  if (fRadius < 0)
    fRadius = 0;

  if (fRadius == m_fRadius)
    return;

  m_fRadius = fRadius;
  m_uiParamsDirtyBit = 1;
}

void plDetourCrowdAgentComponent::SetHeight(float fHeight)
{
  if (fHeight < 0.01f)
    fHeight = 0.01f;

  if (fHeight == m_fHeight)
    return;

  m_fHeight = fHeight;
  m_uiParamsDirtyBit = 1;
}

void plDetourCrowdAgentComponent::SetMaxSpeed(float fMaxSpeed)
{
  if (fMaxSpeed < 0.0f)
    fMaxSpeed = 0.0f;

  if (fMaxSpeed == m_fMaxSpeed)
    return;

  m_fMaxSpeed = fMaxSpeed;
  m_uiParamsDirtyBit = 1;
}

void plDetourCrowdAgentComponent::SetMaxAcceleration(float fMaxAcceleration)
{
  if (fMaxAcceleration < 0.0f)
    fMaxAcceleration = 0.0f;

  if (m_fMaxAcceleration == fMaxAcceleration)
    return;

  m_fMaxAcceleration = fMaxAcceleration;
  m_uiParamsDirtyBit = 1;
}

void plDetourCrowdAgentComponent::SetStoppingDistance(float fStoppingDistance)
{
  if (fStoppingDistance < 0.001f)
    fStoppingDistance = 0.001f;

  m_fStoppingDistance = fStoppingDistance;
}

void plDetourCrowdAgentComponent::SetMaxAngularSpeed(plAngle maxAngularSpeed)
{
  if (maxAngularSpeed.GetRadian() < 0.0f)
    maxAngularSpeed.SetRadian(0.0f);

  m_MaxAngularSpeed = maxAngularSpeed;
}

void plDetourCrowdAgentComponent::SetPushiness(float fPushiness)
{
  if (fPushiness < 0.0f)
    fPushiness = 0.0f;

  if (fPushiness == m_fPushiness)
    return;

  m_fPushiness = fPushiness;
  m_uiParamsDirtyBit = 1;
}

void plDetourCrowdAgentComponent::SetTargetPosition(const plVec3& vPosition)
{
  m_vTargetPosition = vPosition;
  m_PathToTargetState = plAgentPathFindingState::HasTargetWaitingForPath;
  m_uiTargetDirtyBit = 1;
}

void plDetourCrowdAgentComponent::ClearTargetPosition()
{
  m_PathToTargetState = plAgentPathFindingState::HasNoTarget;
}

void plDetourCrowdAgentComponent::OnDeactivated()
{
  m_uiErrorBit = 0;

  SUPER::OnDeactivated();
}

plQuat plDetourCrowdAgentComponent::RotateTowardsDirection(const plQuat& qCurrentRot, const plVec3& vTargetDir, plAngle& out_angularSpeed) const
{
  // This function makes use of the fact that the object is always upright
  
  plAngle currentAngle = 2.0 * plMath::ACos(qCurrentRot.w);
  if (qCurrentRot.z < 0)
    currentAngle = plAngle::MakeFromRadian(2.0f * plMath::Pi<float>() - currentAngle.GetRadian());

  plAngle targetAngle = plMath::ATan2(vTargetDir.y, vTargetDir.x);
  if (targetAngle.GetRadian() < 0)
    targetAngle = plAngle::MakeFromRadian(targetAngle.GetRadian() + 2.0f * plMath::Pi<float>());

  float fAngleDiff = targetAngle.GetRadian() - currentAngle.GetRadian();
  if (fAngleDiff > plMath::Pi<float>())
    fAngleDiff -= 2.0f * plMath::Pi<float>();
  else if (fAngleDiff < -plMath::Pi<float>())
    fAngleDiff += 2.0f * plMath::Pi<float>();
  fAngleDiff = plMath::Sign(fAngleDiff) * plMath::Min(plMath::Abs(fAngleDiff), m_MaxAngularSpeed.GetRadian() * GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds());

  out_angularSpeed = plAngle::MakeFromRadian(fAngleDiff);
  return plQuat::MakeFromAxisAndAngle(plVec3(0, 0, 1), plAngle::MakeFromRadian(currentAngle.GetRadian() + fAngleDiff));
}

plVec3 plDetourCrowdAgentComponent::GetDirectionToNextPathCorner(const plVec3& vCurrentPos, const struct dtCrowdAgent* pDtAgent) const
{
  if (pDtAgent->ncorners > 0)
  {
    plVec3 vNextCorner = plRcPos(pDtAgent->cornerVerts);
    plVec3 vDiff = vNextCorner - vCurrentPos;

    if (vDiff.GetLengthSquared() > 0.0001f)
      return vDiff;

    if (pDtAgent->ncorners > 1)
    {
      vNextCorner = plRcPos(pDtAgent->cornerVerts + 3);
      return vNextCorner - vCurrentPos;
    }
  }

  return plVec3::MakeZero();
}

bool plDetourCrowdAgentComponent::SyncRotation(const plVec3& vPosition, plQuat& input_qRotation, const plVec3& vVelocity, const struct dtCrowdAgent* pDtAgent)
{
  m_AngularSpeed.SetRadian(0);

  if (m_MaxAngularSpeed.GetRadian() <= 0.0f)
    return false;

  plVec3 vDirection;

  switch (m_RotationMode)
  {
    case plDetourCrowdAgentRotationMode::LookAtNextPathCorner:
      vDirection = GetDirectionToNextPathCorner(vPosition, pDtAgent);
      break;
    case plDetourCrowdAgentRotationMode::MatchVelocityDirection:
      vDirection = vVelocity;
      break;
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  };

  vDirection.z = 0;

  if (vDirection.IsZero())
    return false;

  vDirection.Normalize();
  input_qRotation = RotateTowardsDirection(input_qRotation, vDirection, m_AngularSpeed);

  return true;
}

void plDetourCrowdAgentComponent::SyncTransform(const struct dtCrowdAgent* pDtAgent, bool bTeleport)
{
  const plVec3 vPosition = plRcPos(pDtAgent->npos);
  const plVec3 vVelocity = plRcPos(pDtAgent->vel);

  //if (m_MovementMode == plDetourCrowdAgentMovementMode::SetPositionDirectly)
  {
    m_vVelocity = vVelocity;

    plTransform xform = GetOwner()->GetGlobalTransform();
    xform.m_vPosition = vPosition;
    SyncRotation(vPosition, xform.m_qRotation, vVelocity, pDtAgent);
    GetOwner()->SetGlobalTransform(xform);
  }

  // Below is the unfinished CC code that might be useful in future

  //else if (m_MovementMode == plDetourCrowdAgentMovementMode::SendMsgToCharacterController)
  //{
  //  if (!bTeleport)
  //  {
  //    const float fDeltaTime = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();
  //    plTransform xform = GetOwner()->GetGlobalTransform();
//
  //    plVec3 vDiff = vPosition - xform.m_vPosition;
  //    vDiff.z = 0;
//
  //    if (SyncRotation(xform.m_vPosition, xform.m_qRotation, vDiff, pDtAgent))
  //      GetOwner()->SetGlobalTransform(xform);
//
  //    const float fDistance = vDiff.GetLength();
  //    if (fDistance < 0.01f)
  //    {
  //      m_vVelocity = plVec3::MakeZero();
  //      return;
  //    }
  //    vDiff /= fDistance;
  //    
  //    // Speed is increased in case we're trying to chase the dtAgent
  //    const float fSpeed = plMath::Min(m_fMaxSpeed * 1.3f, fDistance / fDeltaTime);
//
  //    m_vVelocity = vDiff * fSpeed;
//
  //    if (fDistance > 0.25f)
  //    {
  //      plAgentSteeringEvent e;
  //      e.m_pComponent = this;
  //      e.m_Type = plAgentSteeringEvent::ErrorSteeringFailed;
  //      m_SteeringEvents.Broadcast(e);
//
  //      ClearTargetPosition();
  //      m_uiSteeringFailedBit = 1;
//
  //      return;
  //    }
//
  //    const plVec3 vRelativeVelocity = GetOwner()->GetGlobalRotation().GetInverse() * m_vVelocity;
//
  //    // Currently, the JoltDefaultCharacterComponent will scale those values by its own speed, which makes this API unusable for this purpose
  //    plMsgMoveCharacterController msg;
  //    msg.m_fMoveForwards = plMath::Max(0.0f, vRelativeVelocity.x);
  //    msg.m_fMoveBackwards = plMath::Max(0.0f, -vRelativeVelocity.x);
  //    msg.m_fStrafeLeft = plMath::Max(0.0f, -vRelativeVelocity.y);
  //    msg.m_fStrafeRight = plMath::Max(0.0f, vRelativeVelocity.y);
//
  //    GetOwner()->SendMessage(msg);
  //  }
  //  else
  //  {
  //    m_vVelocity = vVelocity;
  //    
  //    plQuat qRotation = GetOwner()->GetGlobalRotation();
  //    if (SyncRotation(vPosition, qRotation, vVelocity, pDtAgent))
  //      GetOwner()->SetGlobalRotation(qRotation);
//
  //    plMsgTeleportObject msg;
  //    msg.m_vNewPosition = vPosition;
//
  //    GetOwner()->SendMessage(msg);
  //  }
  //}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plDetourCrowdAgentComponentManager::plDetourCrowdAgentComponentManager(plWorld* pWorld)
  : SUPER(pWorld)
{
}
plDetourCrowdAgentComponentManager::~plDetourCrowdAgentComponentManager() = default;

void plDetourCrowdAgentComponentManager::Initialize()
{
  SUPER::Initialize();

  m_pDetourCrowdModule = GetWorld()->GetOrCreateModule<plDetourCrowdWorldModule>();

  {
    auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plDetourCrowdAgentComponentManager::Update, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_fPriority = 0.0f;

    RegisterUpdateFunction(desc);
  }
}

void plDetourCrowdAgentComponentManager::Update(const plWorldModule::UpdateContext& ctx)
{
  if (!m_pDetourCrowdModule->IsInitializedAndReady())
    return;

  for (auto it = this->m_ComponentStorage.GetIterator(ctx.m_uiFirstComponentIndex, ctx.m_uiComponentCount); it.IsValid(); ++it)
  {
    plDetourCrowdAgentComponent* pAgent = it;
    const dtCrowdAgent* pDtAgent = pAgent->m_iAgentId != -1 ? m_pDetourCrowdModule->GetAgentById(pAgent->m_iAgentId) : nullptr;

    if (pAgent->IsActiveAndSimulating())
    {
      bool bTeleport = false;

      // If active and sumulating plAgent doesn't have a corresponding dtAgent, create one
      if (pDtAgent == nullptr || !pDtAgent->active || static_cast<plUInt32>(reinterpret_cast<std::uintptr_t>(pDtAgent->params.userData)) != pAgent->m_uiOwnerId)
      {
        plDetourCrowdAgentParams params = plDetourCrowdAgentParams::Default();
        pAgent->FillAgentParams(params);
        params.m_pUserData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(m_uiNextOwnerId));

        plInt32 iAgentId = m_pDetourCrowdModule->CreateAgent(pAgent->GetOwner()->GetGlobalPosition(), params);
        if (iAgentId == -1)
        {
          // Check m_uiErrorBit to prevent spamming into the log
          if (!pAgent->m_uiErrorBit)
          {
            plLog::Warning("Couldn't create DetourCrowd agent for '{0}'", pAgent->GetOwner()->GetName());
            pAgent->m_uiErrorBit = 1;
          }
          continue;
        }

        pAgent->m_iAgentId = iAgentId;
        pAgent->m_uiOwnerId = m_uiNextOwnerId;
        pAgent->m_uiErrorBit = 0;
        pAgent->m_uiParamsDirtyBit = 0;

        m_uiNextOwnerId += 1;

        pDtAgent = m_pDetourCrowdModule->GetAgentById(pAgent->m_iAgentId);

        // If dtAgent was just created, we want to teleport plAgent to its position
        bTeleport = true;
      }

      // Update dtAgent's parameters if any of the plAgent's properties (Height, Radius, etc) changed
      if (pAgent->m_uiParamsDirtyBit)
      {
        pAgent->m_uiParamsDirtyBit = 0;

        plDetourCrowdAgentParams params = plDetourCrowdAgentParams::Default();
        pAgent->FillAgentParams(params);
        params.m_pUserData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(pAgent->m_uiOwnerId));

        m_pDetourCrowdModule->UpdateAgentParams(pAgent->m_iAgentId, params);
      }

      // Sync plAgent's position with dtAgent
      pAgent->SyncTransform(pDtAgent, bTeleport);

      // Steering failed. Can only happen if plAgent uses CharacterController for movement and it got out of sync with dtAgent
      // We can do nothing about it, so clear the target and delete the dtAgent (it will be recreated at correct postion next frame)
      if (pAgent->m_uiSteeringFailedBit)
      {
        m_pDetourCrowdModule->DestroyAgent(pAgent->m_iAgentId);
        pAgent->m_iAgentId = -1;
        pAgent->m_uiSteeringFailedBit = 0;
        pAgent->m_uiTargetDirtyBit = 0;
        pAgent->m_PathToTargetState = plAgentPathFindingState::HasNoTarget;
      }

      switch (pAgent->m_PathToTargetState)
      {
        // If plAgent has no target, but dtAgent has one, clear dtAgent's target
        case plAgentPathFindingState::HasNoTarget:
          if (pDtAgent->targetState != DT_CROWDAGENT_TARGET_NONE && pDtAgent->targetState != DT_CROWDAGENT_TARGET_FAILED)
          {
            m_pDetourCrowdModule->ClearAgentTargetPosition(pAgent->m_iAgentId);

            plAgentSteeringEvent e;
            e.m_pComponent = pAgent;
            e.m_Type = plAgentSteeringEvent::TargetCleared;
            pAgent->m_SteeringEvents.Broadcast(e);
          }
          break;
        case plAgentPathFindingState::HasTargetWaitingForPath:
          // If plAgent has a target, but dtAgent has none, set target
          // Likewise, if plAgent's target has changed since the last time (m_uiTargetDirtyBit is set), set target
          if (pAgent->m_uiTargetDirtyBit || pDtAgent->targetState == DT_CROWDAGENT_TARGET_NONE || pDtAgent->targetState == DT_CROWDAGENT_TARGET_FAILED)
          {
            m_pDetourCrowdModule->SetAgentTargetPosition(pAgent->m_iAgentId, pAgent->m_vTargetPosition);
            pAgent->m_vActualTargetPosition = plRcPos(pDtAgent->targetPos);
            pAgent->m_uiTargetDirtyBit = 0;
          }
          
          // If plAgent is waiting for path status, but dtAgent has already got it, sync it and fire events
          if (pDtAgent->targetState == DT_CROWDAGENT_TARGET_VALID)
          {
            pAgent->m_PathToTargetState = plAgentPathFindingState::HasTargetAndValidPath;

            plAgentSteeringEvent e;
            e.m_pComponent = pAgent;
            e.m_Type = pDtAgent->partial ? plAgentSteeringEvent::WarningNoFullPathToTarget : plAgentSteeringEvent::PathToTargetFound;
            pAgent->m_SteeringEvents.Broadcast(e);
          }
          else if (pDtAgent->targetState == DT_CROWDAGENT_TARGET_FAILED)
          {
            pAgent->m_PathToTargetState = plAgentPathFindingState::HasTargetPathFindingFailed;

            plAgentSteeringEvent e;
            e.m_pComponent = pAgent;
            e.m_Type = plAgentSteeringEvent::ErrorNoPathToTarget;
            pAgent->m_SteeringEvents.Broadcast(e);
          }
          break;
        case plAgentPathFindingState::HasTargetPathFindingFailed:
          // If plAgent thinks pathfinding failed, but dtAgent has a valid target, clear it
          if (pDtAgent->targetState != DT_CROWDAGENT_TARGET_NONE && pDtAgent->targetState != DT_CROWDAGENT_TARGET_FAILED)
          {
            m_pDetourCrowdModule->ClearAgentTargetPosition(pAgent->m_iAgentId);
          }
          break;
        case plAgentPathFindingState::HasTargetAndValidPath:
          // If plAgent thinks it has a path, but dtAgent has none (probably because it was deleted), repeat the process
          if (pDtAgent->targetState == DT_CROWDAGENT_TARGET_NONE)
          {
            m_pDetourCrowdModule->SetAgentTargetPosition(pAgent->m_iAgentId, pAgent->m_vTargetPosition);
            pAgent->m_PathToTargetState = plAgentPathFindingState::HasTargetWaitingForPath;
            pAgent->m_vActualTargetPosition = plRcPos(pDtAgent->targetPos);
            pAgent->m_uiTargetDirtyBit = 0;
          }
          // If plAgent and dtAgent both agree they have a target and a valid path, check if target is reached
          else
          {
            const plVec3 vTargetPos = plRcPos(pDtAgent->targetPos);
            const plVec3 vDiff = vTargetPos - pAgent->GetOwner()->GetGlobalPosition();

            if (vDiff.GetLengthSquared() < pAgent->m_fStoppingDistance * pAgent->m_fStoppingDistance)
            {
              m_pDetourCrowdModule->ClearAgentTargetPosition(pAgent->m_iAgentId);

              pAgent->m_PathToTargetState = plAgentPathFindingState::HasNoTarget;
              
              plAgentSteeringEvent e;
              e.m_pComponent = pAgent;
              e.m_Type = plAgentSteeringEvent::TargetReached;
              pAgent->m_SteeringEvents.Broadcast(e);
            }
          }
          break;
        default:
          PL_ASSERT_NOT_IMPLEMENTED;
      };
    }
    else
    {
      // If plAgent is inactive, but still has a corresponding dtAgent, destroy the dtAgent
      if (pDtAgent)
      {
        // Only destroy dtAgent if it was actually owned by plAgent (could be just stale m_iAgentId)
        if (pDtAgent->active && static_cast<plUInt32>(reinterpret_cast<intptr_t>(pDtAgent->params.userData)) == pAgent->m_uiOwnerId)
        {
          m_pDetourCrowdModule->DestroyAgent(pAgent->m_iAgentId);
        }
        pAgent->m_iAgentId = -1;
        pAgent->m_uiTargetDirtyBit = 0;
        pAgent->m_uiSteeringFailedBit = 0;
        pAgent->m_PathToTargetState = plAgentPathFindingState::HasNoTarget;
      }
    }
  }
}

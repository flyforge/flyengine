#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/Stats.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <JoltPlugin/Character/JoltDefaultCharacterComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Debug/DebugRenderer.h>

plCVarBool cvar_JoltCcFootCheck("Jolt.CC.FootCheck", true, plCVarFlags::Default, "Stay down");

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltDefaultCharacterComponent, 1, plComponentMode::Dynamic)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ShapeRadius", m_fShapeRadius)->AddAttributes(new plDefaultValueAttribute(0.25f)),
    PLASMA_MEMBER_PROPERTY("CrouchHeight", m_fCylinderHeightCrouch)->AddAttributes(new plDefaultValueAttribute(0.2f), new plClampValueAttribute(0.0f, 10.0f)),
    PLASMA_MEMBER_PROPERTY("StandHeight", m_fCylinderHeightStand)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 10.0f)),
    PLASMA_MEMBER_PROPERTY("FootRadius", m_fFootRadius)->AddAttributes(new plDefaultValueAttribute(0.15f)),
    PLASMA_MEMBER_PROPERTY("WalkSpeedCrouching", m_fWalkSpeedCrouching)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 100.0f)),
    PLASMA_MEMBER_PROPERTY("WalkSpeedStanding", m_fWalkSpeedStanding)->AddAttributes(new plDefaultValueAttribute(2.5f), new plClampValueAttribute(0.0f, 100.0f)),
    PLASMA_MEMBER_PROPERTY("WalkSpeedRunning", m_fWalkSpeedRunning)->AddAttributes(new plDefaultValueAttribute(5.0f), new plClampValueAttribute(0.0f, 100.0f)),
    PLASMA_MEMBER_PROPERTY("AirSpeed", m_fAirSpeed)->AddAttributes(new plDefaultValueAttribute(2.5f), new plClampValueAttribute(0.0f, 100.0f)),
    PLASMA_MEMBER_PROPERTY("AirFriction", m_fAirFriction)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_MEMBER_PROPERTY("MaxStepUp", m_fMaxStepUp)->AddAttributes(new plDefaultValueAttribute(0.25f), new plClampValueAttribute(0.0f, 10.0f)),
    PLASMA_MEMBER_PROPERTY("MaxStepDown", m_fMaxStepDown)->AddAttributes(new plDefaultValueAttribute(0.25f), new plClampValueAttribute(0.0f, 10.0f)),
    PLASMA_MEMBER_PROPERTY("JumpImpulse", m_fJumpImpulse)->AddAttributes(new plDefaultValueAttribute(5.0f), new plClampValueAttribute(0.0f, 1000.0f)),
    PLASMA_MEMBER_PROPERTY("RotateSpeed", m_RotateSpeed)->AddAttributes(new plDefaultValueAttribute(plAngle::Degree(90.0f)), new plClampValueAttribute(plAngle::Degree(1.0f), plAngle::Degree(360.0f))),
    PLASMA_ACCESSOR_PROPERTY("WalkSurfaceInteraction", GetWalkSurfaceInteraction, SetWalkSurfaceInteraction)->AddAttributes(new plDynamicStringEnumAttribute("SurfaceInteractionTypeEnum"), new plDefaultValueAttribute(plStringView("Footstep"))),
    PLASMA_MEMBER_PROPERTY("WalkInteractionDistance", m_fWalkInteractionDistance)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("RunInteractionDistance", m_fRunInteractionDistance)->AddAttributes(new plDefaultValueAttribute(3.0f)),
    PLASMA_ACCESSOR_PROPERTY("FallbackWalkSurface", GetFallbackWalkSurfaceFile, SetFallbackWalkSurfaceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface", plDependencyFlags::Package)),
    PLASMA_ACCESSOR_PROPERTY("HeadObject", DummyGetter, SetHeadObjectReference)->AddAttributes(new plGameObjectReferenceAttribute()),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCapsuleVisualizerAttribute("StandHeight", "ShapeRadius", plColor::WhiteSmoke, nullptr, plVisualizerAnchor::NegZ),
    new plCapsuleVisualizerAttribute("CrouchHeight", "ShapeRadius", plColor::LightSlateGrey, nullptr, plVisualizerAnchor::NegZ),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgMoveCharacterController, SetInputState),
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
    PLASMA_MESSAGE_HANDLER(plMsgApplyRootMotion, OnApplyRootMotion),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_FUNCTIONS
  {
    //PLASMA_SCRIPT_FUNCTION_PROPERTY(IsDestinationUnobstructed, In, "globalFootPosition", In, "characterHeight"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(TeleportCharacter, In, "globalFootPosition"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(IsStandingOnGround),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(IsSlidingOnGround),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(IsInAir),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(IsCrouching),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plJoltDefaultCharacterComponent::plJoltDefaultCharacterComponent() = default;
plJoltDefaultCharacterComponent::~plJoltDefaultCharacterComponent() = default;

void plJoltDefaultCharacterComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(plBoundingSphere(plVec3(0, 0, GetShapeRadius()), GetShapeRadius()), plInvalidSpatialDataCategory);
  msg.AddBounds(plBoundingSphere(plVec3(0, 0, GetCurrentCapsuleHeight() - GetShapeRadius()), GetShapeRadius()), plInvalidSpatialDataCategory);
}

void plJoltDefaultCharacterComponent::OnApplyRootMotion(plMsgApplyRootMotion& msg)
{
  m_vAbsoluteRootMotion += msg.m_vTranslation;
  m_InputRotateZ += msg.m_RotationZ;
}

void plJoltDefaultCharacterComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_RotateSpeed;
  s << m_fShapeRadius;
  s << m_fCylinderHeightCrouch;
  s << m_fCylinderHeightStand;
  s << m_fWalkSpeedCrouching;
  s << m_fWalkSpeedStanding;
  s << m_fWalkSpeedRunning;
  s << m_fMaxStepUp;
  s << m_fMaxStepDown;
  s << m_fJumpImpulse;
  s << m_sWalkSurfaceInteraction;
  s << m_fWalkInteractionDistance;
  s << m_fRunInteractionDistance;
  s << m_hFallbackWalkSurface;
  s << m_fAirFriction;
  s << m_fAirSpeed;
  s << m_fFootRadius;

  inout_stream.WriteGameObjectHandle(m_hHeadObject);
}

void plJoltDefaultCharacterComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_RotateSpeed;
  s >> m_fShapeRadius;
  s >> m_fCylinderHeightCrouch;
  s >> m_fCylinderHeightStand;
  s >> m_fWalkSpeedCrouching;
  s >> m_fWalkSpeedStanding;
  s >> m_fWalkSpeedRunning;
  s >> m_fMaxStepUp;
  s >> m_fMaxStepDown;
  s >> m_fJumpImpulse;
  s >> m_sWalkSurfaceInteraction;
  s >> m_fWalkInteractionDistance;
  s >> m_fRunInteractionDistance;
  s >> m_hFallbackWalkSurface;
  s >> m_fAirFriction;
  s >> m_fAirSpeed;
  s >> m_fFootRadius;

  m_hHeadObject = inout_stream.ReadGameObjectHandle();

  ResetInternalState();
}

void plJoltDefaultCharacterComponent::ResetInternalState()
{
  m_fShapeRadius = plMath::Clamp(m_fShapeRadius, 0.05f, 5.0f);
  m_fFootRadius = plMath::Clamp(m_fFootRadius, 0.01f, m_fShapeRadius);
  m_fCylinderHeightCrouch = plMath::Max(m_fCylinderHeightCrouch, 0.01f);
  m_fCylinderHeightStand = plMath::Max(m_fCylinderHeightStand, m_fCylinderHeightCrouch);
  m_fMaxStepUp = plMath::Clamp(m_fMaxStepUp, 0.0f, m_fCylinderHeightStand);
  m_fMaxStepDown = plMath::Clamp(m_fMaxStepDown, 0.0f, m_fCylinderHeightStand);

  m_fNextCylinderHeight = m_fCylinderHeightStand;
  m_fCurrentCylinderHeight = m_fNextCylinderHeight;

  m_vVelocityLateral.SetZero();
  m_fVelocityUp = 0;

  m_fAccumulatedWalkDistance = 0;
}

void plJoltDefaultCharacterComponent::ResetInputState()
{
  m_vInputDirection.SetZero();
  m_InputRotateZ = plAngle();
  m_uiInputCrouchBit = 0;
  m_uiInputRunBit = 0;
  m_uiInputJumpBit = 0;
  m_vAbsoluteRootMotion.SetZero();
}

void plJoltDefaultCharacterComponent::SetInputState(plMsgMoveCharacterController& ref_msg)
{
  const float fDistanceToMove = plMath::Max(plMath::Abs((float)(ref_msg.m_fMoveForwards - ref_msg.m_fMoveBackwards)), plMath::Abs((float)(ref_msg.m_fStrafeRight - ref_msg.m_fStrafeLeft)));

  m_vInputDirection += plVec2((float)(ref_msg.m_fMoveForwards - ref_msg.m_fMoveBackwards), (float)(ref_msg.m_fStrafeRight - ref_msg.m_fStrafeLeft));
  m_vInputDirection.NormalizeIfNotZero(plVec2::ZeroVector()).IgnoreResult();
  m_vInputDirection *= fDistanceToMove;

  m_InputRotateZ += m_RotateSpeed * (float)(ref_msg.m_fRotateRight - ref_msg.m_fRotateLeft);

  if (ref_msg.m_bRun)
  {
    m_uiInputRunBit = 1;
  }

  if (ref_msg.m_bJump)
  {
    m_uiInputJumpBit = 1;
  }

  if (ref_msg.m_bCrouch)
  {
    m_uiInputCrouchBit = 1;
  }
}

float plJoltDefaultCharacterComponent::GetCurrentCylinderHeight() const
{
  return m_fCurrentCylinderHeight;
}

float plJoltDefaultCharacterComponent::GetCurrentCapsuleHeight() const
{
  return GetCurrentCylinderHeight() + 2.0f * GetShapeRadius();
}

float plJoltDefaultCharacterComponent::GetShapeRadius() const
{
  return m_fShapeRadius;
}

void plJoltDefaultCharacterComponent::TeleportCharacter(const plVec3& vGlobalFootPosition)
{
  TeleportToPosition(vGlobalFootPosition);
}

void plJoltDefaultCharacterComponent::OnActivated()
{
  SUPER::OnActivated();

  ResetInternalState();

  GetOwner()->UpdateLocalBounds();
}

void plJoltDefaultCharacterComponent::OnDeactivated()
{
  SUPER::OnDeactivated();
}

JPH::Ref<JPH::Shape> plJoltDefaultCharacterComponent::MakeNextCharacterShape()
{
  const float fTotalCapsuleHeight = m_fNextCylinderHeight + 2.0f * GetShapeRadius();

  JPH::CapsuleShapeSettings opt;
  opt.mRadius = GetShapeRadius();
  opt.mHalfHeightOfCylinder = 0.5f * m_fNextCylinderHeight;

  JPH::RotatedTranslatedShapeSettings up;
  up.mInnerShapePtr = opt.Create().Get();
  up.mPosition = JPH::Vec3(0, 0, fTotalCapsuleHeight * 0.5f);
  up.mRotation = JPH::Quat::sFromTo(JPH::Vec3::sAxisY(), JPH::Vec3::sAxisZ());

  return up.Create().Get();
}

void plJoltDefaultCharacterComponent::SetHeadObjectReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hHeadObject = resolver(szReference, GetHandle(), "HeadObject");
}

void plJoltDefaultCharacterComponent::OnSimulationStarted()
{
  ResetInternalState();

  // creates the CC, so the next shape size must be set already
  SUPER::OnSimulationStarted();

  plGameObject* pHeadObject;
  if (!m_hHeadObject.IsInvalidated() && GetWorld()->TryGetObject(m_hHeadObject, pHeadObject))
  {
    m_fHeadHeightOffset = pHeadObject->GetLocalPosition().z;
    m_fHeadTargetHeight = m_fHeadHeightOffset;
  }
}

void plJoltDefaultCharacterComponent::ApplyRotationZ()
{
  if (m_InputRotateZ.GetRadian() == 0.0f)
    return;

  plQuat qRotZ;
  qRotZ.SetFromAxisAndAngle(plVec3(0, 0, 1), m_InputRotateZ);
  m_InputRotateZ.SetRadian(0.0);

  GetOwner()->SetGlobalRotation(qRotZ * GetOwner()->GetGlobalRotation());
}

void plJoltDefaultCharacterComponent::SetFallbackWalkSurfaceFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hFallbackWalkSurface = plResourceManager::LoadResource<plSurfaceResource>(szFile);
  }

  if (m_hFallbackWalkSurface.IsValid())
    plResourceManager::PreloadResource(m_hFallbackWalkSurface);
}

const char* plJoltDefaultCharacterComponent::GetFallbackWalkSurfaceFile() const
{
  if (!m_hFallbackWalkSurface.IsValid())
    return "";

  return m_hFallbackWalkSurface.GetResourceID();
}

void plJoltDefaultCharacterComponent::ApplyCrouchState()
{
  if (m_uiInputCrouchBit == m_uiIsCrouchingBit)
    return;

  m_fNextCylinderHeight = m_uiInputCrouchBit ? m_fCylinderHeightCrouch : m_fCylinderHeightStand;

  if (TryChangeShape(MakeNextCharacterShape().GetPtr()).Succeeded())
  {
    m_uiIsCrouchingBit = m_uiInputCrouchBit;
    m_fCurrentCylinderHeight = m_fNextCylinderHeight;
  }
}

void plJoltDefaultCharacterComponent::StoreLateralVelocity()
{
  const plVec3 endPosition = GetOwner()->GetGlobalPosition();
  const plVec3 vVelocity = (endPosition - m_PreviousTransform.m_vPosition) * GetInverseUpdateTimeDelta();

  m_vVelocityLateral.Set(vVelocity.x, vVelocity.y);
}

void plJoltDefaultCharacterComponent::ClampLateralVelocity()
{
  const plVec3 endPosition = GetOwner()->GetGlobalPosition();
  const plVec3 vVelocity = (endPosition - m_PreviousTransform.m_vPosition) * GetInverseUpdateTimeDelta();

  plVec2 vRealDirLateral(vVelocity.x, vVelocity.y);

  if (!vRealDirLateral.IsZero())
  {
    vRealDirLateral.Normalize();

    const float fSpeedAlongRealDir = vRealDirLateral.Dot(m_vVelocityLateral);

    m_vVelocityLateral = vRealDirLateral * fSpeedAlongRealDir;
  }
  else
    m_vVelocityLateral.SetZero();
}

void plJoltDefaultCharacterComponent::InteractWithSurfaces(const ContactPoint& contact, const Config& cfg)
{
  if (cfg.m_sGroundInteraction.IsEmpty())
  {
    m_fAccumulatedWalkDistance = 0;
    return;
  }

  const plVec2 vIntendedWalkAmount = (cfg.m_vVelocity * GetUpdateTimeDelta()).GetAsVec2() + m_vAbsoluteRootMotion.GetAsVec2();

  const plVec3 vOldPos = m_PreviousTransform.m_vPosition;
  const plVec3 vNewPos = GetOwner()->GetGlobalPosition();

  m_fAccumulatedWalkDistance += plMath::Min(vIntendedWalkAmount.GetLength(), (vNewPos - vOldPos).GetLength());

  if (m_fAccumulatedWalkDistance < cfg.m_fGroundInteractionDistanceThreshold)
    return;

  m_fAccumulatedWalkDistance = 0.0f;

  SpawnContactInteraction(contact, cfg.m_sGroundInteraction, m_hFallbackWalkSurface);
}

void plJoltDefaultCharacterComponent::MoveHeadObject()
{
  plGameObject* pHeadObject;
  if (!m_hHeadObject.IsInvalidated() && GetWorld()->TryGetObject(m_hHeadObject, pHeadObject))
  {
    m_fHeadTargetHeight = m_fHeadHeightOffset;

    if (IsCrouching())
    {
      m_fHeadTargetHeight -= (m_fCylinderHeightStand - m_fCylinderHeightCrouch);
    }

    plVec3 pos = pHeadObject->GetLocalPosition();

    const float fTimeDiff = plMath::Max(GetUpdateTimeDelta(), 0.005f); // prevent stuff from breaking at high frame rates
    const float fFactor = 1.0f - plMath::Pow(0.001f, fTimeDiff);
    pos.z = plMath::Lerp(pos.z, m_fHeadTargetHeight, fFactor);

    pHeadObject->SetLocalPosition(pos);
  }
}

void plJoltDefaultCharacterComponent::DebugVisualizations()
{
  if (m_DebugFlags.IsSet(plJoltCharacterDebugFlags::PrintState))
  {
    switch (GetJoltCharacter()->GetGroundState())
    {
      case JPH::CharacterBase::EGroundState::OnGround:
        plDebugRenderer::DrawInfoText(GetWorld(), plDebugTextPlacement::TopLeft, "JCC", "Jolt: On Ground", plColor::Brown);
        break;
      case JPH::CharacterBase::EGroundState::InAir:
        plDebugRenderer::DrawInfoText(GetWorld(), plDebugTextPlacement::TopLeft, "JCC", "Jolt: In Air", plColor::CornflowerBlue);
        break;
      case JPH::CharacterBase::EGroundState::NotSupported:
        plDebugRenderer::DrawInfoText(GetWorld(), plDebugTextPlacement::TopLeft, "JCC", "Jolt: Not Supported", plColor::Yellow);
        break;
      case JPH::CharacterBase::EGroundState::OnSteepGround:
        plDebugRenderer::DrawInfoText(GetWorld(), plDebugTextPlacement::TopLeft, "JCC", "Jolt: Steep", plColor::OrangeRed);
        break;
    }

    // const plTransform newTransform = GetOwner()->GetGlobalTransform();
    // const float fDistTraveled = (m_PreviousTransform.m_vPosition - newTransform.m_vPosition).GetLength();
    // const float fSpeedTraveled = fDistTraveled * GetInverseUpdateTimeDelta();
    // const float fSpeedTraveledLateral = fDistTraveled * GetInverseUpdateTimeDelta();
    // plDebugRenderer::DrawInfoText(GetWorld(), plDebugTextPlacement::TopLeft, "JCC", plFmt("Speed 1: {} m/s", fSpeedTraveled), plColor::WhiteSmoke);
    // plDebugRenderer::DrawInfoText(GetWorld(), plDebugTextPlacement::TopLeft, "JCC", plFmt("Speed 2: {} m/s", fSpeedTraveledLateral), plColor::WhiteSmoke);
  }

  if (m_DebugFlags.IsSet(plJoltCharacterDebugFlags::VisGroundContact))
  {
    plVec3 gpos = plJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundPosition());
    plVec3 gnom = plJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundNormal());

    if (!gnom.IsZero(0.01f))
    {
      plQuat rot;
      rot.SetShortestRotation(plVec3::UnitXAxis(), gnom);

      plDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.2f, plColor::ZeroColor(), plColor::Aquamarine, plTransform(gpos, rot));
    }
  }

  if (m_DebugFlags.IsSet(plJoltCharacterDebugFlags::VisShape))
  {
    plTransform shapeTrans = GetOwner()->GetGlobalTransform();

    shapeTrans.m_vPosition.z += GetCurrentCapsuleHeight() * 0.5f;

    plDebugRenderer::DrawLineCapsuleZ(GetWorld(), GetCurrentCylinderHeight(), GetShapeRadius(), plColor::CornflowerBlue, shapeTrans);
  }
}

void plJoltDefaultCharacterComponent::CheckFeet()
{
  if (!cvar_JoltCcFootCheck)
  {
    // pretend we always touch the ground
    m_bFeetOnSolidGround = true;
    return;
  }

  if (m_fFootRadius <= 0 || m_fMaxStepDown <= 0.0f)
    return;

  m_bFeetOnSolidGround = false;

  plTransform shapeTrans = GetOwner()->GetGlobalTransform();
  plQuat shapeRot;
  shapeRot.SetShortestRotation(plVec3(0, 1, 0), plVec3(0, 0, 1));

  const float radius = m_fFootRadius;
  const float halfHeight = plMath::Max(0.0f, m_fMaxStepDown - radius);

  JPH::CapsuleShape shape(halfHeight, radius);

  plHybridArray<ContactPoint, 32> contacts;
  CollectContacts(contacts, &shape, shapeTrans.m_vPosition, shapeRot, 0.01f);

  for (const auto& contact : contacts)
  {
    plVec3 gpos = contact.m_vPosition;
    plVec3 gnom = contact.m_vSurfaceNormal;

    plColor color = plColor::LightYellow;
    plQuat rot;

    if (gnom.IsZero(0.01f))
    {
      rot.SetShortestRotation(plVec3::UnitXAxis(), plVec3::UnitZAxis());
      color = plColor::OrangeRed;
    }
    else
    {
      rot.SetShortestRotation(plVec3::UnitXAxis(), gnom);

      if (gnom.Dot(plVec3::UnitZAxis()) > plMath::Cos(plAngle::Degree(40)))
      {
        m_bFeetOnSolidGround = true;
        color = plColor::GreenYellow;
      }
    }

    if (m_DebugFlags.IsAnySet(plJoltCharacterDebugFlags::VisFootCheck))
    {
      plDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.2f, plColor::ZeroColor(), color, plTransform(gpos, rot));
    }
  }

  if (m_DebugFlags.IsAnySet(plJoltCharacterDebugFlags::VisFootCheck))
  {
    plDebugRenderer::DrawLineCapsuleZ(GetWorld(), halfHeight * 2.0f, radius, plColor::YellowGreen, plTransform(shapeTrans.m_vPosition));
  }
}

void plJoltDefaultCharacterComponent::DetermineConfig(Config& out_inputs)
{
  // velocity
  {
    float fSpeed = 0;

    switch (GetGroundState())
    {
      case plJoltDefaultCharacterComponent::GroundState::OnGround:
        fSpeed = m_fWalkSpeedStanding;

        if (m_uiIsCrouchingBit)
        {
          fSpeed = m_fWalkSpeedCrouching;
        }
        else if (m_uiInputRunBit)
        {
          fSpeed = m_fWalkSpeedRunning;
        }
        break;

      case plJoltDefaultCharacterComponent::GroundState::Sliding:
        fSpeed = m_fWalkSpeedStanding;

        if (m_uiIsCrouchingBit)
        {
          fSpeed = m_fWalkSpeedCrouching;
        }
        break;

      case plJoltDefaultCharacterComponent::GroundState::InAir:
        fSpeed = m_fAirSpeed;
        break;
    }

    out_inputs.m_vVelocity = GetOwner()->GetGlobalRotation() * m_vInputDirection.GetAsVec3(0) * fSpeed;
  }

  // ground interaction
  {
    switch (GetGroundState())
    {
      case plJoltDefaultCharacterComponent::GroundState::OnGround:
        out_inputs.m_sGroundInteraction = (m_uiInputRunBit == 1) ? m_sWalkSurfaceInteraction : m_sWalkSurfaceInteraction; // TODO: run interaction
        out_inputs.m_fGroundInteractionDistanceThreshold = (m_uiInputRunBit == 1) ? m_fRunInteractionDistance : m_fWalkInteractionDistance;
        break;

      case plJoltDefaultCharacterComponent::GroundState::Sliding:
        // TODO: slide interaction
        break;

      case GroundState::InAir:
        break;
    }
  }

  out_inputs.m_bAllowCrouch = true;
  out_inputs.m_bAllowJump = (GetGroundState() == GroundState::OnGround) && !IsCrouching() && m_bFeetOnSolidGround;
  out_inputs.m_bApplyGroundVelocity = true;
  out_inputs.m_fPushDownForce = GetMass();
  out_inputs.m_fMaxStepUp = (m_bFeetOnSolidGround && !out_inputs.m_vVelocity.IsZero()) ? m_fMaxStepUp : 0.0f;
  out_inputs.m_fMaxStepDown = ((GetGroundState() == GroundState::OnGround) || (GetGroundState() == GroundState::Sliding)) && m_bFeetOnSolidGround ? m_fMaxStepDown : 0.0f;
}

void plJoltDefaultCharacterComponent::UpdateCharacter()
{
  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();
  m_PreviousTransform = GetOwner()->GetGlobalTransform();

  switch (GetJoltCharacter()->GetGroundState())
  {
    case JPH::CharacterBase::EGroundState::InAir:
    case JPH::CharacterBase::EGroundState::NotSupported:
      m_LastGroundState = GroundState::InAir;
      // TODO: filter out 'sliding' when touching a ceiling (should be 'in air')
      break;

    case JPH::CharacterBase::EGroundState::OnGround:
      m_LastGroundState = GroundState::OnGround;
      break;

    case JPH::CharacterBase::EGroundState::OnSteepGround:
      m_LastGroundState = GroundState::Sliding;
      break;
  }

  CheckFeet();

  Config cfg;
  DetermineConfig(cfg);

  ApplyCrouchState();

  if (m_uiInputJumpBit && cfg.m_bAllowJump)
  {
    m_fVelocityUp = m_fJumpImpulse;
    cfg.m_fMaxStepUp = 0;
    cfg.m_fMaxStepDown = 0;
  }

  plVec3 vGroundVelocity = plVec3::ZeroVector();

  ContactPoint groundContact;
  {
    groundContact.m_vPosition = plJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundPosition());
    groundContact.m_vContactNormal = plJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundNormal());
    groundContact.m_vSurfaceNormal = groundContact.m_vContactNormal;
    groundContact.m_BodyID = GetJoltCharacter()->GetGroundBodyID();
    groundContact.m_SubShapeID = GetJoltCharacter()->GetGroundSubShapeID();

    /*vGroundVelocity =*/GetContactVelocityAndPushAway(groundContact, cfg.m_fPushDownForce);

    // TODO: on rotating surfaces I see the same error with this value and the one returned above
    vGroundVelocity = plJoltConversionUtils::ToVec3(GetJoltCharacter()->GetGroundVelocity());
    vGroundVelocity.z = 0.0f;

    if (!cfg.m_bApplyGroundVelocity)
      vGroundVelocity.SetZero();
  }

  const bool bWasOnGround = GetJoltCharacter()->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;

  if (bWasOnGround)
  {
    m_vVelocityLateral.SetZero();
  }

  // AIR: apply 'drag' to the lateral velocity
  m_vVelocityLateral *= plMath::Pow(1.0f - m_fAirFriction, GetUpdateTimeDelta());

  plVec3 vRootVelocity = GetInverseUpdateTimeDelta() * (GetOwner()->GetGlobalRotation() * m_vAbsoluteRootMotion);

  if (!m_vVelocityLateral.IsZero())
  {
    // remove the lateral velocity component from the root motion
    // to prevent root motion being amplified when both values are active
    plVec3 vLatDir = m_vVelocityLateral.GetNormalized().GetAsVec3(0);
    float fProj = plMath::Max(0.0f, vLatDir.Dot(vRootVelocity));
    vRootVelocity -= vLatDir * fProj;
  }

  plVec3 vVelocityToApply = cfg.m_vVelocity + vGroundVelocity;
  vVelocityToApply += m_vVelocityLateral.GetAsVec3(0);
  vVelocityToApply += vRootVelocity;
  vVelocityToApply.z = m_fVelocityUp;

  RawMoveWithVelocity(vVelocityToApply, cfg.m_fMaxStepUp, cfg.m_fMaxStepDown);

  if (!cfg.m_sGroundInteraction.IsEmpty())
  {
    if (groundContact.m_vContactNormal.IsValid() && !groundContact.m_vContactNormal.IsZero(0.001f))
    {
      // TODO: sometimes the CC reports contacts with zero normals
      InteractWithSurfaces(groundContact, cfg);
    }
  }

  // retrieve the actual up velocity
  float groundVerticalVelocity = GetJoltCharacter()->GetGroundVelocity().GetZ();
  if (GetJoltCharacter()->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround // If on ground
      && (m_fVelocityUp - groundVerticalVelocity) < 0.1f)                                   // And not moving away from ground
  {
    m_fVelocityUp = groundVerticalVelocity;
  }

  if (bWasOnGround)
  {
    StoreLateralVelocity();
  }
  else
  {
    ClampLateralVelocity();
  }

  m_fVelocityUp += GetUpdateTimeDelta() * pModule->GetCharacterGravity().z;

  ApplyRotationZ();

  MoveHeadObject();

  DebugVisualizations();

  ResetInputState();
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Character_Implementation_JoltDefaultCharacterComponent);

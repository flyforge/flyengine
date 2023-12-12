#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/PathComponent.h>
#include <GameEngine/Physics/RopeSimulator.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Skeleton/Skeleton.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Components/JoltRopeComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <RendererCore/AnimationSystem/Declarations.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plJoltRopeAnchorConstraintMode, 1)
  PLASMA_ENUM_CONSTANTS(plJoltRopeAnchorConstraintMode::None, plJoltRopeAnchorConstraintMode::Point, plJoltRopeAnchorConstraintMode::Fixed, plJoltRopeAnchorConstraintMode::Cone)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_COMPONENT_TYPE(plJoltRopeComponent, 2, plComponentMode::Dynamic)
  {
    PLASMA_BEGIN_PROPERTIES
    {
      PLASMA_ACCESSOR_PROPERTY("Anchor1", DummyGetter, SetAnchor1Reference)->AddAttributes(new plGameObjectReferenceAttribute()),
      PLASMA_ACCESSOR_PROPERTY("Anchor2", DummyGetter, SetAnchor2Reference)->AddAttributes(new plGameObjectReferenceAttribute()),
      PLASMA_ENUM_ACCESSOR_PROPERTY("Anchor1Constraint", plJoltRopeAnchorConstraintMode, GetAnchor1ConstraintMode, SetAnchor1ConstraintMode),
      PLASMA_ENUM_ACCESSOR_PROPERTY("Anchor2Constraint", plJoltRopeAnchorConstraintMode, GetAnchor2ConstraintMode, SetAnchor2ConstraintMode),
      PLASMA_MEMBER_PROPERTY("Pieces", m_uiPieces)->AddAttributes(new plDefaultValueAttribute(16), new plClampValueAttribute(2, 64)),
      PLASMA_MEMBER_PROPERTY("Slack", m_fSlack)->AddAttributes(new plDefaultValueAttribute(0.3f)),
      PLASMA_MEMBER_PROPERTY("Mass", m_fTotalMass)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.1f, 1000.0f)),
      PLASMA_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new plDefaultValueAttribute(0.05f), new plClampValueAttribute(0.01f, 0.5f)),
      PLASMA_MEMBER_PROPERTY("BendStiffness", m_fBendStiffness)->AddAttributes(new plClampValueAttribute(0.0f,   plVariant())),
      PLASMA_MEMBER_PROPERTY("MaxBend", m_MaxBend)->AddAttributes(new plDefaultValueAttribute(plAngle::Degree(30)), new plClampValueAttribute(plAngle::Degree(5), plAngle::Degree(90))),
      PLASMA_MEMBER_PROPERTY("MaxTwist", m_MaxTwist)->AddAttributes(new plDefaultValueAttribute(plAngle::Degree(30)), new plClampValueAttribute(plAngle::Degree(0.01f), plAngle::Degree(90))),
      PLASMA_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
      PLASMA_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface")),
      PLASMA_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new plDefaultValueAttribute(1.0f)),
      PLASMA_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
      PLASMA_MEMBER_PROPERTY("ContinuousCollisionDetection", m_bCCD),
    }
    PLASMA_END_PROPERTIES;
    PLASMA_BEGIN_MESSAGEHANDLERS
    {
      PLASMA_MESSAGE_HANDLER(plMsgPhysicsAddForce, AddForceAtPos),
      PLASMA_MESSAGE_HANDLER(plMsgPhysicsAddImpulse, AddImpulseAtPos),
      PLASMA_MESSAGE_HANDLER(plJoltMsgDisconnectConstraints, OnJoltMsgDisconnectConstraints),
    }
    PLASMA_END_MESSAGEHANDLERS;
    PLASMA_BEGIN_ATTRIBUTES
    {
      new plCategoryAttribute("Physics/Jolt/Animation"),
    new plColorAttribute(plColorScheme::Physics),
    }
    PLASMA_END_ATTRIBUTES;
  }
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltRopeComponent::plJoltRopeComponent() = default;
plJoltRopeComponent::~plJoltRopeComponent() = default;

void plJoltRopeComponent::SetSurfaceFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hSurface = plResourceManager::LoadResource<plSurfaceResource>(szFile);
  }

  if (m_hSurface.IsValid())
    plResourceManager::PreloadResource(m_hSurface);
}

const char* plJoltRopeComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}

void plJoltRopeComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_uiCollisionLayer;
  s << m_uiPieces;
  s << m_fThickness;
  s << m_Anchor1ConstraintMode;
  s << m_Anchor2ConstraintMode;
  s << m_bSelfCollision;
  s << m_fGravityFactor;
  s << m_hSurface;
  s << m_MaxBend;
  s << m_MaxTwist;
  s << m_fBendStiffness;
  s << m_fTotalMass;
  s << m_fSlack;
  s << m_bCCD;

  inout_stream.WriteGameObjectHandle(m_hAnchor1);
  inout_stream.WriteGameObjectHandle(m_hAnchor2);
}

void plJoltRopeComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_uiCollisionLayer;
  s >> m_uiPieces;
  s >> m_fThickness;

  if (uiVersion >= 2)
  {
    s >> m_Anchor1ConstraintMode;
    s >> m_Anchor2ConstraintMode;
  }
  else
  {
    bool m_bAttachToAnchor1, m_bAttachToAnchor2;
    s >> m_bAttachToAnchor1;
    s >> m_bAttachToAnchor2;

    m_Anchor1ConstraintMode = m_bAttachToAnchor1 ? plJoltRopeAnchorConstraintMode::Point : plJoltRopeAnchorConstraintMode::None;
    m_Anchor2ConstraintMode = m_bAttachToAnchor2 ? plJoltRopeAnchorConstraintMode::Point : plJoltRopeAnchorConstraintMode::None;
  }

  s >> m_bSelfCollision;
  s >> m_fGravityFactor;
  s >> m_hSurface;
  s >> m_MaxBend;
  s >> m_MaxTwist;
  s >> m_fBendStiffness;
  s >> m_fTotalMass;
  s >> m_fSlack;
  s >> m_bCCD;

  if (uiVersion >= 2)
  {
    m_hAnchor1 = inout_stream.ReadGameObjectHandle();
  }

  m_hAnchor2 = inout_stream.ReadGameObjectHandle();
}

void plJoltRopeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  CreateRope();
}

void plJoltRopeComponent::OnActivated()
{
  UpdatePreview();
}

void plJoltRopeComponent::OnDeactivated()
{
  DestroyPhysicsShapes();

  // tell the render components, that the rope is gone
  plMsgRopePoseUpdated poseMsg;
  GetOwner()->SendMessage(poseMsg);

  SUPER::OnDeactivated();
}

const plJoltMaterial* plJoltRopeComponent::GetJoltMaterial()
{
  if (m_hSurface.IsValid())
  {
    plResourceLock<plSurfaceResource> pSurface(m_hSurface, plResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      return static_cast<const plJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  return plJoltCore::GetDefaultMaterial();
}

void plJoltRopeComponent::CreateRope()
{
  plGameObjectHandle hAnchor1 = m_hAnchor1;
  plGameObjectHandle hAnchor2 = m_hAnchor2;

  if (hAnchor1.IsInvalidated())
    hAnchor1 = GetOwner()->GetHandle();
  if (hAnchor2.IsInvalidated())
    hAnchor2 = GetOwner()->GetHandle();

  if (hAnchor1 == hAnchor2)
    return;

  const plTransform tRoot = GetOwner()->GetGlobalTransform();

  plHybridArray<plTransform, 65> nodes;
  float fPieceLength;
  if (CreateSegmentTransforms(nodes, fPieceLength, hAnchor1, hAnchor2).Failed())
    return;

  const plUInt32 numPieces = nodes.GetCount() - 1;

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  m_uiObjectFilterID = pModule->CreateObjectFilterID();

  const plJoltMaterial* pMaterial = GetJoltMaterial();

  plJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  JPH::Ref<JPH::RagdollSettings> opt = new JPH::RagdollSettings();
  opt->mSkeleton = new JPH::Skeleton();
  opt->mSkeleton->GetJoints().resize(numPieces);
  opt->mParts.resize(numPieces);

  const float fPieceMass = m_fTotalMass / numPieces;

  plStringBuilder name;

  JPH::CapsuleShapeSettings capsule;
  capsule.mRadius = m_fThickness * 0.5f;
  capsule.mHalfHeightOfCylinder = fPieceLength * 0.5f;
  capsule.mMaterial = pMaterial;
  capsule.mUserData = reinterpret_cast<plUInt64>(pUserData);

  JPH::RotatedTranslatedShapeSettings capsOffset;
  capsOffset.mInnerShapePtr = capsule.Create().Get();
  capsOffset.mPosition = JPH::Vec3(fPieceLength * 0.5f, 0, 0);
  capsOffset.mRotation = JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), plAngle::Degree(-90).GetRadian());
  capsOffset.mUserData = reinterpret_cast<plUInt64>(pUserData);

  for (plUInt32 idx = 0; idx < numPieces; ++idx)
  {
    // skeleton
    {
      auto& joint = opt->mSkeleton->GetJoint(idx);

      // set previous name as parent name
      joint.mParentName = name;

      name.Format("Link{}", idx);
      joint.mName = name;
      joint.mParentJointIndex = static_cast<plInt32>(idx) - 1;
    }

    auto& part = opt->mParts[idx];
    part.mObjectLayer = plJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, plJoltBroadphaseLayer::Rope);
    part.mGravityFactor = m_fGravityFactor;
    part.mMotionQuality = m_bCCD ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
    part.mMotionType = JPH::EMotionType::Dynamic;
    part.mPosition = plJoltConversionUtils::ToVec3(nodes[idx].m_vPosition);
    part.mRotation = plJoltConversionUtils::ToQuat(nodes[idx].m_qRotation);
    part.mUserData = reinterpret_cast<plUInt64>(pUserData);
    part.SetShape(capsOffset.Create().Get()); // shape is cached, only 1 is created
    part.mMassPropertiesOverride.mMass = fPieceMass;
    part.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    part.mRestitution = pMaterial->m_fRestitution;
    part.mFriction = pMaterial->m_fFriction;
    part.mLinearDamping = 0.1f;
    part.mAngularDamping = 0.1f;
    part.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
    part.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilterIgnoreSame()); // this is used if m_bSelfCollision is off, otherwise it gets overridden below

    if (idx > 0)
    {
      JPH::SwingTwistConstraintSettings* pConstraint = new JPH::SwingTwistConstraintSettings();
      pConstraint->mDrawConstraintSize = 0.1f;
      pConstraint->mPosition1 = plJoltConversionUtils::ToVec3(nodes[idx].m_vPosition);
      pConstraint->mPosition2 = pConstraint->mPosition1;
      pConstraint->mNormalHalfConeAngle = m_MaxBend.GetRadian();
      pConstraint->mPlaneHalfConeAngle = m_MaxBend.GetRadian();
      pConstraint->mTwistAxis1 = plJoltConversionUtils::ToVec3(nodes[idx - 1].m_qRotation * plVec3(1, 0, 0)).Normalized();
      pConstraint->mTwistAxis2 = plJoltConversionUtils::ToVec3(nodes[idx].m_qRotation * plVec3(1, 0, 0)).Normalized();
      pConstraint->mPlaneAxis1 = plJoltConversionUtils::ToVec3(nodes[idx - 1].m_qRotation * plVec3(0, 1, 0)).Normalized();
      pConstraint->mPlaneAxis2 = plJoltConversionUtils::ToVec3(nodes[idx].m_qRotation * plVec3(0, 1, 0)).Normalized();
      pConstraint->mTwistMinAngle = -m_MaxTwist.GetRadian();
      pConstraint->mTwistMaxAngle = m_MaxTwist.GetRadian();
      pConstraint->mMaxFrictionTorque = m_fBendStiffness;
      part.mToParent = pConstraint;
    }

    if ((m_Anchor1ConstraintMode != plJoltRopeAnchorConstraintMode::None && idx == 0) ||
        (m_Anchor2ConstraintMode != plJoltRopeAnchorConstraintMode::None && idx + 1 == numPieces))
    {
      // disable all collisions for the first and last rope segment
      // this prevents colliding with walls that the rope is attached to
      part.mObjectLayer = plJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, plJoltBroadphaseLayer::Query);
    }
  }

  if (m_bSelfCollision)
  {
    // overrides the group filter above to one that allows collision with itself, except for directly joined bodies
    opt->DisableParentChildCollisions();
  }

  opt->Stabilize();

  m_pRagdoll = opt->CreateRagdoll(m_uiObjectFilterID, reinterpret_cast<plUInt64>(pUserData), pModule->GetJoltSystem());
  m_pRagdoll->AddRef();
  m_pRagdoll->AddToPhysicsSystem(JPH::EActivation::Activate);

  if (m_Anchor1ConstraintMode != plJoltRopeAnchorConstraintMode::None)
  {
    m_pConstraintAnchor1 = CreateConstraint(hAnchor1, nodes[0], m_pRagdoll->GetBodyID(0).GetIndexAndSequenceNumber(), m_Anchor1ConstraintMode, m_uiAnchor1BodyID);
  }

  if (m_Anchor2ConstraintMode != plJoltRopeAnchorConstraintMode::None)
  {
    plTransform end = nodes.PeekBack();
    end.m_qRotation = -end.m_qRotation;
    m_pConstraintAnchor2 = CreateConstraint(hAnchor2, end, m_pRagdoll->GetBodyIDs().back().GetIndexAndSequenceNumber(), m_Anchor2ConstraintMode, m_uiAnchor2BodyID);
  }
}

JPH::Constraint* plJoltRopeComponent::CreateConstraint(const plGameObjectHandle& hTarget, const plTransform& pieceLoc, plUInt32 uiBodyID, plJoltRopeAnchorConstraintMode::Enum mode, plUInt32& out_uiConnectedToBodyID)
{
  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();


  JPH::BodyID bodyIDs[2] = {JPH::BodyID(JPH::BodyID::cInvalidBodyID), JPH::BodyID(uiBodyID)};

  plGameObject* pTarget = nullptr;
  if (!GetWorld()->TryGetObject(hTarget, pTarget))
    return nullptr;

  // const auto targetLoc = pTarget->GetGlobalTransform();

  plJoltDynamicActorComponent* pActor = nullptr;
  while (pTarget && !pTarget->TryGetComponentOfBaseType(pActor))
  {
    // search for a parent with a physics actor
    pTarget = pTarget->GetParent();
  }

  if (pActor)
  {
    pActor->EnsureSimulationStarted();
    out_uiConnectedToBodyID = pActor->GetJoltBodyID();
    bodyIDs[0] = JPH::BodyID(out_uiConnectedToBodyID);
  }


  // create the joint
  {
    JPH::BodyLockMultiWrite bodies(pModule->GetJoltSystem()->GetBodyLockInterface(), bodyIDs, 2);

    if (bodies.GetBody(1) == nullptr)
      return nullptr;

    JPH::Body* pAnchor = bodies.GetBody(0) != nullptr ? bodies.GetBody(0) : &JPH::Body::sFixedToWorld;

    const plVec3 vTwistAxis1 = pieceLoc.m_qRotation * plVec3(1, 0, 0);
    const plVec3 vTwistAxis2 = pieceLoc.m_qRotation * plVec3(1, 0, 0);

    const plVec3 vOrthoAxis1 = pieceLoc.m_qRotation * plVec3(0, 1, 0);
    const plVec3 vOrthoAxis2 = pieceLoc.m_qRotation * plVec3(0, 1, 0);

    JPH::Constraint* pConstraint = nullptr;

    if (mode == plJoltRopeAnchorConstraintMode::Cone)
    {
      JPH::SwingTwistConstraintSettings constraint;
      constraint.mSpace = JPH::EConstraintSpace::WorldSpace;
      constraint.mDrawConstraintSize = 0.1f;
      constraint.mPosition1 = plJoltConversionUtils::ToVec3(pieceLoc.m_vPosition);
      constraint.mPosition2 = plJoltConversionUtils::ToVec3(pieceLoc.m_vPosition);
      constraint.mNormalHalfConeAngle = m_MaxBend.GetRadian();
      constraint.mPlaneHalfConeAngle = m_MaxBend.GetRadian();

      constraint.mTwistAxis1 = plJoltConversionUtils::ToVec3(vTwistAxis1).Normalized();
      constraint.mTwistAxis2 = plJoltConversionUtils::ToVec3(vTwistAxis2).Normalized();

      constraint.mPlaneAxis1 = plJoltConversionUtils::ToVec3(vOrthoAxis1).Normalized();
      constraint.mPlaneAxis2 = plJoltConversionUtils::ToVec3(vOrthoAxis2).Normalized();

      constraint.mTwistMinAngle = -m_MaxTwist.GetRadian();
      constraint.mTwistMaxAngle = m_MaxTwist.GetRadian();
      constraint.mMaxFrictionTorque = m_fBendStiffness;

      pConstraint = constraint.Create(*pAnchor, *bodies.GetBody(1));
    }
    else if (mode == plJoltRopeAnchorConstraintMode::Point)
    {
      JPH::PointConstraintSettings constraint;
      constraint.mSpace = JPH::EConstraintSpace::WorldSpace;
      constraint.mDrawConstraintSize = 0.1f;
      constraint.mPoint1 = plJoltConversionUtils::ToVec3(pieceLoc.m_vPosition);
      constraint.mPoint2 = plJoltConversionUtils::ToVec3(pieceLoc.m_vPosition);

      pConstraint = constraint.Create(*pAnchor, *bodies.GetBody(1));
    }
    else if (mode == plJoltRopeAnchorConstraintMode::Fixed)
    {
      JPH::FixedConstraintSettings constraint;
      constraint.mSpace = JPH::EConstraintSpace::WorldSpace;
      constraint.mDrawConstraintSize = 0.1f;
      constraint.mPoint1 = plJoltConversionUtils::ToVec3(pieceLoc.m_vPosition);
      constraint.mPoint2 = plJoltConversionUtils::ToVec3(pieceLoc.m_vPosition);
      constraint.mAxisX1 = plJoltConversionUtils::ToVec3(vTwistAxis2).Normalized();
      constraint.mAxisY1 = plJoltConversionUtils::ToVec3(vOrthoAxis2).Normalized();
      constraint.mAxisX2 = plJoltConversionUtils::ToVec3(vTwistAxis2).Normalized();
      constraint.mAxisY2 = plJoltConversionUtils::ToVec3(vOrthoAxis2).Normalized();

      pConstraint = constraint.Create(*pAnchor, *bodies.GetBody(1));
    }
    else
    {
      PLASMA_ASSERT_NOT_IMPLEMENTED;
    }

    pConstraint->AddRef();
    pModule->GetJoltSystem()->AddConstraint(pConstraint);

    if (pActor)
    {
      pActor->AddConstraint(GetHandle());
    }

    return pConstraint;
  }
}

void plJoltRopeComponent::UpdatePreview()
{
  plGameObject* pAnchor1 = nullptr;
  plGameObject* pAnchor2 = nullptr;
  if (!GetWorld()->TryGetObject(m_hAnchor1, pAnchor1))
    pAnchor1 = GetOwner();
  if (!GetWorld()->TryGetObject(m_hAnchor2, pAnchor2))
    pAnchor2 = GetOwner();

  if (pAnchor1 == pAnchor2)
    return;

  plUInt32 uiHash = 0;
  plQuat rot;

  plVec3 pos = GetOwner()->GetGlobalPosition();
  uiHash = plHashingUtils::xxHash32(&pos, sizeof(plVec3), uiHash);

  pos = pAnchor1->GetGlobalPosition();
  uiHash = plHashingUtils::xxHash32(&pos, sizeof(plVec3), uiHash);
  rot = pAnchor1->GetGlobalRotation();
  uiHash = plHashingUtils::xxHash32(&rot, sizeof(plQuat), uiHash);

  pos = pAnchor2->GetGlobalPosition();
  uiHash = plHashingUtils::xxHash32(&pos, sizeof(plVec3), uiHash);
  rot = pAnchor2->GetGlobalRotation();
  uiHash = plHashingUtils::xxHash32(&rot, sizeof(plQuat), uiHash);

  uiHash = plHashingUtils::xxHash32(&m_fSlack, sizeof(float), uiHash);
  uiHash = plHashingUtils::xxHash32(&m_uiPieces, sizeof(plUInt16), uiHash);
  uiHash = plHashingUtils::xxHash32(&m_Anchor1ConstraintMode, sizeof(plJoltRopeAnchorConstraintMode::StorageType), uiHash);
  uiHash = plHashingUtils::xxHash32(&m_Anchor2ConstraintMode, sizeof(plJoltRopeAnchorConstraintMode::StorageType), uiHash);

  if (uiHash != m_uiPreviewHash)
  {
    m_uiPreviewHash = uiHash;
    SendPreviewPose();
  }
}

plResult plJoltRopeComponent::CreateSegmentTransforms(plDynamicArray<plTransform>& transforms, float& out_fPieceLength, plGameObjectHandle hAnchor1, plGameObjectHandle hAnchor2)
{
  out_fPieceLength = 0.0f;

  if (m_uiPieces == 0)
    return PLASMA_FAILURE;

  // plPathComponent* pPath;
  // if (GetOwner()->TryGetComponentOfBaseType(pPath))
  //{
  //   // generally working, but the usability is still WIP

  //  pPath->EnsureLinearizedRepresentationIsUpToDate();

  //  const float fLength = pPath->GetLinearizedRepresentationLength();

  //  if (plMath::IsZero(fLength, 0.001f))
  //    return PLASMA_FAILURE;

  //  const plTransform ownTrans = GetOwner()->GetGlobalTransform();

  //  out_fPieceLength = fLength / m_uiPieces;

  //  transforms.SetCountUninitialized(m_uiPieces + 1);

  //  plPathComponent::LinearSampler sampler;
  //  auto t0 = pPath->SampleLinearizedRepresentation(sampler);

  //  for (plUInt16 p = 0; p < m_uiPieces; ++p)
  //  {
  //    float fAddDistance = out_fPieceLength;
  //    pPath->AdvanceLinearSamplerBy(sampler, fAddDistance);
  //    const auto t1 = pPath->SampleLinearizedRepresentation(sampler);

  //    transforms[p].m_vPosition = ownTrans * t0.m_vPosition;
  //    transforms[p].m_vScale.Set(1);
  //    transforms[p].m_qRotation.SetShortestRotation(plVec3::UnitXAxis(), ownTrans.m_qRotation * (t1.m_vPosition - t0.m_vPosition).GetNormalized());

  //    t0 = t1;
  //  }

  //  transforms.PeekBack().m_vPosition = ownTrans * t0.m_vPosition;
  //  transforms.PeekBack().m_vScale.Set(1);
  //  transforms.PeekBack().m_qRotation = transforms[m_uiPieces - 1].m_qRotation;

  //  return PLASMA_SUCCESS;
  //}
  // else
  {
    const plGameObject* pAnchor1 = nullptr;
    const plGameObject* pAnchor2 = nullptr;

    if (!GetWorld()->TryGetObject(hAnchor1, pAnchor1))
      return PLASMA_FAILURE;
    if (!GetWorld()->TryGetObject(hAnchor2, pAnchor2))
      return PLASMA_FAILURE;

    const plVec3 vOrgAnchor1 = pAnchor1->GetGlobalPosition();
    const plVec3 vOrgAnchor2 = pAnchor2->GetGlobalPosition();

    plSimdVec4f vAnchor1 = plSimdConversion::ToVec3(vOrgAnchor1);
    plSimdVec4f vAnchor2 = plSimdConversion::ToVec3(vOrgAnchor2);

    const float fLength = (vAnchor2 - vAnchor1).GetLength<3>();
    if (plMath::IsZero(fLength, 0.001f))
      return PLASMA_FAILURE;

    // the rope simulation always introduces some sag,
    // (m_fSlack - 0.1f) puts the rope under additional tension to counteract the imprecise simulation
    // we could also drastically ramp up the simulation steps, but that costs way too much performance
    const float fIntendedRopeLength = fLength + fLength * (plMath::Abs(m_fSlack) - 0.1f);

    const float fPieceLength = fIntendedRopeLength / m_uiPieces;

    plUInt16 uiSimulatedPieces = m_uiPieces;

    bool bAnchor1Fixed = false;
    bool bAnchor2Fixed = false;

    if (m_Anchor1ConstraintMode == plJoltRopeAnchorConstraintMode::Cone ||
        m_Anchor1ConstraintMode == plJoltRopeAnchorConstraintMode::Fixed)
    {
      bAnchor1Fixed = true;
      vAnchor1 += plSimdConversion::ToVec3(pAnchor1->GetGlobalDirForwards()) * fPieceLength;
      --uiSimulatedPieces;
    }

    if (m_Anchor2ConstraintMode == plJoltRopeAnchorConstraintMode::Cone ||
        m_Anchor2ConstraintMode == plJoltRopeAnchorConstraintMode::Fixed)
    {
      bAnchor2Fixed = true;
      vAnchor2 += plSimdConversion::ToVec3(pAnchor2->GetGlobalDirForwards()) * fPieceLength;
      --uiSimulatedPieces;
    }

    plRopeSimulator rope;
    rope.m_bFirstNodeIsFixed = true;
    rope.m_bLastNodeIsFixed = true;
    rope.m_fDampingFactor = 0.97f;
    rope.m_fSegmentLength = fPieceLength;
    rope.m_Nodes.SetCount(uiSimulatedPieces + 1);
    rope.m_vAcceleration.Set(0, 0, plMath::Sign(m_fSlack) * -1);

    for (plUInt16 i = 0; i < uiSimulatedPieces + 1; ++i)
    {
      rope.m_Nodes[i].m_vPosition = vAnchor1 + (vAnchor2 - vAnchor1) * ((float)i / (float)uiSimulatedPieces);
      rope.m_Nodes[i].m_vPreviousPosition = rope.m_Nodes[i].m_vPosition;
    }

    rope.SimulateTillEquilibrium(0.001f, 200);

    transforms.SetCountUninitialized(m_uiPieces + 1);

    plUInt16 idx2 = 0;

    out_fPieceLength = 0.0f;

    if (bAnchor1Fixed)
    {
      out_fPieceLength += fPieceLength;

      transforms[idx2].m_vScale.Set(1);
      transforms[idx2].m_vPosition = vOrgAnchor1;
      transforms[idx2].m_qRotation = pAnchor1->GetGlobalRotation();

      ++idx2;
    }

    const float fRopeLen = rope.GetTotalLength();
    const float fRopePieceLen = fRopeLen / uiSimulatedPieces;

    plSimdVec4f p0 = rope.m_Nodes[0].m_vPosition;

    for (plUInt16 idx = 0; idx < uiSimulatedPieces; ++idx)
    {
      const plSimdVec4f p1 = rope.GetPositionAtLength((idx + 1) * fRopePieceLen);
      plSimdVec4f dir = p1 - p0;

      const plSimdFloat len = dir.GetLength<3>();
      out_fPieceLength += len;

      if (len <= 0.001f)
        dir = plSimdVec4f(1, 0, 0, 0);
      else
        dir /= len;

      transforms[idx2].m_vScale.Set(1);
      transforms[idx2].m_vPosition = plSimdConversion::ToVec3(p0);
      transforms[idx2].m_qRotation.SetShortestRotation(plVec3::UnitXAxis(), plSimdConversion::ToVec3(dir));

      ++idx2;
      p0 = p1;
    }

    {
      transforms[idx2].m_vScale.Set(1);
      transforms[idx2].m_vPosition = plSimdConversion::ToVec3(rope.m_Nodes.PeekBack().m_vPosition);
      transforms[idx2].m_qRotation = transforms[idx2 - 1].m_qRotation;

      ++idx2;
    }

    if (bAnchor2Fixed)
    {
      out_fPieceLength += fPieceLength;

      transforms[idx2].m_vScale.Set(1);
      transforms[idx2].m_vPosition = vOrgAnchor2;
      transforms[idx2].m_qRotation = pAnchor2->GetGlobalRotation();

      plVec3 dir = transforms[idx2].m_qRotation * plVec3(1, 0, 0);
      transforms[idx2].m_qRotation.SetShortestRotation(plVec3(1, 0, 0), -dir);

      // transforms[idx2].m_qRotation.Flip();
      transforms[idx2].m_qRotation.Normalize();
      transforms[idx2 - 1].m_qRotation = transforms[idx2].m_qRotation;
    }

    out_fPieceLength /= m_uiPieces;

    return PLASMA_SUCCESS;
  }
}

void plJoltRopeComponent::DestroyPhysicsShapes()
{
  if (m_pRagdoll)
  {
    plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();

    m_pRagdoll->RemoveFromPhysicsSystem();
    m_pRagdoll->Release();
    m_pRagdoll = nullptr;

    if (m_pConstraintAnchor1)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor1);
      m_pConstraintAnchor1->Release();
      m_pConstraintAnchor1 = nullptr;
      m_uiAnchor1BodyID = plInvalidIndex;
    }

    if (m_pConstraintAnchor2)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor2);
      m_pConstraintAnchor2->Release();
      m_pConstraintAnchor2 = nullptr;
      m_uiAnchor2BodyID = plInvalidIndex;
    }

    pModule->DeallocateUserData(m_uiUserDataIndex);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
  }
}

// applying forces to a ragdoll is a problem,
// since ropes have many links, things like explosions tend to apply the same force to each link
// thus multiplying the effect
// the only reliable solution seems to be to prevent too large incoming forces
// therefore a 'frame budget' is used to only apply a certain amount of force during a single frame
// this effectively ignores most forces that are applied to multiple links and just moves one or two links
constexpr float g_fMaxForce = 1.5f;

void plJoltRopeComponent::Update()
{
  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();

  m_fMaxForcePerFrame = g_fMaxForce * 2.0f;

  if (m_pRagdoll == nullptr)
    return;

  // at runtime, allow to disengage the connection
  {
    if (m_Anchor1ConstraintMode == plJoltRopeAnchorConstraintMode::None && m_pConstraintAnchor1)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor1);
      m_pConstraintAnchor1->Release();
      m_pConstraintAnchor1 = nullptr;
      m_uiAnchor1BodyID = plInvalidIndex;
      m_pRagdoll->Activate();
    }

    if (m_Anchor2ConstraintMode == plJoltRopeAnchorConstraintMode::None && m_pConstraintAnchor2)
    {
      pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor2);
      m_pConstraintAnchor2->Release();
      m_pConstraintAnchor2 = nullptr;
      m_uiAnchor2BodyID = plInvalidIndex;
      m_pRagdoll->Activate();
    }
  }

  // if (m_fWindInfluence > 0.0f)
  //{
  //   if (const plWindWorldModuleInterface* pWind = GetWorld()->GetModuleReadOnly<plWindWorldModuleInterface>())
  //   {
  //     plVec3 ropeDir = m_RopeSim.m_Nodes.PeekBack().m_vPosition - m_RopeSim.m_Nodes[0].m_vPosition;

  //    const plVec3 vWind = pWind->GetWindAt(m_RopeSim.m_Nodes.PeekBack().m_vPosition) * m_fWindInfluence;

  //    plVec3 windForce = vWind;
  //    windForce += pWind->ComputeWindFlutter(vWind, ropeDir, 10.0f, GetOwner()->GetStableRandomSeed());

  //    if (!windForce.IsZero())
  //    {
  //      // apply force to all articulation links
  //    }
  //  }
  //}

  plHybridArray<plTransform, 32> poses(plFrameAllocator::GetCurrentAllocator());
  poses.SetCountUninitialized(static_cast<plUInt32>(m_pRagdoll->GetBodyCount()) + 1);

  plMsgRopePoseUpdated poseMsg;
  poseMsg.m_LinkTransforms = poses;

  JPH::Vec3 rootPos;
  JPH::Quat rootRot;
  m_pRagdoll->GetRootTransform(rootPos, rootRot);

  plTransform rootTransform = GetOwner()->GetGlobalTransform();
  rootTransform.m_vPosition = plJoltConversionUtils::ToVec3(rootPos);

  GetOwner()->SetGlobalPosition(rootTransform.m_vPosition);

  auto& lockInterface = pModule->GetJoltSystem()->GetBodyLockInterface();

  plTransform global;
  global.m_vScale.Set(1);

  JPH::BodyLockMultiRead lock(lockInterface, m_pRagdoll->GetBodyIDs().data(), (int)m_pRagdoll->GetBodyCount());

  for (plUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
  {
    if (auto pBody = lock.GetBody(i))
    {
      global.m_vPosition = plJoltConversionUtils::ToVec3(pBody->GetPosition());
      global.m_qRotation = plJoltConversionUtils::ToQuat(pBody->GetRotation());

      poses[i].SetLocalTransform(rootTransform, global);
    }
  }

  // last pose
  {
    const plUInt32 uiLastIdx = static_cast<plUInt32>(m_pRagdoll->GetBodyCount());

    plTransform tLocal;
    tLocal.SetIdentity();
    tLocal.m_vPosition.x = (poses[uiLastIdx - 1].m_vPosition - poses[uiLastIdx - 2].m_vPosition).GetLength();

    poses.PeekBack().SetGlobalTransform(poses[uiLastIdx - 1], tLocal);
  }

  GetOwner()->SendMessage(poseMsg);
}

void plJoltRopeComponent::SendPreviewPose()
{
  if (!IsActiveAndInitialized() || IsActiveAndSimulating())
    return;

  plGameObjectHandle hAnchor1 = m_hAnchor1;
  plGameObjectHandle hAnchor2 = m_hAnchor2;

  if (hAnchor1.IsInvalidated())
    hAnchor1 = GetOwner()->GetHandle();
  if (hAnchor2.IsInvalidated())
    hAnchor2 = GetOwner()->GetHandle();

  if (hAnchor1 == hAnchor2)
    return;

  plDynamicArray<plTransform> pieces(plFrameAllocator::GetCurrentAllocator());

  plMsgRopePoseUpdated poseMsg;
  float fPieceLength;
  if (CreateSegmentTransforms(pieces, fPieceLength, hAnchor1, hAnchor2).Succeeded())
  {
    poseMsg.m_LinkTransforms = pieces;

    const plTransform tOwner = GetOwner()->GetGlobalTransform();

    for (auto& n : pieces)
    {
      n.SetLocalTransform(tOwner, n);
    }
  }

  GetOwner()->PostMessage(poseMsg, plTime::Zero(), plObjectMsgQueueType::AfterInitialized);
}

void plJoltRopeComponent::SetGravityFactor(float fGravity)
{
  if (m_fGravityFactor == fGravity)
    return;

  m_fGravityFactor = fGravity;

  if (!m_pRagdoll)
    return;

  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();

  for (plUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
  {
    pModule->GetJoltSystem()->GetBodyInterface().SetGravityFactor(m_pRagdoll->GetBodyID(i), m_fGravityFactor);
  }

  m_pRagdoll->Activate();
}

void plJoltRopeComponent::SetAnchor1Reference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor1(resolver(szReference, GetHandle(), "Anchor1"));
}

void plJoltRopeComponent::SetAnchor2Reference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor2(resolver(szReference, GetHandle(), "Anchor2"));
}

void plJoltRopeComponent::SetAnchor1(plGameObjectHandle hActor)
{
  m_hAnchor1 = hActor;
}

void plJoltRopeComponent::SetAnchor2(plGameObjectHandle hActor)
{
  m_hAnchor2 = hActor;
}

void plJoltRopeComponent::AddForceAtPos(plMsgPhysicsAddForce& ref_msg)
{
  if (m_pRagdoll == nullptr || m_fMaxForcePerFrame <= 0.0f)
    return;

  JPH::BodyID bodyId;

  if (ref_msg.m_pInternalPhysicsActor != nullptr)
    bodyId = JPH::BodyID(reinterpret_cast<size_t>(ref_msg.m_pInternalPhysicsActor) & 0xFFFFFFFF);
  else
    bodyId = m_pRagdoll->GetBodyID(0);

  plVec3 vImp = ref_msg.m_vForce;
  const float fOrgImp = vImp.GetLength();

  if (fOrgImp > g_fMaxForce)
  {
    vImp.SetLength(g_fMaxForce).IgnoreResult();
    m_fMaxForcePerFrame -= g_fMaxForce;
  }
  else
  {
    m_fMaxForcePerFrame -= fOrgImp;
  }

  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();
  pModule->GetJoltSystem()->GetBodyInterface().AddForce(bodyId, plJoltConversionUtils::ToVec3(vImp), plJoltConversionUtils::ToVec3(ref_msg.m_vGlobalPosition));
}

void plJoltRopeComponent::AddImpulseAtPos(plMsgPhysicsAddImpulse& ref_msg)
{
  if (m_pRagdoll == nullptr || m_fMaxForcePerFrame <= 0.0f)
    return;

  JPH::BodyID bodyId;

  if (ref_msg.m_pInternalPhysicsActor != nullptr)
    bodyId = JPH::BodyID(reinterpret_cast<size_t>(ref_msg.m_pInternalPhysicsActor) & 0xFFFFFFFF);
  else
    bodyId = m_pRagdoll->GetBodyID(0);

  plVec3 vImp = ref_msg.m_vImpulse;
  const float fOrgImp = vImp.GetLength();

  if (fOrgImp > g_fMaxForce)
  {
    vImp.SetLength(g_fMaxForce).IgnoreResult();
    m_fMaxForcePerFrame -= g_fMaxForce;
  }
  else
  {
    m_fMaxForcePerFrame -= fOrgImp;
  }

  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();
  pModule->GetJoltSystem()->GetBodyInterface().AddImpulse(bodyId, plJoltConversionUtils::ToVec3(vImp), plJoltConversionUtils::ToVec3(ref_msg.m_vGlobalPosition));
}

void plJoltRopeComponent::SetAnchor1ConstraintMode(plEnum<plJoltRopeAnchorConstraintMode> mode)
{
  if (m_Anchor1ConstraintMode == mode)
    return;

  m_Anchor1ConstraintMode = mode;

  if (mode == plJoltRopeAnchorConstraintMode::None && m_pConstraintAnchor1)
  {
    m_pRagdoll->Activate();
  }
}

void plJoltRopeComponent::SetAnchor2ConstraintMode(plEnum<plJoltRopeAnchorConstraintMode> mode)
{
  if (m_Anchor2ConstraintMode == mode)
    return;

  m_Anchor2ConstraintMode = mode;

  if (mode == plJoltRopeAnchorConstraintMode::None && m_pConstraintAnchor2)
  {
    m_pRagdoll->Activate();
  }
}

void plJoltRopeComponent::OnJoltMsgDisconnectConstraints(plJoltMsgDisconnectConstraints& msg)
{
  plGameObjectHandle hBody = msg.m_pActor->GetOwner()->GetHandle();
  plWorld* pWorld = GetWorld();

  if (m_pConstraintAnchor1 && msg.m_uiJoltBodyID == m_uiAnchor1BodyID)
  {
    plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
    pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor1);
    m_pConstraintAnchor1->Release();
    m_pConstraintAnchor1 = nullptr;
    m_uiAnchor1BodyID = plInvalidIndex;
  }

  if (m_pConstraintAnchor2 && msg.m_uiJoltBodyID == m_uiAnchor2BodyID)
  {
    plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();

    pModule->GetJoltSystem()->RemoveConstraint(m_pConstraintAnchor2);
    m_pConstraintAnchor2->Release();
    m_pConstraintAnchor2 = nullptr;
    m_uiAnchor2BodyID = plInvalidIndex;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plJoltRopeComponentManager::plJoltRopeComponentManager(plWorld* pWorld)
  : plComponentManager(pWorld)
{
}

plJoltRopeComponentManager::~plJoltRopeComponentManager() = default;

void plJoltRopeComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plJoltRopeComponentManager::Update, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = false;

    this->RegisterUpdateFunction(desc);
  }
}

void plJoltRopeComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  PLASMA_PROFILE_SCOPE("UpdateRopes");

  if (!GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActiveAndInitialized())
      {
        it->UpdatePreview();
      }
    }

    return;
  }

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();

  for (auto itActor : pModule->GetActiveRopes())
  {
    itActor.Key()->Update();
  }
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltRopeComponent);

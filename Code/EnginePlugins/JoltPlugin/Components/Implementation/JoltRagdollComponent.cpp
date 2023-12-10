#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Skeleton/Skeleton.h>
#include <JoltPlugin/Components/JoltRagdollComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <Physics/Collision/Shape/CompoundShape.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRenderer.h>

/* TODO

 * prevent crashes with zero bodies
 * import sphere/box/capsule shapes

  * external constraints
 * max force clamping / point vs area impulse ?
 * communication with anim controller
 * drive to pose
 */

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plJoltRagdollStartMode, 1)
  PLASMA_ENUM_CONSTANTS(plJoltRagdollStartMode::WithBindPose, plJoltRagdollStartMode::WithNextAnimPose, plJoltRagdollStartMode::WithCurrentMeshPose)
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltRagdollComponent, 2, plComponentMode::Dynamic)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
    PLASMA_ENUM_ACCESSOR_PROPERTY("StartMode", plJoltRagdollStartMode, GetStartMode, SetStartMode),
    PLASMA_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("Mass", m_fMass)->AddAttributes(new plDefaultValueAttribute(50.0f)),
    PLASMA_MEMBER_PROPERTY("StiffnessFactor", m_fStiffnessFactor)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("OwnerVelocityScale", m_fOwnerVelocityScale)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("CenterPosition", m_vCenterPosition),
    PLASMA_MEMBER_PROPERTY("CenterVelocity", m_fCenterVelocity)->AddAttributes(new plDefaultValueAttribute(0.0f)),
    PLASMA_MEMBER_PROPERTY("CenterAngularVelocity", m_fCenterAngularVelocity)->AddAttributes(new plDefaultValueAttribute(0.0f)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    PLASMA_MESSAGE_HANDLER(plMsgAnimationPoseProposal, OnMsgAnimationPoseProposal),
    PLASMA_MESSAGE_HANDLER(plMsgRetrieveBoneState, OnRetrieveBoneState),
    PLASMA_MESSAGE_HANDLER(plMsgPhysicsAddImpulse, OnMsgPhysicsAddImpulse),
    PLASMA_MESSAGE_HANDLER(plMsgPhysicsAddForce, OnMsgPhysicsAddForce),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Physics/Jolt/Animation"),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetObjectFilterID),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(SetInitialImpulse, In, "vWorldPosition", In, "vWorldDirectionAndStrength"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(AddInitialImpulse, In, "vWorldPosition", In, "vWorldDirectionAndStrength"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(SetJointTypeOverride, In, "JointName", In, "OverrideType"),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

PLASMA_DEFINE_AS_POD_TYPE(JPH::Vec3);

//////////////////////////////////////////////////////////////////////////

plJoltRagdollComponentManager::plJoltRagdollComponentManager(plWorld* pWorld)
  : plComponentManager<plJoltRagdollComponent, plBlockStorageType::FreeList>(pWorld)
{
}

plJoltRagdollComponentManager::~plJoltRagdollComponentManager() = default;

void plJoltRagdollComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plJoltRagdollComponentManager::Update, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = false;

    this->RegisterUpdateFunction(desc);
  }
}

void plJoltRagdollComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  PLASMA_PROFILE_SCOPE("UpdateRagdolls");

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();

  for (auto it : pModule->GetActiveRagdolls())
  {
    plJoltRagdollComponent* pComponent = it.Key();

    pComponent->Update(false);
  }

  for (plJoltRagdollComponent* pComponent : pModule->GetRagdollsPutToSleep())
  {
    pComponent->Update(true);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plJoltRagdollComponent::plJoltRagdollComponent() = default;
plJoltRagdollComponent::~plJoltRagdollComponent() = default;

void plJoltRagdollComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_StartMode;
  s << m_fGravityFactor;
  s << m_bSelfCollision;
  s << m_fOwnerVelocityScale;
  s << m_fCenterVelocity;
  s << m_fCenterAngularVelocity;
  s << m_vCenterPosition;
  s << m_fMass;
  s << m_fStiffnessFactor;
}

void plJoltRagdollComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  if (uiVersion < 2)
    return;

  s >> m_StartMode;
  s >> m_fGravityFactor;
  s >> m_bSelfCollision;
  s >> m_fOwnerVelocityScale;
  s >> m_fCenterVelocity;
  s >> m_fCenterAngularVelocity;
  s >> m_vCenterPosition;
  s >> m_fMass;
  s >> m_fStiffnessFactor;
}

void plJoltRagdollComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_StartMode == plJoltRagdollStartMode::WithBindPose)
  {
    CreateLimbsFromBindPose();
  }
  if (m_StartMode == plJoltRagdollStartMode::WithCurrentMeshPose)
  {
    CreateLimbsFromCurrentMeshPose();
  }
}

void plJoltRagdollComponent::OnDeactivated()
{
  DestroyAllLimbs();

  SUPER::OnDeactivated();
}

void plJoltRagdollComponent::Update(bool bForce)
{
  if (!HasCreatedLimbs())
    return;

  UpdateOwnerPosition();

  const plVisibilityState visState = GetOwner()->GetVisibilityState();
  if (!bForce && visState != plVisibilityState::Direct)
  {
    m_ElapsedTimeSinceUpdate += plClock::GetGlobalClock()->GetTimeDiff();

    if (visState == plVisibilityState::Indirect && m_ElapsedTimeSinceUpdate < plTime::MakeFromMilliseconds(200))
    {
      // when the ragdoll is only visible by shadows or reflections, update it infrequently
      return;
    }

    if (visState == plVisibilityState::Invisible && m_ElapsedTimeSinceUpdate < plTime::MakeFromMilliseconds(500))
    {
      // when the ragdoll is entirely invisible, update it very rarely
      return;
    }
  }

  RetrieveRagdollPose();
  SendAnimationPoseMsg();

  m_ElapsedTimeSinceUpdate = plTime::MakeZero();
}

plResult plJoltRagdollComponent::EnsureSkeletonIsKnown()
{
  if (!m_hSkeleton.IsValid())
  {
    plMsgQueryAnimationSkeleton msg;
    GetOwner()->SendMessage(msg);
    m_hSkeleton = msg.m_hSkeleton;
  }

  if (!m_hSkeleton.IsValid())
  {
    plLog::Error("No skeleton available for ragdoll on object '{}'.", GetOwner()->GetName());
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

bool plJoltRagdollComponent::HasCreatedLimbs() const
{
  return m_pRagdoll != nullptr;
}

void plJoltRagdollComponent::CreateLimbsFromBindPose()
{
  DestroyAllLimbs();

  if (EnsureSkeletonIsKnown().Failed())
    return;

  plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::BlockTillLoaded);
  const auto& desc = pSkeleton->GetDescriptor();

  m_CurrentLimbTransforms.SetCountUninitialized(desc.m_Skeleton.GetJointCount());

  auto ComputeFullJointTransform = [&](plUInt32 uiJointIdx, auto self) -> plMat4 {
    const auto& joint = desc.m_Skeleton.GetJointByIndex(uiJointIdx);
    const plMat4 jointTransform = joint.GetRestPoseLocalTransform().GetAsMat4();

    if (joint.GetParentIndex() != plInvalidJointIndex)
    {
      const plMat4 parentTransform = self(joint.GetParentIndex(), self);

      return parentTransform * jointTransform;
    }

    return jointTransform;
  };

  for (plUInt32 i = 0; i < m_CurrentLimbTransforms.GetCount(); ++i)
  {
    m_CurrentLimbTransforms[i] = ComputeFullJointTransform(i, ComputeFullJointTransform);
  }

  plMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &desc.m_Skeleton;
  msg.m_ModelTransforms = m_CurrentLimbTransforms;

  CreateLimbsFromPose(msg);
}

void plJoltRagdollComponent::CreateLimbsFromCurrentMeshPose()
{
  DestroyAllLimbs();

  if (EnsureSkeletonIsKnown().Failed())
    return;

  plAnimatedMeshComponent* pMesh = nullptr;
  if (!GetOwner()->TryGetComponentOfBaseType<plAnimatedMeshComponent>(pMesh))
    return;

  plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::BlockTillLoaded_NeverFail);

  plTransform tRoot;
  pMesh->RetrievePose(m_CurrentLimbTransforms, tRoot, pSkeleton->GetDescriptor().m_Skeleton);

  plMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &tRoot;
  msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
  msg.m_ModelTransforms = m_CurrentLimbTransforms;

  CreateLimbsFromPose(msg);
}

void plJoltRagdollComponent::DestroyAllLimbs()
{
  if (m_pRagdoll)
  {
    m_pRagdoll->RemoveFromPhysicsSystem();
    m_pRagdoll->Release();
    m_pRagdoll = nullptr;
  }

  if (plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>())
  {
    pModule->DeallocateUserData(m_uiJoltUserDataIndex);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
    m_pJoltUserData = nullptr;
  }

  m_CurrentLimbTransforms.Clear();
  m_Limbs.Clear();
}

void plJoltRagdollComponent::SetGravityFactor(float fFactor)
{
  if (m_fGravityFactor == fFactor)
    return;

  m_fGravityFactor = fFactor;

  if (!m_pRagdoll)
    return;

  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();

  for (plUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
  {
    pModule->GetJoltSystem()->GetBodyInterface().SetGravityFactor(m_pRagdoll->GetBodyID(i), m_fGravityFactor);
  }

  m_pRagdoll->Activate();
}

void plJoltRagdollComponent::SetStartMode(plEnum<plJoltRagdollStartMode> mode)
{
  if (m_StartMode == mode)
    return;

  m_StartMode = mode;
}

void plJoltRagdollComponent::OnMsgPhysicsAddImpulse(plMsgPhysicsAddImpulse& ref_msg)
{
  if (!HasCreatedLimbs())
  {
    m_vInitialImpulsePosition += ref_msg.m_vGlobalPosition;
    m_vInitialImpulseDirection += ref_msg.m_vImpulse;
    m_uiNumInitialImpulses++;
    return;
  }

  JPH::BodyID bodyId = JPH::BodyID(reinterpret_cast<size_t>(ref_msg.m_pInternalPhysicsActor) & 0xFFFFFFFF);
  if (!bodyId.IsInvalid())
  {
    auto pBodies = &GetWorld()->GetModule<plJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
    pBodies->AddImpulse(bodyId, plJoltConversionUtils::ToVec3(ref_msg.m_vImpulse), plJoltConversionUtils::ToVec3(ref_msg.m_vGlobalPosition));
  }
}

void plJoltRagdollComponent::OnMsgPhysicsAddForce(plMsgPhysicsAddForce& ref_msg)
{
  if (!HasCreatedLimbs())
    return;

  JPH::BodyID bodyId = JPH::BodyID(reinterpret_cast<size_t>(ref_msg.m_pInternalPhysicsActor) & 0xFFFFFFFF);
  if (!bodyId.IsInvalid())
  {
    auto pBodies = &GetWorld()->GetModule<plJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
    pBodies->AddForce(bodyId, plJoltConversionUtils::ToVec3(ref_msg.m_vForce), plJoltConversionUtils::ToVec3(ref_msg.m_vGlobalPosition));
  }
}

void plJoltRagdollComponent::SetInitialImpulse(const plVec3& vPosition, const plVec3& vDirectionAndStrength)
{
  if (vDirectionAndStrength.IsZero())
  {
    m_vInitialImpulsePosition.SetZero();
    m_vInitialImpulseDirection.SetZero();
    m_uiNumInitialImpulses = 0;
  }
  else
  {
    m_vInitialImpulsePosition = vPosition;
    m_vInitialImpulseDirection = vDirectionAndStrength;
    m_uiNumInitialImpulses = 1;
  }
}

void plJoltRagdollComponent::AddInitialImpulse(const plVec3& vPosition, const plVec3& vDirectionAndStrength)
{
  m_vInitialImpulsePosition += vPosition;
  m_vInitialImpulseDirection += vDirectionAndStrength;
  m_uiNumInitialImpulses++;
}

void plJoltRagdollComponent::SetJointTypeOverride(plStringView sJointName, plEnum<plSkeletonJointType> type)
{
  const plTempHashedString sJointNameHashed(sJointName);

  for (plUInt32 i = 0; i < m_JointOverrides.GetCount(); ++i)
  {
    if (m_JointOverrides[i].m_sJointName == sJointNameHashed)
    {
      m_JointOverrides[i].m_JointType = type;
      m_JointOverrides[i].m_bOverrideType = true;
      return;
    }
  }

  auto& jo = m_JointOverrides.ExpandAndGetRef();
  jo.m_sJointName = sJointNameHashed;
  jo.m_JointType = type;
  jo.m_bOverrideType = true;
}

void plJoltRagdollComponent::OnMsgAnimationPoseProposal(plMsgAnimationPoseProposal& ref_poseMsg)
{
  if (!IsActiveAndSimulating() || !HasCreatedLimbs())
    return;

  // ref_poseMsg.m_bContinueAnimating = false;

  // JPH::SkeletonPose pose;
  // pose.SetSkeleton(m_pRagdoll->GetRagdollSettings()->GetSkeleton());

  // for (plUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  //{
  //   if (m_Limbs[uiLimbIdx].m_uiPartIndex == plInvalidJointIndex)
  //   {
  //     // no need to do anything, just pass the original pose through
  //     continue;
  //   }

  //  const plMat4 srcMat = ref_poseMsg.m_ModelTransforms[uiLimbIdx];

  //  // TODO: add global transform to every bone

  //  JPH::Mat44& dstMat = pose.GetJointMatrix(m_Limbs[uiLimbIdx].m_uiPartIndex);

  //  memcpy(&dstMat, &srcMat, sizeof(plMat4));

  //  // const plTransform limbGlobalPose = plJoltConversionUtils::ToTransform(bodyRead.GetBody().GetPosition(), bodyRead.GetBody().GetRotation());
  //  // m_CurrentLimbTransforms[uiLimbIdx] = (mInv * limbGlobalPose.GetAsMat4()) * scale;
  //}

  // pose.CalculateJointStates();
  // m_pRagdoll->DriveToPoseUsingKinematics(pose, 1.0f / 30.0f);
}

void plJoltRagdollComponent::OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& ref_poseMsg)
{
  if (!IsActiveAndSimulating())
    return;

  if (HasCreatedLimbs())
  {
    ref_poseMsg.m_bContinueAnimating = false; // TODO: change this

    // TODO: if at some point we can layer ragdolls with detail animations, we should
    // take poses for all bones for which there are no shapes (link == null) -> to animate leafs (fingers and such)
    return;
  }

  if (m_StartMode != plJoltRagdollStartMode::WithNextAnimPose)
    return;

  m_CurrentLimbTransforms = ref_poseMsg.m_ModelTransforms;

  CreateLimbsFromPose(ref_poseMsg);
}

void plJoltRagdollComponent::OnRetrieveBoneState(plMsgRetrieveBoneState& ref_msg) const
{
  if (!HasCreatedLimbs())
    return;

  plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::BlockTillLoaded);
  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  for (plUInt32 uiJointIdx = 0; uiJointIdx < skeleton.GetJointCount(); ++uiJointIdx)
  {
    plMat4 mJoint = m_CurrentLimbTransforms[uiJointIdx];

    const auto& joint = skeleton.GetJointByIndex(uiJointIdx);
    const plUInt16 uiParentIdx = joint.GetParentIndex();
    if (uiParentIdx != plInvalidJointIndex)
    {
      // remove the parent transform to get the pure local transform
      const plMat4 mParent = m_CurrentLimbTransforms[uiParentIdx].GetInverse();

      mJoint = mParent * mJoint;
    }

    auto& t = ref_msg.m_BoneTransforms[joint.GetName().GetString()];
    t.m_vPosition = mJoint.GetTranslationVector();
    t.m_qRotation.ReconstructFromMat4(mJoint);
    t.m_vScale.Set(1.0f);
  }
}

void plJoltRagdollComponent::SendAnimationPoseMsg()
{
  plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::BlockTillLoaded);
  const plTransform rootTransform = pSkeleton->GetDescriptor().m_RootTransform;

  plMsgAnimationPoseUpdated poseMsg;
  poseMsg.m_ModelTransforms = m_CurrentLimbTransforms;
  poseMsg.m_pRootTransform = &rootTransform;
  poseMsg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;

  GetOwner()->SendMessage(poseMsg);
}

plTransform plJoltRagdollComponent::GetRagdollRootTransform() const
{
  JPH::Vec3 joltRootPos;
  JPH::Quat joltRootRot;
  m_pRagdoll->GetRootTransform(joltRootPos, joltRootRot);

  plTransform res = plJoltConversionUtils::ToTransform(joltRootPos, joltRootRot);
  res.m_vScale = GetOwner()->GetGlobalScaling();

  return res;
}

void plJoltRagdollComponent::UpdateOwnerPosition()
{
  GetOwner()->SetGlobalTransform(GetRagdollRootTransform() * m_RootBodyLocalTransform.GetInverse());
}

void plJoltRagdollComponent::RetrieveRagdollPose()
{
  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();

  plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::BlockTillLoaded);
  const plSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
  const plTransform rootTransform = pSkeleton->GetDescriptor().m_RootTransform;
  const plMat4 invRootTransform = rootTransform.GetAsMat4().GetInverse();
  const plMat4 mInv = invRootTransform * m_RootBodyLocalTransform.GetAsMat4() * GetRagdollRootTransform().GetInverse().GetAsMat4();

  const plVec3 vObjectScale = GetOwner()->GetGlobalScaling();
  const float fObjectScale = plMath::Max(vObjectScale.x, vObjectScale.y, vObjectScale.z);

  plMat4 scale = plMat4::MakeScaling(rootTransform.m_vScale * fObjectScale);

  plHybridArray<plMat4, 64> relativeTransforms;

  {
    // m_CurrentLimbTransforms is stored in model space
    // for bones that don't have their own shape in the ragdoll,
    // we don't get a new transform from the ragdoll, but we still must update them,
    // if there is a parent bone in the ragdoll, otherwise they don't move along as expected
    // therefore we compute their relative transform here
    // and then later we take their new parent transform (which may come from the ragdoll)
    // to set their final new transform

    for (plUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
    {
      if (m_Limbs[uiLimbIdx].m_uiPartIndex != plInvalidJointIndex)
        continue;

      const auto& joint = skeleton.GetJointByIndex(uiLimbIdx);
      const plUInt16 uiParentIdx = joint.GetParentIndex();

      if (uiParentIdx == plInvalidJointIndex)
        continue;

      const plMat4 mJoint = m_CurrentLimbTransforms[uiLimbIdx];

      // remove the parent transform to get the pure local transform
      const plMat4 mParentInv = m_CurrentLimbTransforms[uiParentIdx].GetInverse();

      relativeTransforms.PushBack(mParentInv * mJoint);
    }
  }

  plUInt32 uiNextRelativeIdx = 0;
  for (plUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  {
    if (m_Limbs[uiLimbIdx].m_uiPartIndex == plInvalidJointIndex)
    {
      const auto& joint = skeleton.GetJointByIndex(uiLimbIdx);
      const plUInt16 uiParentIdx = joint.GetParentIndex();

      if (uiParentIdx != plInvalidJointIndex)
      {
        m_CurrentLimbTransforms[uiLimbIdx] = m_CurrentLimbTransforms[uiParentIdx] * relativeTransforms[uiNextRelativeIdx];
        ++uiNextRelativeIdx;
      }
    }
    else
    {
      const JPH::BodyID bodyId = m_pRagdoll->GetBodyID(m_Limbs[uiLimbIdx].m_uiPartIndex);
      PLASMA_ASSERT_DEBUG(!bodyId.IsInvalid(), "Invalid limb -> body mapping");
      JPH::BodyLockRead bodyRead(pModule->GetJoltSystem()->GetBodyLockInterface(), bodyId);

      const plTransform limbGlobalPose = plJoltConversionUtils::ToTransform(bodyRead.GetBody().GetPosition(), bodyRead.GetBody().GetRotation());

      m_CurrentLimbTransforms[uiLimbIdx] = (mInv * limbGlobalPose.GetAsMat4()) * scale;
    }
  }
}

void plJoltRagdollComponent::CreateLimbsFromPose(const plMsgAnimationPoseUpdated& pose)
{
  PLASMA_ASSERT_DEBUG(!HasCreatedLimbs(), "Limbs are already created.");

  if (EnsureSkeletonIsKnown().Failed())
    return;

  const plVec3 vObjectScale = GetOwner()->GetGlobalScaling();
  const float fObjectScale = plMath::Max(vObjectScale.x, vObjectScale.y, vObjectScale.z);

  plJoltWorldModule& worldModule = *GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  m_uiObjectFilterID = worldModule.CreateObjectFilterID();
  m_uiJoltUserDataIndex = worldModule.AllocateUserData(m_pJoltUserData);
  m_pJoltUserData->Init(this);

  plResourceLock<plSkeletonResource> pSkeletonResource(m_hSkeleton, plResourceAcquireMode::BlockTillLoaded);

  // allocate the limbs array
  m_Limbs.SetCount(pose.m_ModelTransforms.GetCount());

  JPH::Ref<JPH::RagdollSettings> ragdollSettings = new JPH::RagdollSettings();
  PLASMA_SCOPE_EXIT(m_pRagdollSettings = nullptr);

  m_pRagdollSettings = ragdollSettings.GetPtr();
  m_pRagdollSettings->mParts.reserve(pSkeletonResource->GetDescriptor().m_Skeleton.GetJointCount());
  m_pRagdollSettings->mSkeleton = new JPH::Skeleton(); // TODO: share this in the resource
  m_pRagdollSettings->mSkeleton->GetJoints().reserve(m_pRagdollSettings->mParts.size());

  CreateAllLimbs(*pSkeletonResource.GetPointer(), pose, worldModule, fObjectScale);
  ApplyBodyMass();
  SetupLimbJoints(pSkeletonResource.GetPointer());
  ApplyPartInitialVelocity();

  if (m_bSelfCollision)
  {
    // enables collisions between all bodies except the ones that are directly connected to each other
    m_pRagdollSettings->DisableParentChildCollisions();
  }

  m_pRagdollSettings->Stabilize();

  m_pRagdoll = m_pRagdollSettings->CreateRagdoll(m_uiObjectFilterID, reinterpret_cast<plUInt64>(m_pJoltUserData), worldModule.GetJoltSystem());

  m_pRagdoll->AddRef();
  m_pRagdoll->AddToPhysicsSystem(JPH::EActivation::Activate);

  ApplyInitialImpulse(worldModule, pSkeletonResource->GetDescriptor().m_fMaxImpulse);
}

void plJoltRagdollComponent::ConfigureRagdollPart(void* pRagdollSettingsPart, const plTransform& globalTransform, plUInt8 uiCollisionLayer, plJoltWorldModule& worldModule)
{
  JPH::RagdollSettings::Part* pPart = reinterpret_cast<JPH::RagdollSettings::Part*>(pRagdollSettingsPart);

  pPart->mPosition = plJoltConversionUtils::ToVec3(globalTransform.m_vPosition);
  pPart->mRotation = plJoltConversionUtils::ToQuat(globalTransform.m_qRotation).Normalized();
  pPart->mMotionQuality = JPH::EMotionQuality::LinearCast;
  pPart->mGravityFactor = m_fGravityFactor;
  pPart->mUserData = reinterpret_cast<plUInt64>(m_pJoltUserData);
  pPart->mObjectLayer = plJoltCollisionFiltering::ConstructObjectLayer(uiCollisionLayer, plJoltBroadphaseLayer::Ragdoll);
  pPart->mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  pPart->mCollisionGroup.SetGroupFilter(worldModule.GetGroupFilterIgnoreSame()); // this is used if m_bSelfCollision is off, otherwise it gets overridden below
}

void plJoltRagdollComponent::ApplyPartInitialVelocity()
{
  JPH::Vec3 vCommonVelocity = plJoltConversionUtils::ToVec3(GetOwner()->GetLinearVelocity() * m_fOwnerVelocityScale);
  const JPH::Vec3 vCenterPos = plJoltConversionUtils::ToVec3(GetOwner()->GetGlobalTransform() * m_vCenterPosition);

  plCoordinateSystem coord;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), coord);
  plRandom& rng = GetOwner()->GetWorld()->GetRandomNumberGenerator();

  for (JPH::RagdollSettings::Part& part : m_pRagdollSettings->mParts)
  {
    part.mLinearVelocity = vCommonVelocity;

    if (m_fCenterVelocity != 0.0f)
    {
      const JPH::Vec3 vVelocityDir = (part.mPosition - vCenterPos).NormalizedOr(JPH::Vec3::sZero());
      part.mLinearVelocity += vVelocityDir * plMath::Min(part.mMaxLinearVelocity, m_fCenterVelocity);
    }

    if (m_fCenterAngularVelocity != 0.0f)
    {
      const plVec3 vVelocityDir = plJoltConversionUtils::ToVec3(part.mPosition - vCenterPos);
      plVec3 vRotationDir = vVelocityDir.CrossRH(coord.m_vUpDir);
      vRotationDir.NormalizeIfNotZero(coord.m_vUpDir).IgnoreResult();

      plVec3 vRotationAxis = plVec3::MakeRandomDeviation(rng, plAngle::MakeFromDegree(30.0f), vRotationDir);
      vRotationAxis *= rng.Bool() ? 1.0f : -1.0f;

      float fSpeed = rng.FloatVariance(m_fCenterAngularVelocity, 0.5f);
      fSpeed = plMath::Min(fSpeed, part.mMaxAngularVelocity * 0.95f);

      part.mAngularVelocity = plJoltConversionUtils::ToVec3(vRotationAxis) * fSpeed;
    }
  }
}

void plJoltRagdollComponent::ApplyInitialImpulse(plJoltWorldModule& worldModule, float fMaxImpulse)
{
  if (m_uiNumInitialImpulses == 0)
    return;

  if (m_uiNumInitialImpulses > 1)
  {
    plLog::Info("Impulses: {} - {}", m_uiNumInitialImpulses, m_vInitialImpulseDirection.GetLength());
  }

  auto pJoltSystem = worldModule.GetJoltSystem();

  m_vInitialImpulsePosition /= m_uiNumInitialImpulses;

  float fImpulse = m_vInitialImpulseDirection.GetLength();

  if (fImpulse > fMaxImpulse)
  {
    fImpulse = fMaxImpulse;
    m_vInitialImpulseDirection.SetLength(fImpulse).AssertSuccess();
  }

  const JPH::Vec3 vImpulsePosition = plJoltConversionUtils::ToVec3(m_vInitialImpulsePosition);
  float fLowestDistanceSqr = 100000;

  JPH::BodyID closestBody;

  for (plUInt32 uiBodyIdx = 0; uiBodyIdx < m_pRagdoll->GetBodyCount(); ++uiBodyIdx)
  {
    const JPH::BodyID bodyId = m_pRagdoll->GetBodyID(uiBodyIdx);
    JPH::BodyLockRead bodyRead(pJoltSystem->GetBodyLockInterface(), bodyId);

    const float fDistanceToImpulseSqr = (bodyRead.GetBody().GetPosition() - vImpulsePosition).LengthSq();

    if (fDistanceToImpulseSqr < fLowestDistanceSqr)
    {
      fLowestDistanceSqr = fDistanceToImpulseSqr;
      closestBody = bodyId;
    }
  }

  pJoltSystem->GetBodyInterface().AddImpulse(closestBody, plJoltConversionUtils::ToVec3(m_vInitialImpulseDirection), vImpulsePosition);
}


void plJoltRagdollComponent::ApplyBodyMass()
{
  if (m_fMass <= 0.0f)
    return;

  float fPartMass = m_fMass / m_pRagdollSettings->mParts.size();

  for (auto& part : m_pRagdollSettings->mParts)
  {
    part.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    part.mMassPropertiesOverride.mMass = fPartMass;
  }
}

void plJoltRagdollComponent::ComputeLimbModelSpaceTransform(plTransform& transform, const plMsgAnimationPoseUpdated& pose, plUInt32 uiPoseJointIndex)
{
  plMat4 mFullTransform;
  pose.ComputeFullBoneTransform(uiPoseJointIndex, mFullTransform, transform.m_qRotation);

  transform.m_vScale.Set(1);
  transform.m_vPosition = mFullTransform.GetTranslationVector();
}


void plJoltRagdollComponent::ComputeLimbGlobalTransform(plTransform& transform, const plMsgAnimationPoseUpdated& pose, plUInt32 uiPoseJointIndex)
{
  plTransform local;
  ComputeLimbModelSpaceTransform(local, pose, uiPoseJointIndex);
  transform = plTransform::MakeGlobalTransform(GetOwner()->GetGlobalTransform(), local);
}

void plJoltRagdollComponent::CreateAllLimbs(const plSkeletonResource& skeletonResource, const plMsgAnimationPoseUpdated& pose, plJoltWorldModule& worldModule, float fObjectScale)
{
  plMap<plUInt16, LimbConstructionInfo> limbConstructionInfos(plFrameAllocator::GetCurrentAllocator());
  limbConstructionInfos.FindOrAdd(plInvalidJointIndex); // dummy root link

  plUInt16 uiLastLimbIdx = plInvalidJointIndex;
  plHybridArray<const plSkeletonResourceGeometry*, 8> geometries;

  for (const auto& geo : skeletonResource.GetDescriptor().m_Geometry)
  {
    if (geo.m_Type == plSkeletonJointGeometryType::None)
      continue;

    if (geo.m_uiAttachedToJoint != uiLastLimbIdx)
    {
      CreateLimb(skeletonResource, limbConstructionInfos, geometries, pose, worldModule, fObjectScale);
      geometries.Clear();
      uiLastLimbIdx = geo.m_uiAttachedToJoint;
    }

    geometries.PushBack(&geo);
  }

  CreateLimb(skeletonResource, limbConstructionInfos, geometries, pose, worldModule, fObjectScale);

  // get the limb with the lowest index (ie. the first one added) as the root joint
  // and use it's transform to compute m_RootBodyLocalTransform
  m_RootBodyLocalTransform = plTransform::MakeLocalTransform(GetOwner()->GetGlobalTransform(), limbConstructionInfos.GetIterator().Value().m_GlobalTransform);
}

void plJoltRagdollComponent::CreateLimb(const plSkeletonResource& skeletonResource, plMap<plUInt16, LimbConstructionInfo>& limbConstructionInfos, plArrayPtr<const plSkeletonResourceGeometry*> geometries, const plMsgAnimationPoseUpdated& pose, plJoltWorldModule& worldModule, float fObjectScale)
{
  if (geometries.IsEmpty())
    return;

  const plSkeleton& skeleton = skeletonResource.GetDescriptor().m_Skeleton;

  const plUInt16 uiThisJointIdx = geometries[0]->m_uiAttachedToJoint;
  const plSkeletonJoint& thisLimbJoint = skeleton.GetJointByIndex(uiThisJointIdx);
  plUInt16 uiParentJointIdx = thisLimbJoint.GetParentIndex();

  // find the parent joint that is also part of the ragdoll
  while (!limbConstructionInfos.Contains(uiParentJointIdx))
  {
    uiParentJointIdx = skeleton.GetJointByIndex(uiParentJointIdx).GetParentIndex();
  }
  // now uiParentJointIdx is either the index of a limb that has been created before, or plInvalidJointIndex

  LimbConstructionInfo& thisLimbInfo = limbConstructionInfos[uiThisJointIdx];
  const LimbConstructionInfo& parentLimbInfo = limbConstructionInfos[uiParentJointIdx];

  thisLimbInfo.m_uiJoltPartIndex = (plUInt16)m_pRagdollSettings->mParts.size();
  m_pRagdollSettings->mParts.resize(m_pRagdollSettings->mParts.size() + 1);

  m_Limbs[uiThisJointIdx].m_uiPartIndex = thisLimbInfo.m_uiJoltPartIndex;

  m_pRagdollSettings->mSkeleton->AddJoint(thisLimbJoint.GetName().GetData(), parentLimbInfo.m_uiJoltPartIndex != plInvalidJointIndex ? parentLimbInfo.m_uiJoltPartIndex : -1);

  ComputeLimbGlobalTransform(thisLimbInfo.m_GlobalTransform, pose, uiThisJointIdx);
  ConfigureRagdollPart(&m_pRagdollSettings->mParts[thisLimbInfo.m_uiJoltPartIndex], thisLimbInfo.m_GlobalTransform, thisLimbJoint.GetCollisionLayer(), worldModule);
  CreateAllLimbGeoShapes(thisLimbInfo, geometries, thisLimbJoint, skeletonResource, fObjectScale);
}

JPH::Shape* plJoltRagdollComponent::CreateLimbGeoShape(const LimbConstructionInfo& limbConstructionInfo, const plSkeletonResourceGeometry& geo, const plJoltMaterial* pJoltMaterial, const plQuat& qBoneDirAdjustment, const plTransform& skeletonRootTransform, plTransform& out_shapeTransform, float fObjectScale)
{
  out_shapeTransform.SetIdentity();
  out_shapeTransform.m_vPosition = qBoneDirAdjustment * geo.m_Transform.m_vPosition * fObjectScale;
  out_shapeTransform.m_qRotation = qBoneDirAdjustment * geo.m_Transform.m_qRotation;

  JPH::Ref<JPH::Shape> pShape;

  switch (geo.m_Type)
  {
    case plSkeletonJointGeometryType::Sphere:
    {
      JPH::SphereShapeSettings shape;
      shape.mUserData = reinterpret_cast<plUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;
      shape.mRadius = geo.m_Transform.m_vScale.z * fObjectScale;

      pShape = shape.Create().Get();
    }
    break;

    case plSkeletonJointGeometryType::Box:
    {
      JPH::BoxShapeSettings shape;
      shape.mUserData = reinterpret_cast<plUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;
      shape.mHalfExtent = plJoltConversionUtils::ToVec3(geo.m_Transform.m_vScale * 0.5f) * fObjectScale;

      out_shapeTransform.m_vPosition += qBoneDirAdjustment * plVec3(geo.m_Transform.m_vScale.x * 0.5f * fObjectScale, 0, 0);

      pShape = shape.Create().Get();
    }
    break;

    case plSkeletonJointGeometryType::Capsule:
    {
      JPH::CapsuleShapeSettings shape;
      shape.mUserData = reinterpret_cast<plUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;
      shape.mHalfHeightOfCylinder = geo.m_Transform.m_vScale.x * 0.5f * fObjectScale;
      shape.mRadius = geo.m_Transform.m_vScale.z * fObjectScale;

      plQuat qRot = plQuat::MakeFromAxisAndAngle(plVec3::MakeAxisZ(), plAngle::MakeFromDegree(-90));
      out_shapeTransform.m_qRotation = out_shapeTransform.m_qRotation * qRot;
      out_shapeTransform.m_vPosition += qBoneDirAdjustment * plVec3(geo.m_Transform.m_vScale.x * 0.5f * fObjectScale, 0, 0);

      pShape = shape.Create().Get();
    }
    break;

    case plSkeletonJointGeometryType::ConvexMesh:
    {
      // convex mesh vertices are in "global space" of the mesh file format
      // so first move them into global space of the PLASMA convention (skeletonRootTransform)
      // then move them to the global position of the ragdoll object
      // then apply the inverse global transform of the limb, to move everything into local space of the limb

      out_shapeTransform = limbConstructionInfo.m_GlobalTransform.GetInverse() * GetOwner()->GetGlobalTransform() * skeletonRootTransform;
      out_shapeTransform.m_vPosition *= fObjectScale;

      plHybridArray<JPH::Vec3, 256> verts;
      verts.SetCountUninitialized(geo.m_VertexPositions.GetCount());

      for (plUInt32 i = 0; i < verts.GetCount(); ++i)
      {
        verts[i] = plJoltConversionUtils::ToVec3(geo.m_VertexPositions[i] * fObjectScale);
      }

      JPH::ConvexHullShapeSettings shape(verts.GetData(), (int)verts.GetCount());
      shape.mUserData = reinterpret_cast<plUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;

      const auto shapeRes = shape.Create();

      if (shapeRes.HasError())
      {
        plLog::Error("Cooking convex ragdoll piece failed: {}", shapeRes.GetError().c_str());
        return nullptr;
      }

      pShape = shapeRes.Get();
    }
    break;

      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  pShape->AddRef();
  return pShape;
}

void plJoltRagdollComponent::CreateAllLimbGeoShapes(const LimbConstructionInfo& limbConstructionInfo, plArrayPtr<const plSkeletonResourceGeometry*> geometries, const plSkeletonJoint& thisLimbJoint, const plSkeletonResource& skeletonResource, float fObjectScale)
{
  const plJoltMaterial* pJoltMaterial = plJoltCore::GetDefaultMaterial();

  if (thisLimbJoint.GetSurface().IsValid())
  {
    plResourceLock<plSurfaceResource> pSurface(thisLimbJoint.GetSurface(), plResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      pJoltMaterial = static_cast<plJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  const plTransform& skeletonRootTransform = skeletonResource.GetDescriptor().m_RootTransform;

  const auto srcBoneDir = skeletonResource.GetDescriptor().m_Skeleton.m_BoneDirection;
  const plQuat qBoneDirAdjustment = plBasisAxis::GetBasisRotation(plBasisAxis::PositiveX, srcBoneDir);

  JPH::RagdollSettings::Part* pBodyDesc = &m_pRagdollSettings->mParts[limbConstructionInfo.m_uiJoltPartIndex];

  if (geometries.GetCount() > 1)
  {
    JPH::StaticCompoundShapeSettings compound;

    for (const plSkeletonResourceGeometry* pGeo : geometries)
    {
      plTransform shapeTransform;
      if (JPH::Shape* pSubShape = CreateLimbGeoShape(limbConstructionInfo, *pGeo, pJoltMaterial, qBoneDirAdjustment, skeletonRootTransform, shapeTransform, fObjectScale))
      {
        compound.AddShape(plJoltConversionUtils::ToVec3(shapeTransform.m_vPosition), plJoltConversionUtils::ToQuat(shapeTransform.m_qRotation), pSubShape);
        pSubShape->Release(); // had to manual AddRef once
      }
    }

    const auto compoundRes = compound.Create();
    if (!compoundRes.IsValid())
    {
      plLog::Error("Creating a compound shape for a ragdoll failed: {}", compoundRes.GetError().c_str());
      return;
    }

    pBodyDesc->SetShape(compoundRes.Get());
  }
  else
  {
    plTransform shapeTransform;
    JPH::Shape* pSubShape = CreateLimbGeoShape(limbConstructionInfo, *geometries[0], pJoltMaterial, qBoneDirAdjustment, skeletonRootTransform, shapeTransform, fObjectScale);

    if (!shapeTransform.IsEqual(plTransform::MakeIdentity(), 0.001f))
    {
      JPH::RotatedTranslatedShapeSettings outerShape;
      outerShape.mInnerShapePtr = pSubShape;
      outerShape.mPosition = plJoltConversionUtils::ToVec3(shapeTransform.m_vPosition);
      outerShape.mRotation = plJoltConversionUtils::ToQuat(shapeTransform.m_qRotation);
      outerShape.mUserData = reinterpret_cast<plUInt64>(m_pJoltUserData);

      pBodyDesc->SetShape(outerShape.Create().Get());
    }
    else
    {
      pBodyDesc->SetShape(pSubShape);
    }

    pSubShape->Release(); // had to manual AddRef once
  }
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


void plJoltRagdollComponent::SetupLimbJoints(const plSkeletonResource* pSkeleton)
{
  // TODO: still needed ? (it should be)
  // the main direction of Jolt bones is +X (for bone limits and such)
  // therefore the main direction of the source bones has to be adjusted
  // const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  // const plQuat qBoneDirAdjustment = -plBasisAxis::GetBasisRotation(srcBoneDir, plBasisAxis::PositiveX);

  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  for (plUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  {
    const auto& thisLimb = m_Limbs[uiLimbIdx];

    if (thisLimb.m_uiPartIndex == plInvalidJointIndex)
      continue;

    const plSkeletonJoint& thisJoint = skeleton.GetJointByIndex(uiLimbIdx);
    plUInt16 uiParentLimb = thisJoint.GetParentIndex();
    while (uiParentLimb != plInvalidJointIndex && m_Limbs[uiParentLimb].m_uiPartIndex == plInvalidJointIndex)
    {
      uiParentLimb = skeleton.GetJointByIndex(uiParentLimb).GetParentIndex();
    }

    if (uiParentLimb == plInvalidJointIndex)
      continue;

    const auto& parentLimb = m_Limbs[uiParentLimb];

    CreateLimbJoint(thisJoint, &m_pRagdollSettings->mParts[parentLimb.m_uiPartIndex], &m_pRagdollSettings->mParts[thisLimb.m_uiPartIndex]);
  }
}

void plJoltRagdollComponent::CreateLimbJoint(const plSkeletonJoint& thisJoint, void* pParentBodyDesc, void* pThisBodyDesc)
{
  plEnum<plSkeletonJointType> jointType = thisJoint.GetJointType();

  for (plUInt32 i = 0; i < m_JointOverrides.GetCount(); ++i)
  {
    if (m_JointOverrides[i].m_sJointName == thisJoint.GetName())
    {
      if (m_JointOverrides[i].m_bOverrideType)
      {
        jointType = m_JointOverrides[i].m_JointType;
      }

      break;
    }
  }

  if (jointType == plSkeletonJointType::None)
    return;

  JPH::RagdollSettings::Part* pLink = reinterpret_cast<JPH::RagdollSettings::Part*>(pThisBodyDesc);
  JPH::RagdollSettings::Part* pParentLink = reinterpret_cast<JPH::RagdollSettings::Part*>(pParentBodyDesc);

  plTransform tParent = plJoltConversionUtils::ToTransform(pParentLink->mPosition, pParentLink->mRotation);
  plTransform tThis = plJoltConversionUtils::ToTransform(pLink->mPosition, pLink->mRotation);

  if (jointType == plSkeletonJointType::Fixed)
  {
    JPH::FixedConstraintSettings* pJoint = new JPH::FixedConstraintSettings();
    pLink->mToParent = pJoint;

    pJoint->mDrawConstraintSize = 0.1f;
    pJoint->mPoint1 = pLink->mPosition;
    pJoint->mPoint2 = pLink->mPosition;
  }

  if (jointType == plSkeletonJointType::SwingTwist)
  {
    JPH::SwingTwistConstraintSettings* pJoint = new JPH::SwingTwistConstraintSettings();
    pLink->mToParent = pJoint;

    const plQuat offsetRot = thisJoint.GetLocalOrientation();

    plQuat qTwist = plQuat::MakeFromAxisAndAngle(plVec3::MakeAxisY(), thisJoint.GetTwistLimitCenterAngle());

    pJoint->mDrawConstraintSize = 0.1f;
    pJoint->mPosition1 = pLink->mPosition;
    pJoint->mPosition2 = pLink->mPosition;
    pJoint->mNormalHalfConeAngle = thisJoint.GetHalfSwingLimitZ().GetRadian();
    pJoint->mPlaneHalfConeAngle = thisJoint.GetHalfSwingLimitY().GetRadian();
    pJoint->mTwistMinAngle = -thisJoint.GetTwistLimitHalfAngle().GetRadian();
    pJoint->mTwistMaxAngle = thisJoint.GetTwistLimitHalfAngle().GetRadian();
    pJoint->mMaxFrictionTorque = m_fStiffnessFactor * thisJoint.GetStiffness();
    pJoint->mPlaneAxis1 = plJoltConversionUtils::ToVec3(tParent.m_qRotation * offsetRot * qTwist * plVec3::MakeAxisZ()).Normalized();
    pJoint->mPlaneAxis2 = plJoltConversionUtils::ToVec3(tThis.m_qRotation * qTwist * plVec3::MakeAxisZ()).Normalized();
    pJoint->mTwistAxis1 = plJoltConversionUtils::ToVec3(tParent.m_qRotation * offsetRot * plVec3::MakeAxisY()).Normalized();
    pJoint->mTwistAxis2 = plJoltConversionUtils::ToVec3(tThis.m_qRotation * plVec3::MakeAxisY()).Normalized();
  }
}

PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltRagdollComponent);

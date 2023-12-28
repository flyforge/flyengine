#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plSkeletonPoseMode, 1)
  PLASMA_ENUM_CONSTANTS(plSkeletonPoseMode::CustomPose, plSkeletonPoseMode::RestPose, plSkeletonPoseMode::Disabled)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_COMPONENT_TYPE(plSkeletonPoseComponent, 4, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    PLASMA_ENUM_ACCESSOR_PROPERTY("Mode", plSkeletonPoseMode, GetPoseMode, SetPoseMode),
    PLASMA_MEMBER_PROPERTY("EditBones", m_fDummy),
    PLASMA_MAP_ACCESSOR_PROPERTY("Bones", GetBones, GetBone, SetBone, RemoveBone)->AddAttributes(new plExposedParametersAttribute("Skeleton"), new plContainerAttribute(false, true, false)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation"),
    new plBoneManipulatorAttribute("Bones", "EditBones"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSkeletonPoseComponent::plSkeletonPoseComponent() = default;
plSkeletonPoseComponent::~plSkeletonPoseComponent() = default;

void plSkeletonPoseComponent::Update()
{
  if (m_uiResendPose == 0)
    return;

  if (--m_uiResendPose > 0)
  {
    static_cast<plSkeletonPoseComponentManager*>(GetOwningManager())->EnqueueUpdate(GetHandle());
  }

  if (m_PoseMode == plSkeletonPoseMode::RestPose)
  {
    SendRestPose();
    return;
  }

  if (m_PoseMode == plSkeletonPoseMode::CustomPose)
  {
    SendCustomPose();
    return;
  }
}

void plSkeletonPoseComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hSkeleton;
  s << m_PoseMode;

  m_Bones.Sort();
  plUInt16 numBones = static_cast<plUInt16>(m_Bones.GetCount());
  s << numBones;

  for (plUInt16 i = 0; i < numBones; ++i)
  {
    s << m_Bones.GetKey(i);
    s << m_Bones.GetValue(i).m_sName;
    s << m_Bones.GetValue(i).m_sParent;
    s << m_Bones.GetValue(i).m_Transform;
  }
}

void plSkeletonPoseComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hSkeleton;
  s >> m_PoseMode;

  plHashedString sKey;
  plExposedBone bone;

  plUInt16 numBones = 0;
  s >> numBones;
  m_Bones.Reserve(numBones);

  for (plUInt16 i = 0; i < numBones; ++i)
  {
    s >> sKey;
    s >> bone.m_sName;
    s >> bone.m_sParent;
    s >> bone.m_Transform;

    m_Bones[sKey] = bone;
  }
  ResendPose();
}

void plSkeletonPoseComponent::OnActivated()
{
  SUPER::OnActivated();

  ResendPose();
}

void plSkeletonPoseComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ResendPose();
}

void plSkeletonPoseComponent::SetSkeletonFile(const char* szFile)
{
  plSkeletonResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plSkeletonResource>(szFile);
  }

  SetSkeleton(hResource);
}

const char* plSkeletonPoseComponent::GetSkeletonFile() const
{
  if (!m_hSkeleton.IsValid())
    return "";

  return m_hSkeleton.GetResourceID();
}

void plSkeletonPoseComponent::SetSkeleton(const plSkeletonResourceHandle& hResource)
{
  if (m_hSkeleton != hResource)
  {
    m_hSkeleton = hResource;
    ResendPose();
  }
}

void plSkeletonPoseComponent::SetPoseMode(plEnum<plSkeletonPoseMode> mode)
{
  m_PoseMode = mode;
  ResendPose();
}

void plSkeletonPoseComponent::ResendPose()
{
  if (m_uiResendPose == 2)
    return;

  m_uiResendPose = 2;
  static_cast<plSkeletonPoseComponentManager*>(GetOwningManager())->EnqueueUpdate(GetHandle());
}

const plRangeView<const char*, plUInt32> plSkeletonPoseComponent::GetBones() const
{
  return plRangeView<const char*, plUInt32>([]() -> plUInt32 { return 0; },
    [this]() -> plUInt32 { return m_Bones.GetCount(); },
    [](plUInt32& ref_uiIt) { ++ref_uiIt; },
    [this](const plUInt32& uiIt) -> const char* { return m_Bones.GetKey(uiIt).GetString().GetData(); });
}

void plSkeletonPoseComponent::SetBone(const char* szKey, const plVariant& value)
{
  plHashedString hs;
  hs.Assign(szKey);

  if (value.GetReflectedType() == plGetStaticRTTI<plExposedBone>())
  {
    m_Bones[hs] = *reinterpret_cast<const plExposedBone*>(value.GetData());
  }

  // TODO
  // if (IsActiveAndInitialized())
  //{
  //  // only add to update list, if not yet activated,
  //  // since OnActivate will do the instantiation anyway
  //  GetWorld()->GetComponentManager<plPrefabReferenceComponentManager>()->AddToUpdateList(this);
  //}
  ResendPose();
}

void plSkeletonPoseComponent::RemoveBone(const char* szKey)
{
  if (m_Bones.RemoveAndCopy(plTempHashedString(szKey)))
  {
    // TODO
    // if (IsActiveAndInitialized())
    //{
    //  // only add to update list, if not yet activated,
    //  // since OnActivate will do the instantiation anyway
    //  GetWorld()->GetComponentManager<plPrefabReferenceComponentManager>()->AddToUpdateList(this);
    //}

    ResendPose();
  }
}

bool plSkeletonPoseComponent::GetBone(const char* szKey, plVariant& out_value) const
{
  plUInt32 it = m_Bones.Find(szKey);

  if (it == plInvalidIndex)
    return false;

  out_value.CopyTypedObject(&m_Bones.GetValue(it), plGetStaticRTTI<plExposedBone>());
  return true;
}

void plSkeletonPoseComponent::SendRestPose()
{
  if (!m_hSkeleton.IsValid())
    return;

  plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  const auto& desc = pSkeleton->GetDescriptor();
  const auto& skel = desc.m_Skeleton;

  if (skel.GetJointCount() == 0)
    return;

  plHybridArray<plMat4, 32> finalTransforms(plFrameAllocator::GetCurrentAllocator());
  finalTransforms.SetCountUninitialized(skel.GetJointCount());

  {
    ozz::animation::LocalToModelJob job;
    job.input = skel.GetOzzSkeleton().joint_rest_poses();
    job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(finalTransforms.GetData()), finalTransforms.GetCount());
    job.skeleton = &skel.GetOzzSkeleton();
    job.Run();
  }

  plMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &skel;
  msg.m_ModelTransforms = finalTransforms;

  GetOwner()->SendMessage(msg);

  if (msg.m_bContinueAnimating == false)
    m_PoseMode = plSkeletonPoseMode::Disabled;
}

void plSkeletonPoseComponent::SendCustomPose()
{
  if (!m_hSkeleton.IsValid())
    return;

  plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::BlockTillLoaded);
  const auto& desc = pSkeleton->GetDescriptor();
  const auto& skel = desc.m_Skeleton;

  plHybridArray<plMat4, 32> finalTransforms(plFrameAllocator::GetCurrentAllocator());
  finalTransforms.SetCountUninitialized(skel.GetJointCount());

  for (plUInt32 i = 0; i < finalTransforms.GetCount(); ++i)
  {
    finalTransforms[i].SetIdentity();
  }

  ozz::vector<ozz::math::SoaTransform> ozzLocalTransforms;
  ozzLocalTransforms.resize((skel.GetJointCount() + 3) / 4);

  auto restPoses = skel.GetOzzSkeleton().joint_rest_poses();

  // initialize the skeleton with the rest pose
  for (plUInt32 i = 0; i < ozzLocalTransforms.size(); ++i)
  {
    ozzLocalTransforms[i] = restPoses[i];
  }

  for (const auto& boneIt : m_Bones)
  {
    const plUInt16 uiBone = skel.FindJointByName(boneIt.key);
    if (uiBone == plInvalidJointIndex)
      continue;

    const plExposedBone& thisBone = boneIt.value;

    // this can happen when the property was reverted
    if (thisBone.m_sName.IsEmpty() || thisBone.m_sParent.IsEmpty())
      continue;

    PLASMA_ASSERT_DEBUG(!thisBone.m_Transform.m_qRotation.IsNaN(), "Invalid bone transform in pose component");

    const plQuat& boneRot = thisBone.m_Transform.m_qRotation;

    const plUInt32 idx0 = uiBone / 4;
    const plUInt32 idx1 = uiBone % 4;

    ozz::math::SoaQuaternion& q = ozzLocalTransforms[idx0].rotation;
    reinterpret_cast<float*>(&q.x)[idx1] = boneRot.v.x;
    reinterpret_cast<float*>(&q.y)[idx1] = boneRot.v.y;
    reinterpret_cast<float*>(&q.z)[idx1] = boneRot.v.z;
    reinterpret_cast<float*>(&q.w)[idx1] = boneRot.w;
  }

  ozz::animation::LocalToModelJob job;
  job.input = ozz::span<const ozz::math::SoaTransform>(ozzLocalTransforms.data(), ozzLocalTransforms.size());
  job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(finalTransforms.GetData()), finalTransforms.GetCount());
  job.skeleton = &skel.GetOzzSkeleton();
  PLASMA_ASSERT_DEBUG(job.Validate(), "");
  job.Run();


  plMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &skel;
  msg.m_ModelTransforms = finalTransforms;

  GetOwner()->SendMessage(msg);

  if (msg.m_bContinueAnimating == false)
    m_PoseMode = plSkeletonPoseMode::Disabled;
}

//////////////////////////////////////////////////////////////////////////

void plSkeletonPoseComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  plDeque<plComponentHandle> requireUpdate;

  {
    PLASMA_LOCK(m_Mutex);
    requireUpdate.Swap(m_RequireUpdate);
  }

  for (const auto& hComp : requireUpdate)
  {
    plSkeletonPoseComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp) || !pComp->IsActiveAndInitialized())
      continue;

    pComp->Update();
  }
}

void plSkeletonPoseComponentManager::EnqueueUpdate(plComponentHandle hComponent)
{
  PLASMA_LOCK(m_Mutex);

  if (m_RequireUpdate.IndexOf(hComponent) != plInvalidIndex)
    return;

  m_RequireUpdate.PushBack(hComponent);
}

void plSkeletonPoseComponentManager::Initialize()
{
  SUPER::Initialize();

  plWorldModule::UpdateFunctionDesc desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plSkeletonPoseComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonPoseComponent);

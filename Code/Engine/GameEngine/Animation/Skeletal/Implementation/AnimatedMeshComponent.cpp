#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/RenderWorld/RenderWorld.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/span.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plAnimatedMeshComponent, 13, plComponentMode::Dynamic); // TODO: why dynamic ? (I guess because the overridden CreateRenderData() has to be called every frame)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned")),
    PLASMA_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new plExposeColorAlphaAttribute()),
    PLASMA_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation"),
  }
  PLASMA_END_ATTRIBUTES;

  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    PLASMA_MESSAGE_HANDLER(plMsgQueryAnimationSkeleton, OnQueryAnimationSkeleton),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_COMPONENT_TYPE

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plRootMotionMode, 1)
  PLASMA_ENUM_CONSTANTS(plRootMotionMode::Ignore, plRootMotionMode::ApplyToOwner, plRootMotionMode::SendMoveCharacterMsg)
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

plAnimatedMeshComponent::plAnimatedMeshComponent() = default;
plAnimatedMeshComponent::~plAnimatedMeshComponent() = default;

void plAnimatedMeshComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
}

void plAnimatedMeshComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  PLASMA_ASSERT_DEV(uiVersion >= 13, "Unsupported version, delete the file and reexport it");
}

void plAnimatedMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  InitializeAnimationPose();
}

void plAnimatedMeshComponent::OnDeactivated()
{
  m_SkinningState.Clear();

  SUPER::OnDeactivated();
}

void plAnimatedMeshComponent::InitializeAnimationPose()
{
  m_MaxBounds = plBoundingBox::MakeInvalid();

  if (!m_hMesh.IsValid())
    return;

  plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::BlockTillLoaded);
  if (pMesh.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  m_hDefaultSkeleton = pMesh->m_hDefaultSkeleton;
  const auto hSkeleton = m_hDefaultSkeleton;

  if (!hSkeleton.IsValid())
    return;

  plResourceLock<plSkeletonResource> pSkeleton(hSkeleton, plResourceAcquireMode::BlockTillLoaded);
  if (pSkeleton.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  {
    const ozz::animation::Skeleton* pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
    const plUInt32 uiNumSkeletonJoints = pOzzSkeleton->num_joints();

    plArrayPtr<plMat4> pPoseMatrices = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plMat4, uiNumSkeletonJoints);

    {
      ozz::animation::LocalToModelJob job;
      job.input = pOzzSkeleton->joint_rest_poses();
      job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(pPoseMatrices.GetPtr()), reinterpret_cast<ozz::math::Float4x4*>(pPoseMatrices.GetEndPtr()));
      job.skeleton = pOzzSkeleton;
      job.Run();
    }

    plMsgAnimationPoseUpdated msg;
    msg.m_ModelTransforms = pPoseMatrices;
    msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
    msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;

    OnAnimationPoseUpdated(msg);
  }

  TriggerLocalBoundsUpdate();
}


void plAnimatedMeshComponent::MapModelSpacePoseToSkinningSpace(const plHashTable<plHashedString, plMeshResourceDescriptor::BoneData>& bones, const plSkeleton& skeleton, plArrayPtr<const plMat4> modelSpaceTransforms, plBoundingBox* bounds)
{
  m_SkinningState.m_Transforms.SetCountUninitialized(bones.GetCount());

  if (bounds)
  {
    for (auto itBone : bones)
    {
      const plUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

      if (uiJointIdx == plInvalidJointIndex)
        continue;

      bounds->ExpandToInclude(modelSpaceTransforms[uiJointIdx].GetTranslationVector());
      m_SkinningState.m_Transforms[itBone.Value().m_uiBoneIndex] = modelSpaceTransforms[uiJointIdx] * itBone.Value().m_GlobalInverseRestPoseMatrix;
    }
  }
  else
  {
    for (auto itBone : bones)
    {
      const plUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

      if (uiJointIdx == plInvalidJointIndex)
        continue;

      m_SkinningState.m_Transforms[itBone.Value().m_uiBoneIndex] = modelSpaceTransforms[uiJointIdx] * itBone.Value().m_GlobalInverseRestPoseMatrix;
    }
  }
}

plMeshRenderData* plAnimatedMeshComponent::CreateRenderData() const
{
  auto pRenderData = plCreateRenderDataForThisFrame<plSkinnedMeshRenderData>(GetOwner());
  pRenderData->m_GlobalTransform = m_RootTransform;

  m_SkinningState.FillSkinnedMeshRenderData(*pRenderData);

  return pRenderData;
}

void plAnimatedMeshComponent::RetrievePose(plDynamicArray<plMat4>& out_modelTransforms, plTransform& out_rootTransform, const plSkeleton& skeleton)
{
  out_modelTransforms.Clear();

  if (!m_hMesh.IsValid())
    return;

  out_rootTransform = m_RootTransform;

  plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::BlockTillLoaded);

  const plHashTable<plHashedString, plMeshResourceDescriptor::BoneData>& bones = pMesh->m_Bones;

  out_modelTransforms.SetCount(skeleton.GetJointCount(), plMat4::MakeIdentity());

  for (auto itBone : bones)
  {
    const plUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

    if (uiJointIdx == plInvalidJointIndex)
      continue;

    out_modelTransforms[uiJointIdx] = m_SkinningState.m_Transforms[itBone.Value().m_uiBoneIndex].GetAsMat4() * itBone.Value().m_GlobalInverseRestPoseMatrix.GetInverse();
  }
}

void plAnimatedMeshComponent::OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& msg)
{
  if (!m_hMesh.IsValid())
    return;

  m_RootTransform = *msg.m_pRootTransform;

  plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::BlockTillLoaded);

  plBoundingBox poseBounds = plBoundingBox::MakeInvalid();
  MapModelSpacePoseToSkinningSpace(pMesh->m_Bones, *msg.m_pSkeleton, msg.m_ModelTransforms, &poseBounds);

  if (poseBounds.IsValid() && (!m_MaxBounds.IsValid() || !m_MaxBounds.Contains(poseBounds)))
  {
    m_MaxBounds.ExpandToInclude(poseBounds);
    TriggerLocalBoundsUpdate();
  }
  else if (((plRenderWorld::GetFrameCounter() + GetUniqueIdForRendering()) & (PLASMA_BIT(10) - 1)) == 0) // reset the bbox every once in a while
  {
    m_MaxBounds = poseBounds;
    TriggerLocalBoundsUpdate();
  }

  m_SkinningState.TransformsChanged();
}

void plAnimatedMeshComponent::OnQueryAnimationSkeleton(plMsgQueryAnimationSkeleton& msg)
{
  if (!msg.m_hSkeleton.IsValid() && m_hMesh.IsValid())
  {
    // only overwrite, if no one else had a better skeleton (e.g. the plSkeletonComponent)

    plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::BlockTillLoaded);
    if (pMesh.GetAcquireResult() == plResourceAcquireResult::Final)
    {
      msg.m_hSkeleton = pMesh->m_hDefaultSkeleton;
    }
  }
}

plResult plAnimatedMeshComponent::GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg)
{
  if (!m_MaxBounds.IsValid() || !m_hMesh.IsValid())
    return PLASMA_FAILURE;

  plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::BlockTillLoaded);
  if (pMesh.GetAcquireResult() != plResourceAcquireResult::Final)
    return PLASMA_FAILURE;

  plBoundingBox bbox = m_MaxBounds;
  bbox.Grow(plVec3(pMesh->m_fMaxBoneVertexOffset));
  bounds = bbox;
  bounds.Transform(m_RootTransform.GetAsMat4());
  return PLASMA_SUCCESS;
}

void plRootMotionMode::Apply(plRootMotionMode::Enum mode, plGameObject* pObject, const plVec3& vTranslation, plAngle rotationX, plAngle rotationY, plAngle rotationZ)
{
  switch (mode)
  {
    case plRootMotionMode::Ignore:
      return;

    case plRootMotionMode::ApplyToOwner:
    {
      plVec3 vNewPos = pObject->GetLocalPosition();
      vNewPos += pObject->GetLocalRotation() * vTranslation;
      pObject->SetLocalPosition(vNewPos);

      // not tested whether this is actually correct
      plQuat rotation = plQuat::MakeFromEulerAngles(rotationX, rotationY, rotationZ);

      pObject->SetLocalRotation(rotation * pObject->GetLocalRotation());

      return;
    }

    case plRootMotionMode::SendMoveCharacterMsg:
    {
      plMsgApplyRootMotion msg;
      msg.m_vTranslation = vTranslation;
      msg.m_RotationX = rotationX;
      msg.m_RotationY = rotationY;
      msg.m_RotationZ = rotationZ;

      while (pObject != nullptr)
      {
        pObject->SendMessage(msg);
        pObject = pObject->GetParent();
      }

      return;
    }
  }
}

//////////////////////////////////////////////////////////////////////////


plAnimatedMeshComponentManager::plAnimatedMeshComponentManager(plWorld* pWorld)
  : plComponentManager<ComponentType, plBlockStorageType::Compact>(pWorld)
{
  plResourceManager::GetResourceEvents().AddEventHandler(plMakeDelegate(&plAnimatedMeshComponentManager::ResourceEventHandler, this));
}

plAnimatedMeshComponentManager::~plAnimatedMeshComponentManager()
{
  plResourceManager::GetResourceEvents().RemoveEventHandler(plMakeDelegate(&plAnimatedMeshComponentManager::ResourceEventHandler, this));
}

void plAnimatedMeshComponentManager::Initialize()
{
  auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plAnimatedMeshComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void plAnimatedMeshComponentManager::ResourceEventHandler(const plResourceEvent& e)
{
  if (e.m_Type == plResourceEvent::Type::ResourceContentUnloading)
  {
    if (plMeshResource* pResource = plDynamicCast<plMeshResource*>(e.m_pResource))
    {
      plMeshResourceHandle hMesh(pResource);

      for (auto it = GetComponents(); it.IsValid(); it.Next())
      {
        if (it->m_hMesh == hMesh)
        {
          AddToUpdateList(it);
        }
      }
    }

    if (plSkeletonResource* pResource = plDynamicCast<plSkeletonResource*>(e.m_pResource))
    {
      plSkeletonResourceHandle hSkeleton(pResource);

      for (auto it = GetComponents(); it.IsValid(); it.Next())
      {
        if (it->m_hDefaultSkeleton == hSkeleton)
        {
          AddToUpdateList(it);
        }
      }
    }
  }
}

void plAnimatedMeshComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    plAnimatedMeshComponent* pComponent = nullptr;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    if (!pComponent->IsActive())
      continue;

    pComponent->InitializeAnimationPose();
  }

  m_ComponentsToUpdate.Clear();
}

void plAnimatedMeshComponentManager::AddToUpdateList(plAnimatedMeshComponent* pComponent)
{
  plComponentHandle hComponent = pComponent->GetHandle();

  if (m_ComponentsToUpdate.IndexOf(hComponent) == plInvalidIndex)
  {
    m_ComponentsToUpdate.PushBack(hComponent);
  }
}

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimatedMeshComponent);

#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <ozz/animation/runtime/skeleton.h>

plMutex plAnimController::s_SharedDataMutex;
plHashTable<plString, plSharedPtr<plAnimGraphSharedBoneWeights>> plAnimController::s_SharedBoneWeights;

plAnimController::plAnimController() = default;
plAnimController::~plAnimController() = default;

void plAnimController::Initialize(const plSkeletonResourceHandle& hSkeleton, plAnimPoseGenerator& ref_poseGenerator, const plSharedPtr<plBlackboard>& pBlackboard /*= nullptr*/)
{
  m_hSkeleton = hSkeleton;
  m_pPoseGenerator = &ref_poseGenerator;
  m_pBlackboard = pBlackboard;
}

void plAnimController::GetRootMotion(plVec3& ref_vTranslation, plAngle& ref_rotationX, plAngle& ref_rotationY, plAngle& ref_rotationZ) const
{
  ref_vTranslation = m_vRootMotion;
  ref_rotationX = m_RootRotationX;
  ref_rotationY = m_RootRotationY;
  ref_rotationZ = m_RootRotationZ;
}

void plAnimController::Update(plTime diff, plGameObject* pTarget)
{
  if (!m_hSkeleton.IsValid())
    return;

  plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  m_pCurrentModelTransforms = nullptr;

  m_CurrentLocalTransformOutputs.Clear();

  m_vRootMotion = plVec3::MakeZero();
  m_RootRotationX = {};
  m_RootRotationY = {};
  m_RootRotationZ = {};

  m_pPoseGenerator->Reset(pSkeleton.GetPointer());

  m_PinDataBoneWeights.Clear();
  m_PinDataLocalTransforms.Clear();
  m_PinDataModelTransforms.Clear();

  // TODO: step all instances

  for (auto& inst : m_Instances)
  {
    inst.m_pInstance->Update(*this, diff, pTarget, pSkeleton.GetPointer());
  }

  GenerateLocalResultProcessors(pSkeleton.GetPointer());

  if (auto newPose = GetPoseGenerator().GeneratePose(pTarget); !newPose.IsEmpty())
  {
    plMsgAnimationPoseUpdated msg;
    msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
    msg.m_ModelTransforms = newPose;

    // TODO: root transform has to be applied first, only then can the world-space IK be done, and then the pose can be finalized
    msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;

    // recursive, so that objects below the mesh can also listen in on these changes
    // for example bone attachments
    pTarget->SendMessageRecursive(msg);
  }
}

void plAnimController::SetOutputModelTransform(plAnimGraphPinDataModelTransforms* pModelTransform)
{
  m_pCurrentModelTransforms = pModelTransform;
}

void plAnimController::SetRootMotion(const plVec3& vTranslation, plAngle rotationX, plAngle rotationY, plAngle rotationZ)
{
  m_vRootMotion = vTranslation;
  m_RootRotationX = rotationX;
  m_RootRotationY = rotationY;
  m_RootRotationZ = rotationZ;
}

void plAnimController::AddOutputLocalTransforms(plAnimGraphPinDataLocalTransforms* pLocalTransforms)
{
  m_CurrentLocalTransformOutputs.PushBack(pLocalTransforms->m_uiOwnIndex);
}

plSharedPtr<plAnimGraphSharedBoneWeights> plAnimController::CreateBoneWeights(const char* szUniqueName, const plSkeletonResource& skeleton, plDelegate<void(plAnimGraphSharedBoneWeights&)> fill)
{
  PLASMA_LOCK(s_SharedDataMutex);

  plSharedPtr<plAnimGraphSharedBoneWeights>& bw = s_SharedBoneWeights[szUniqueName];

  if (bw == nullptr)
  {
    bw = PLASMA_DEFAULT_NEW(plAnimGraphSharedBoneWeights);
    bw->m_Weights.SetCountUninitialized(skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());
    plMemoryUtils::ZeroFill<ozz::math::SimdFloat4>(bw->m_Weights.GetData(), bw->m_Weights.GetCount());
  }

  fill(*bw);

  return bw;
}

void plAnimController::GenerateLocalResultProcessors(const plSkeletonResource* pSkeleton)
{
  if (m_CurrentLocalTransformOutputs.IsEmpty())
    return;

  plAnimGraphPinDataLocalTransforms* pOut = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[0]];

  // combine multiple outputs
  if (m_CurrentLocalTransformOutputs.GetCount() > 1 || pOut->m_pWeights != nullptr)
  {
    const plUInt32 m_uiMaxPoses = 6; // TODO

    pOut = AddPinDataLocalTransforms();
    pOut->m_vRootMotion.SetZero();

    float fSummedRootMotionWeight = 0.0f;

    // TODO: skip blending, if only a single animation is played
    // unless the weight is below 1.0 and the bind pose should be faded in

    auto& cmd = GetPoseGenerator().AllocCommandCombinePoses();

    struct PinWeight
    {
      plUInt32 m_uiPinIdx;
      float m_fPinWeight = 0.0f;
    };

    plHybridArray<PinWeight, 16> pw;
    pw.SetCount(m_CurrentLocalTransformOutputs.GetCount());

    for (plUInt32 i = 0; i < m_CurrentLocalTransformOutputs.GetCount(); ++i)
    {
      pw[i].m_uiPinIdx = i;

      const plAnimGraphPinDataLocalTransforms* pTransforms = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[i]];

      if (pTransforms != nullptr)
      {
        pw[i].m_fPinWeight = pTransforms->m_fOverallWeight;

        if (pTransforms->m_pWeights)
        {
          pw[i].m_fPinWeight *= pTransforms->m_pWeights->m_fOverallWeight;
        }
      }
    }

    if (pw.GetCount() > m_uiMaxPoses)
    {
      pw.Sort([](const PinWeight& lhs, const PinWeight& rhs) { return lhs.m_fPinWeight > rhs.m_fPinWeight; });
      pw.SetCount(m_uiMaxPoses);
    }

    plArrayPtr<const ozz::math::SimdFloat4> invWeights;

    for (const auto& in : pw)
    {
      const plAnimGraphPinDataLocalTransforms* pTransforms = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[in.m_uiPinIdx]];

      if (in.m_fPinWeight > 0 && pTransforms->m_pWeights)
      {
        // only initialize and use the inverse mask, when it is actually needed
        if (invWeights.IsEmpty())
        {
          m_BlendMask.SetCountUninitialized(pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());

          for (auto& sj : m_BlendMask)
          {
            sj = ozz::math::simd_float4::one();
          }

          invWeights = m_BlendMask;
        }

        const ozz::math::SimdFloat4 factor = ozz::math::simd_float4::Load1(in.m_fPinWeight);

        const plArrayPtr<const ozz::math::SimdFloat4> weights = pTransforms->m_pWeights->m_pSharedBoneWeights->m_Weights;

        for (plUInt32 i = 0; i < m_BlendMask.GetCount(); ++i)
        {
          const auto& weight = weights[i];
          auto& mask = m_BlendMask[i];

          const auto oneMinusWeight = ozz::math::NMAdd(factor, weight, ozz::math::simd_float4::one());

          mask = ozz::math::Min(mask, oneMinusWeight);
        }
      }
    }

    for (const auto& in : pw)
    {
      if (in.m_fPinWeight > 0)
      {
        const plAnimGraphPinDataLocalTransforms* pTransforms = &m_PinDataLocalTransforms[m_CurrentLocalTransformOutputs[in.m_uiPinIdx]];

        if (pTransforms->m_pWeights)
        {
          const plArrayPtr<const ozz::math::SimdFloat4> weights = pTransforms->m_pWeights->m_pSharedBoneWeights->m_Weights;

          cmd.m_InputBoneWeights.PushBack(weights);
        }
        else
        {
          cmd.m_InputBoneWeights.PushBack(invWeights);
        }

        if (pTransforms->m_bUseRootMotion)
        {
          fSummedRootMotionWeight += in.m_fPinWeight;
          pOut->m_vRootMotion += pTransforms->m_vRootMotion * in.m_fPinWeight;

          // TODO: combining quaternions is mathematically tricky
          // could maybe use multiple slerps to concatenate weighted quaternions \_(ツ)_/

          pOut->m_bUseRootMotion = true;
        }

        cmd.m_Inputs.PushBack(pTransforms->m_CommandID);
        cmd.m_InputWeights.PushBack(in.m_fPinWeight);
      }
    }

    if (fSummedRootMotionWeight > 1.0f) // normalize down, but not up
    {
      pOut->m_vRootMotion /= fSummedRootMotionWeight;
    }

    pOut->m_CommandID = cmd.GetCommandID();
  }

  plAnimGraphPinDataModelTransforms* pModelTransform = AddPinDataModelTransforms();

  // local space to model space
  {
    if (pOut->m_bUseRootMotion)
    {
      pModelTransform->m_bUseRootMotion = true;
      pModelTransform->m_vRootMotion = pOut->m_vRootMotion;
    }

    auto& cmd = GetPoseGenerator().AllocCommandLocalToModelPose();
    cmd.m_Inputs.PushBack(pOut->m_CommandID);

    pModelTransform->m_CommandID = cmd.GetCommandID();
  }

  // model space to output
  {
    plVec3 rootMotion = plVec3::MakeZero();
    plAngle rootRotationX;
    plAngle rootRotationY;
    plAngle rootRotationZ;
    GetRootMotion(rootMotion, rootRotationX, rootRotationY, rootRotationZ);

    auto& cmd = GetPoseGenerator().AllocCommandModelPoseToOutput();
    cmd.m_Inputs.PushBack(pModelTransform->m_CommandID);

    if (pModelTransform->m_bUseRootMotion)
    {
      rootMotion += pModelTransform->m_vRootMotion;
      rootRotationX += pModelTransform->m_RootRotationX;
      rootRotationY += pModelTransform->m_RootRotationY;
      rootRotationZ += pModelTransform->m_RootRotationZ;
    }

    SetOutputModelTransform(pModelTransform);

    SetRootMotion(rootMotion, rootRotationX, rootRotationY, rootRotationZ);
  }
}

plAnimGraphPinDataBoneWeights* plAnimController::AddPinDataBoneWeights()
{
  plAnimGraphPinDataBoneWeights* pData = &m_PinDataBoneWeights.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<plUInt16>(m_PinDataBoneWeights.GetCount()) - 1;
  return pData;
}

plAnimGraphPinDataLocalTransforms* plAnimController::AddPinDataLocalTransforms()
{
  plAnimGraphPinDataLocalTransforms* pData = &m_PinDataLocalTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<plUInt16>(m_PinDataLocalTransforms.GetCount()) - 1;
  return pData;
}

plAnimGraphPinDataModelTransforms* plAnimController::AddPinDataModelTransforms()
{
  plAnimGraphPinDataModelTransforms* pData = &m_PinDataModelTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<plUInt16>(m_PinDataModelTransforms.GetCount()) - 1;
  return pData;
}

void plAnimController::AddAnimGraph(const plAnimGraphResourceHandle& hGraph)
{
  if (!hGraph.IsValid())
    return;

  for (auto& inst : m_Instances)
  {
    if (inst.m_hAnimGraph == hGraph)
      return;
  }

  plResourceLock<plAnimGraphResource> pAnimGraph(hGraph, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimGraph.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  auto& inst = m_Instances.ExpandAndGetRef();
  inst.m_hAnimGraph = hGraph;
  inst.m_pInstance = PLASMA_DEFAULT_NEW(plAnimGraphInstance);
  inst.m_pInstance->Configure(pAnimGraph->GetAnimationGraph());

  for (auto& clip : pAnimGraph->GetAnimationClipMapping())
  {
    bool bExisted = false;
    auto& info = m_AnimationClipMapping.FindOrAdd(clip.m_sClipName, &bExisted);
    if (!bExisted)
    {
      info.m_hClip = clip.m_hClip;
    }
  }

  for (auto& ig : pAnimGraph->GetIncludeGraphs())
  {
    AddAnimGraph(plResourceManager::LoadResource<plAnimGraphResource>(ig));
  }
}

const plAnimController::AnimClipInfo& plAnimController::GetAnimationClipInfo(plTempHashedString sClipName) const
{
  auto it = m_AnimationClipMapping.Find(sClipName);
  if (!it.IsValid())
    return m_InvalidClipInfo;

  return it.Value();
}

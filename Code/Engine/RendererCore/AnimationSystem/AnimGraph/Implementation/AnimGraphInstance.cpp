#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/skeleton.h>

plAnimGraphInstance::plAnimGraphInstance() = default;

plAnimGraphInstance::~plAnimGraphInstance()
{
  if (m_pAnimGraph)
  {
    m_pAnimGraph->GetInstanceDataAlloator().DestructAndDeallocate(m_InstanceData);
  }
}

void plAnimGraphInstance::Configure(const plAnimGraph& animGraph)
{
  m_pAnimGraph = &animGraph;

  m_InstanceData = m_pAnimGraph->GetInstanceDataAlloator().AllocateAndConstruct();

  // EXTEND THIS if a new type is introduced
  m_pTriggerInputPinStates = (plInt8*)plInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[plAnimGraphPin::Type::Trigger]);
  m_pNumberInputPinStates = (double*)plInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[plAnimGraphPin::Type::Number]);
  m_pBoolInputPinStates = (bool*)plInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[plAnimGraphPin::Type::Bool]);
  m_pBoneWeightInputPinStates = (plUInt16*)plInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[plAnimGraphPin::Type::BoneWeights]);
  m_pModelPoseInputPinStates = (plUInt16*)plInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[plAnimGraphPin::Type::ModelPose]);

  m_LocalPoseInputPinStates.SetCount(animGraph.m_uiInputPinCounts[plAnimGraphPin::Type::LocalPose]);
}

void plAnimGraphInstance::Update(plAnimController& ref_controller, plTime diff, plGameObject* pTarget, const plSkeletonResource* pSekeltonResource)
{
  // reset all pin states
  {
    // EXTEND THIS if a new type is introduced

    plMemoryUtils::ZeroFill(m_pTriggerInputPinStates, m_pAnimGraph->m_uiInputPinCounts[plAnimGraphPin::Type::Trigger]);
    plMemoryUtils::ZeroFill(m_pNumberInputPinStates, m_pAnimGraph->m_uiInputPinCounts[plAnimGraphPin::Type::Number]);
    plMemoryUtils::ZeroFill(m_pBoolInputPinStates, m_pAnimGraph->m_uiInputPinCounts[plAnimGraphPin::Type::Bool]);
    plMemoryUtils::ZeroFill(m_pBoneWeightInputPinStates, m_pAnimGraph->m_uiInputPinCounts[plAnimGraphPin::Type::BoneWeights]);
    plMemoryUtils::PatternFill(m_pModelPoseInputPinStates, 0xFF, m_pAnimGraph->m_uiInputPinCounts[plAnimGraphPin::Type::ModelPose]);

    for (auto& pins : m_LocalPoseInputPinStates)
    {
      pins.Clear();
    }
  }

  for (const auto& pNode : m_pAnimGraph->GetNodes())
  {
    pNode->Step(ref_controller, *this, diff, pSekeltonResource, pTarget);
  }
}

#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphPin, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("PinIdx", m_iPinIndex)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("NumConnections", m_uiNumConnections)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphInputPin, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphOutputPin, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plAnimGraphPin::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream << m_iPinIndex;
  inout_stream << m_uiNumConnections;
  return PLASMA_SUCCESS;
}

plResult plAnimGraphPin::Deserialize(plStreamReader& inout_stream)
{
  inout_stream >> m_iPinIndex;
  inout_stream >> m_uiNumConnections;
  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphTriggerInputPin, 1, plRTTIDefaultAllocator<plAnimGraphTriggerInputPin>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphTriggerOutputPin, 1, plRTTIDefaultAllocator<plAnimGraphTriggerOutputPin>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plAnimGraphTriggerOutputPin::SetTriggered(plAnimGraphInstance& ref_graph) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[plAnimGraphPin::Trigger][m_iPinIndex];


  const plInt8 offset = +1; // bTriggered ? +1 : -1;

  // trigger or reset all input pins that are connected to this output pin
  for (plUInt16 idx : map)
  {
    ref_graph.m_pTriggerInputPinStates[idx] += offset;
  }
}

bool plAnimGraphTriggerInputPin::IsTriggered(plAnimGraphInstance& ref_graph) const
{
  if (m_iPinIndex < 0)
    return false;

  return ref_graph.m_pTriggerInputPinStates[m_iPinIndex] > 0;
}

bool plAnimGraphTriggerInputPin::AreAllTriggered(plAnimGraphInstance& ref_graph) const
{
  return ref_graph.m_pTriggerInputPinStates[m_iPinIndex] == m_uiNumConnections;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphNumberInputPin, 1, plRTTIDefaultAllocator<plAnimGraphNumberInputPin>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphNumberOutputPin, 1, plRTTIDefaultAllocator<plAnimGraphNumberOutputPin>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

double plAnimGraphNumberInputPin::GetNumber(plAnimGraphInstance& ref_graph, double fFallback /*= 0.0*/) const
{
  if (m_iPinIndex < 0)
    return fFallback;

  return ref_graph.m_pNumberInputPinStates[m_iPinIndex];
}

void plAnimGraphNumberOutputPin::SetNumber(plAnimGraphInstance& ref_graph, double value) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[plAnimGraphPin::Number][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (plUInt16 idx : map)
  {
    ref_graph.m_pNumberInputPinStates[idx] = value;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphBoolInputPin, 1, plRTTIDefaultAllocator<plAnimGraphBoolInputPin>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphBoolOutputPin, 1, plRTTIDefaultAllocator<plAnimGraphBoolOutputPin>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool plAnimGraphBoolInputPin::GetBool(plAnimGraphInstance& ref_graph, bool bFallback /*= false */) const
{
  if (m_iPinIndex < 0)
    return bFallback;

  return ref_graph.m_pBoolInputPinStates[m_iPinIndex];
}

void plAnimGraphBoolOutputPin::SetBool(plAnimGraphInstance& ref_graph, bool bValue) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[plAnimGraphPin::Bool][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (plUInt16 idx : map)
  {
    ref_graph.m_pBoolInputPinStates[idx] = bValue;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphBoneWeightsInputPin, 1, plRTTIDefaultAllocator<plAnimGraphBoneWeightsInputPin>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphBoneWeightsOutputPin, 1, plRTTIDefaultAllocator<plAnimGraphBoneWeightsOutputPin>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAnimGraphPinDataBoneWeights* plAnimGraphBoneWeightsInputPin::GetWeights(plAnimController& ref_controller, plAnimGraphInstance& ref_graph) const
{
  if (m_iPinIndex < 0 || ref_graph.m_pBoneWeightInputPinStates[m_iPinIndex] == 0xFFFF)
    return nullptr;

  return &ref_controller.m_PinDataBoneWeights[ref_graph.m_pBoneWeightInputPinStates[m_iPinIndex]];
}

void plAnimGraphBoneWeightsOutputPin::SetWeights(plAnimGraphInstance& ref_graph, plAnimGraphPinDataBoneWeights* pWeights) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[plAnimGraphPin::BoneWeights][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (plUInt16 idx : map)
  {
    ref_graph.m_pBoneWeightInputPinStates[idx] = pWeights->m_uiOwnIndex;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphLocalPoseInputPin, 1, plRTTIDefaultAllocator<plAnimGraphLocalPoseInputPin>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphLocalPoseMultiInputPin, 1, plRTTIDefaultAllocator<plAnimGraphLocalPoseMultiInputPin>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphLocalPoseOutputPin, 1, plRTTIDefaultAllocator<plAnimGraphLocalPoseOutputPin>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAnimGraphPinDataLocalTransforms* plAnimGraphLocalPoseInputPin::GetPose(plAnimController& ref_controller, plAnimGraphInstance& ref_graph) const
{
  if (m_iPinIndex < 0)
    return nullptr;

  if (ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].IsEmpty())
    return nullptr;

  return &ref_controller.m_PinDataLocalTransforms[ref_graph.m_LocalPoseInputPinStates[m_iPinIndex][0]];
}

void plAnimGraphLocalPoseMultiInputPin::GetPoses(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plDynamicArray<plAnimGraphPinDataLocalTransforms*>& out_poses) const
{
  out_poses.Clear();

  if (m_iPinIndex < 0)
    return;

  out_poses.SetCountUninitialized(ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].GetCount());
  for (plUInt32 i = 0; i < ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].GetCount(); ++i)
  {
    out_poses[i] = &ref_controller.m_PinDataLocalTransforms[ref_graph.m_LocalPoseInputPinStates[m_iPinIndex][i]];
  }
}

void plAnimGraphLocalPoseOutputPin::SetPose(plAnimGraphInstance& ref_graph, plAnimGraphPinDataLocalTransforms* pPose) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[plAnimGraphPin::LocalPose][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (plUInt16 idx : map)
  {
    ref_graph.m_LocalPoseInputPinStates[idx].PushBack(pPose->m_uiOwnIndex);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphModelPoseInputPin, 1, plRTTIDefaultAllocator<plAnimGraphModelPoseInputPin>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphModelPoseOutputPin, 1, plRTTIDefaultAllocator<plAnimGraphModelPoseOutputPin>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAnimGraphPinDataModelTransforms* plAnimGraphModelPoseInputPin::GetPose(plAnimController& ref_controller, plAnimGraphInstance& ref_graph) const
{
  if (m_iPinIndex < 0 || ref_graph.m_pModelPoseInputPinStates[m_iPinIndex] == 0xFFFF)
    return nullptr;

  return &ref_controller.m_PinDataModelTransforms[ref_graph.m_pModelPoseInputPinStates[m_iPinIndex]];
}

void plAnimGraphModelPoseOutputPin::SetPose(plAnimGraphInstance& ref_graph, plAnimGraphPinDataModelTransforms* pPose) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[plAnimGraphPin::ModelPose][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (plUInt16 idx : map)
  {
    ref_graph.m_pModelPoseInputPinStates[idx] = pPose->m_uiOwnIndex;
  }
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphPins);

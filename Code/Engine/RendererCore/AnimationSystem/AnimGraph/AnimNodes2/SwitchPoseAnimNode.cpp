#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SwitchPoseAnimNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSwitchPoseAnimNode, 1, plRTTIDefaultAllocator<plSwitchPoseAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("TransitionDuration", m_TransitionDuration)->AddAttributes(new plDefaultValueAttribute(plTime::MakeFromMilliseconds(200))),
    PLASMA_MEMBER_PROPERTY("InIndex", m_InIndex)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("PosesCount", m_uiPosesCount)->AddAttributes(new plNoTemporaryTransactionsAttribute(), new plDynamicPinAttribute(), new plDefaultValueAttribute(2)),
    PLASMA_ARRAY_MEMBER_PROPERTY("InPoses", m_InPoses)->AddAttributes(new plHiddenAttribute(), new plDynamicPinAttribute("PosesCount")),
    PLASMA_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Pose Blending"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Yellow)),
    new plTitleAttribute("Pose Switch"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plSwitchPoseAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_TransitionDuration;
  stream << m_uiPosesCount;

  PLASMA_SUCCEED_OR_RETURN(m_InIndex.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(stream.WriteArray(m_InPoses));
  PLASMA_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plSwitchPoseAnimNode::DeserializeNode(plStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);
  PLASMA_IGNORE_UNUSED(version);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_TransitionDuration;
  stream >> m_uiPosesCount;

  PLASMA_SUCCEED_OR_RETURN(m_InIndex.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(stream.ReadArray(m_InPoses));
  PLASMA_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return PLASMA_SUCCESS;
}

void plSwitchPoseAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected() || !m_InIndex.IsConnected())
    return;

  plHybridArray<const plAnimGraphLocalPoseInputPin*, 12> pPins;
  for (plUInt32 i = 0; i < m_InPoses.GetCount(); ++i)
  {
    pPins.PushBack(&m_InPoses[i]);
  }

  // duplicate pin connections to fill up holes
  for (plUInt32 i = 1; i < pPins.GetCount(); ++i)
  {
    if (!pPins[i]->IsConnected())
      pPins[i] = pPins[i - 1];
  }
  for (plUInt32 i = pPins.GetCount(); i > 1; --i)
  {
    if (!pPins[i - 2]->IsConnected())
      pPins[i - 2] = pPins[i - 1];
  }

  if (pPins.IsEmpty() || !pPins[0]->IsConnected())
  {
    // this can only be the case if no pin is connected, at all
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const plInt8 iDstIdx = plMath::Clamp<plInt8>((plInt8)m_InIndex.GetNumber(ref_graph, 0), 0, pPins.GetCount() - 1);

  if (pInstance->m_iTransitionToIndex < 0)
  {
    pInstance->m_iTransitionToIndex = iDstIdx;
    pInstance->m_iTransitionFromIndex = iDstIdx;
  }

  pInstance->m_TransitionTime += tDiff;

  if (iDstIdx != pInstance->m_iTransitionToIndex)
  {
    pInstance->m_iTransitionFromIndex = pInstance->m_iTransitionToIndex;
    pInstance->m_iTransitionToIndex = iDstIdx;
    pInstance->m_TransitionTime = plTime::MakeZero();
  }

  if (pInstance->m_TransitionTime >= m_TransitionDuration)
  {
    pInstance->m_iTransitionFromIndex = pInstance->m_iTransitionToIndex;
  }

  PLASMA_ASSERT_DEBUG(pInstance->m_iTransitionToIndex >= 0 && pInstance->m_iTransitionToIndex < pPins.GetCount(), "Invalid pose index");
  PLASMA_ASSERT_DEBUG(pInstance->m_iTransitionToIndex >= 0 && pInstance->m_iTransitionToIndex < pPins.GetCount(), "Invalid pose index");

  plInt8 iTransitionFromIndex = pInstance->m_iTransitionFromIndex;
  plInt8 iTransitionToIndex = pInstance->m_iTransitionToIndex;

  if (pPins[iTransitionFromIndex]->GetPose(ref_controller, ref_graph) == nullptr)
  {
    // if the 'from' pose already stopped, just jump to the 'to' pose
    iTransitionFromIndex = iTransitionToIndex;
  }

  if (iTransitionFromIndex == iTransitionToIndex)
  {
    const plAnimGraphLocalPoseInputPin* pPinToForward = pPins[iTransitionToIndex];
    plAnimGraphPinDataLocalTransforms* pDataToForward = pPinToForward->GetPose(ref_controller, ref_graph);

    if (pDataToForward == nullptr)
      return;

    plAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();
    pLocalTransforms->m_CommandID = pDataToForward->m_CommandID;
    pLocalTransforms->m_pWeights = pDataToForward->m_pWeights;
    pLocalTransforms->m_fOverallWeight = pDataToForward->m_fOverallWeight;
    pLocalTransforms->m_vRootMotion = pDataToForward->m_vRootMotion;
    pLocalTransforms->m_bUseRootMotion = pDataToForward->m_bUseRootMotion;

    m_OutPose.SetPose(ref_graph, pLocalTransforms);
  }
  else
  {
    auto pPose0 = pPins[iTransitionFromIndex]->GetPose(ref_controller, ref_graph);
    auto pPose1 = pPins[iTransitionToIndex]->GetPose(ref_controller, ref_graph);

    if (pPose0 == nullptr || pPose1 == nullptr)
      return;

    plAnimGraphPinDataLocalTransforms* pPinData = ref_controller.AddPinDataLocalTransforms();

    const float fLerp = (float)plMath::Clamp(pInstance->m_TransitionTime.GetSeconds() / m_TransitionDuration.GetSeconds(), 0.0, 1.0);

    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandCombinePoses();
    cmd.m_InputWeights.SetCount(2);
    cmd.m_InputWeights[0] = 1.0f - fLerp;
    cmd.m_InputWeights[1] = fLerp;
    cmd.m_Inputs.SetCount(2);
    cmd.m_Inputs[0] = pPose0->m_CommandID;
    cmd.m_Inputs[1] = pPose1->m_CommandID;

    pPinData->m_CommandID = cmd.GetCommandID();
    pPinData->m_bUseRootMotion = pPose0->m_bUseRootMotion || pPose1->m_bUseRootMotion;
    pPinData->m_vRootMotion = plMath::Lerp(pPose0->m_vRootMotion, pPose1->m_vRootMotion, fLerp);

    m_OutPose.SetPose(ref_graph, pPinData);
  }
}

bool plSwitchPoseAnimNode::GetInstanceDataDesc(plInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

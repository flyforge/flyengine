#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/LerpPosesAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLerpPosesAnimNode, 1, plRTTIDefaultAllocator<plLerpPosesAnimNode>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Lerp", m_fLerp)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 3.0f)),
    PL_MEMBER_PROPERTY("InLerp", m_InLerp)->AddAttributes(new plHiddenAttribute),
    PL_MEMBER_PROPERTY("PosesCount", m_uiPosesCount)->AddAttributes(new plNoTemporaryTransactionsAttribute(), new plDynamicPinAttribute(), new plDefaultValueAttribute(2)),
    PL_ARRAY_MEMBER_PROPERTY("InPoses", m_InPoses)->AddAttributes(new plHiddenAttribute(), new plDynamicPinAttribute("PosesCount")),
    PL_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new plHiddenAttribute()),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Pose Blending"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Violet)),
    new plTitleAttribute("Lerp Poses"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plLerpPosesAnimNode::plLerpPosesAnimNode() = default;
plLerpPosesAnimNode::~plLerpPosesAnimNode() = default;

plResult plLerpPosesAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fLerp;
  stream << m_uiPosesCount;

  PL_SUCCEED_OR_RETURN(m_InLerp.Serialize(stream));
  PL_SUCCEED_OR_RETURN(stream.WriteArray(m_InPoses));
  PL_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));

  return PL_SUCCESS;
}

plResult plLerpPosesAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fLerp;
  stream >> m_uiPosesCount;

  PL_SUCCEED_OR_RETURN(m_InLerp.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(stream.ReadArray(m_InPoses));
  PL_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return PL_SUCCESS;
}

void plLerpPosesAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected())
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

  const float fIndex = plMath::Clamp((float)m_InLerp.GetNumber(ref_graph, m_fLerp), 0.0f, (float)pPins.GetCount() - 1.0f);

  if (plMath::Fraction(fIndex) == 0.0f)
  {
    const plAnimGraphLocalPoseInputPin* pPinToForward = pPins[(plInt32)plMath::Trunc(fIndex)];
    plAnimGraphPinDataLocalTransforms* pDataToForward = pPinToForward->GetPose(ref_controller, ref_graph);

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
    plAnimGraphPinDataLocalTransforms* pPinData = ref_controller.AddPinDataLocalTransforms();

    const float fLerp = plMath::Fraction(fIndex);

    auto pPose0 = pPins[(plInt32)plMath::Trunc(fIndex)]->GetPose(ref_controller, ref_graph);
    auto pPose1 = pPins[(plInt32)plMath::Trunc(fIndex) + 1]->GetPose(ref_controller, ref_graph);

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


PL_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes2_LerpPosesAnimNode);


#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/BoneWeightsSwitchAnimNode.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSwitchBoneWeightsAnimNode, 1, plRTTIDefaultAllocator<plSwitchBoneWeightsAnimNode>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("InIndex", m_InIndex)->AddAttributes(new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("WeightsCount", m_uiWeightsCount)->AddAttributes(new plNoTemporaryTransactionsAttribute(), new plDynamicPinAttribute(), new plDefaultValueAttribute(2)),
    PL_ARRAY_MEMBER_PROPERTY("InWeights", m_InWeights)->AddAttributes(new plHiddenAttribute(), new plDynamicPinAttribute("WeightsCount")),
    PL_MEMBER_PROPERTY("OutWeights", m_OutWeights)->AddAttributes(new plHiddenAttribute()),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Weights"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Teal)),
    new plTitleAttribute("Bone Weights Switch"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plSwitchBoneWeightsAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_uiWeightsCount;

  PL_SUCCEED_OR_RETURN(m_InIndex.Serialize(stream));
  PL_SUCCEED_OR_RETURN(stream.WriteArray(m_InWeights));
  PL_SUCCEED_OR_RETURN(m_OutWeights.Serialize(stream));

  return PL_SUCCESS;
}

plResult plSwitchBoneWeightsAnimNode::DeserializeNode(plStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);
  PL_IGNORE_UNUSED(version);

  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_uiWeightsCount;

  PL_SUCCEED_OR_RETURN(m_InIndex.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(stream.ReadArray(m_InWeights));
  PL_SUCCEED_OR_RETURN(m_OutWeights.Deserialize(stream));

  return PL_SUCCESS;
}

void plSwitchBoneWeightsAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (!m_OutWeights.IsConnected() || !m_InIndex.IsConnected() || m_InWeights.IsEmpty())
    return;

  const plInt32 iIndex = plMath::Clamp((plInt32)m_InIndex.GetNumber(ref_graph), 0, (plInt32)m_InWeights.GetCount() - 1);

  if (!m_InWeights[iIndex].IsConnected())
    return;

  m_OutWeights.SetWeights(ref_graph, m_InWeights[iIndex].GetWeights(ref_controller, ref_graph));
}


PL_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes2_BoneWeightsSwitchAnimNode);


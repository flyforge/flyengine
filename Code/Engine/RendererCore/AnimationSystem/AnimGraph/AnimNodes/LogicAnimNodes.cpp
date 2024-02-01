#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/LogicAnimNodes.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLogicAndAnimNode, 1, plRTTIDefaultAllocator<plLogicAndAnimNode>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("BoolCount", m_uiBoolCount)->AddAttributes(new plNoTemporaryTransactionsAttribute(), new plDynamicPinAttribute(), new plDefaultValueAttribute(2)),
    PL_ARRAY_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new plHiddenAttribute(), new plDynamicPinAttribute("BoolCount")),
    PL_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new plHiddenAttribute),
    PL_MEMBER_PROPERTY("OutIsFalse", m_OutIsFalse)->AddAttributes(new plHiddenAttribute),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Logic"),
    new plTitleAttribute("AND"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plLogicAndAnimNode::plLogicAndAnimNode() = default;
plLogicAndAnimNode::~plLogicAndAnimNode() = default;

plResult plLogicAndAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_uiBoolCount;
  PL_SUCCEED_OR_RETURN(stream.WriteArray(m_InBool));
  PL_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutIsFalse.Serialize(stream));

  return PL_SUCCESS;
}

plResult plLogicAndAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_uiBoolCount;
  PL_SUCCEED_OR_RETURN(stream.ReadArray(m_InBool));
  PL_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutIsFalse.Deserialize(stream));

  return PL_SUCCESS;
}

void plLogicAndAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  bool res = true;

  for (const auto& pin : m_InBool)
  {
    if (!pin.GetBool(ref_graph, true))
    {
      res = false;
      break;
    }
  }

  m_OutIsTrue.SetBool(ref_graph, res);
  m_OutIsFalse.SetBool(ref_graph, !res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLogicEventAndAnimNode, 1, plRTTIDefaultAllocator<plLogicEventAndAnimNode>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new plHiddenAttribute),
    PL_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new plHiddenAttribute),
    PL_MEMBER_PROPERTY("OutOnActivated", m_OutOnActivated)->AddAttributes(new plHiddenAttribute),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Logic"),
    new plTitleAttribute("Event AND"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plLogicEventAndAnimNode::plLogicEventAndAnimNode() = default;
plLogicEventAndAnimNode::~plLogicEventAndAnimNode() = default;

plResult plLogicEventAndAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  PL_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_InBool.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutOnActivated.Serialize(stream));

  return PL_SUCCESS;
}

plResult plLogicEventAndAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  PL_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_InBool.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutOnActivated.Deserialize(stream));

  return PL_SUCCESS;
}

void plLogicEventAndAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (m_InActivate.IsTriggered(ref_graph) && m_InBool.GetBool(ref_graph))
  {
    m_OutOnActivated.SetTriggered(ref_graph);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLogicOrAnimNode, 1, plRTTIDefaultAllocator<plLogicOrAnimNode>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("BoolCount", m_uiBoolCount)->AddAttributes(new plNoTemporaryTransactionsAttribute(), new plDynamicPinAttribute(), new plDefaultValueAttribute(2)),
    PL_ARRAY_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new plHiddenAttribute(), new plDynamicPinAttribute("BoolCount")),
    PL_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new plHiddenAttribute),
    PL_MEMBER_PROPERTY("OutIsFalse", m_OutIsFalse)->AddAttributes(new plHiddenAttribute),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Logic"),
    new plTitleAttribute("OR"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plLogicOrAnimNode::plLogicOrAnimNode() = default;
plLogicOrAnimNode::~plLogicOrAnimNode() = default;

plResult plLogicOrAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_uiBoolCount;
  PL_SUCCEED_OR_RETURN(stream.WriteArray(m_InBool));
  PL_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutIsFalse.Serialize(stream));

  return PL_SUCCESS;
}

plResult plLogicOrAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_uiBoolCount;
  PL_SUCCEED_OR_RETURN(stream.ReadArray(m_InBool));
  PL_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutIsFalse.Deserialize(stream));

  return PL_SUCCESS;
}

void plLogicOrAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  bool res = false;

  for (const auto& pin : m_InBool)
  {
    if (!pin.GetBool(ref_graph, true))
    {
      res = true;
      break;
    }
  }

  m_OutIsTrue.SetBool(ref_graph, res);
  m_OutIsFalse.SetBool(ref_graph, !res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLogicNotAnimNode, 1, plRTTIDefaultAllocator<plLogicNotAnimNode>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new plHiddenAttribute),
    PL_MEMBER_PROPERTY("OutBool", m_OutBool)->AddAttributes(new plHiddenAttribute),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Logic"),
    new plTitleAttribute("NOT"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plLogicNotAnimNode::plLogicNotAnimNode() = default;
plLogicNotAnimNode::~plLogicNotAnimNode() = default;

plResult plLogicNotAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  PL_SUCCEED_OR_RETURN(m_InBool.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutBool.Serialize(stream));

  return PL_SUCCESS;
}

plResult plLogicNotAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  PL_SUCCEED_OR_RETURN(m_InBool.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutBool.Deserialize(stream));

  return PL_SUCCESS;
}

void plLogicNotAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  const bool value = !m_InBool.GetBool(ref_graph);

  m_OutBool.SetBool(ref_graph, !value);
}

PL_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_LogicAnimNodes);

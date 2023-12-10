#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/LogicAnimNodes.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLogicAndAnimNode, 1, plRTTIDefaultAllocator<plLogicAndAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("BoolCount", m_uiBoolCount)->AddAttributes(new plNoTemporaryTransactionsAttribute(), new plDynamicPinAttribute(), new plDefaultValueAttribute(2)),
    PLASMA_ARRAY_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new plHiddenAttribute(), new plDynamicPinAttribute("BoolCount")),
    PLASMA_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new plHiddenAttribute),
    PLASMA_MEMBER_PROPERTY("OutIsFalse", m_OutIsFalse)->AddAttributes(new plHiddenAttribute),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Logic"),
    new plTitleAttribute("AND"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plLogicAndAnimNode::plLogicAndAnimNode() = default;
plLogicAndAnimNode::~plLogicAndAnimNode() = default;

plResult plLogicAndAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_uiBoolCount;
  PLASMA_SUCCEED_OR_RETURN(stream.WriteArray(m_InBool));
  PLASMA_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutIsFalse.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plLogicAndAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_uiBoolCount;
  PLASMA_SUCCEED_OR_RETURN(stream.ReadArray(m_InBool));
  PLASMA_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutIsFalse.Deserialize(stream));

  return PLASMA_SUCCESS;
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
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLogicEventAndAnimNode, 1, plRTTIDefaultAllocator<plLogicEventAndAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new plHiddenAttribute),
    PLASMA_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new plHiddenAttribute),
    PLASMA_MEMBER_PROPERTY("OutOnActivated", m_OutOnActivated)->AddAttributes(new plHiddenAttribute),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Logic"),
    new plTitleAttribute("Event AND"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plLogicEventAndAnimNode::plLogicEventAndAnimNode() = default;
plLogicEventAndAnimNode::~plLogicEventAndAnimNode() = default;

plResult plLogicEventAndAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  PLASMA_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InBool.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnActivated.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plLogicEventAndAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  PLASMA_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InBool.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnActivated.Deserialize(stream));

  return PLASMA_SUCCESS;
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
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLogicOrAnimNode, 1, plRTTIDefaultAllocator<plLogicOrAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("BoolCount", m_uiBoolCount)->AddAttributes(new plNoTemporaryTransactionsAttribute(), new plDynamicPinAttribute(), new plDefaultValueAttribute(2)),
    PLASMA_ARRAY_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new plHiddenAttribute(), new plDynamicPinAttribute("BoolCount")),
    PLASMA_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new plHiddenAttribute),
    PLASMA_MEMBER_PROPERTY("OutIsFalse", m_OutIsFalse)->AddAttributes(new plHiddenAttribute),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Logic"),
    new plTitleAttribute("OR"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plLogicOrAnimNode::plLogicOrAnimNode() = default;
plLogicOrAnimNode::~plLogicOrAnimNode() = default;

plResult plLogicOrAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_uiBoolCount;
  PLASMA_SUCCEED_OR_RETURN(stream.WriteArray(m_InBool));
  PLASMA_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutIsFalse.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plLogicOrAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_uiBoolCount;
  PLASMA_SUCCEED_OR_RETURN(stream.ReadArray(m_InBool));
  PLASMA_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutIsFalse.Deserialize(stream));

  return PLASMA_SUCCESS;
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
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLogicNotAnimNode, 1, plRTTIDefaultAllocator<plLogicNotAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new plHiddenAttribute),
    PLASMA_MEMBER_PROPERTY("OutBool", m_OutBool)->AddAttributes(new plHiddenAttribute),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Logic"),
    new plTitleAttribute("NOT"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plLogicNotAnimNode::plLogicNotAnimNode() = default;
plLogicNotAnimNode::~plLogicNotAnimNode() = default;

plResult plLogicNotAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  PLASMA_SUCCEED_OR_RETURN(m_InBool.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutBool.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plLogicNotAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  PLASMA_SUCCEED_OR_RETURN(m_InBool.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutBool.Deserialize(stream));

  return PLASMA_SUCCESS;
}

void plLogicNotAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  const bool value = !m_InBool.GetBool(ref_graph);

  m_OutBool.SetBool(ref_graph, !value);
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_LogicAnimNodes);

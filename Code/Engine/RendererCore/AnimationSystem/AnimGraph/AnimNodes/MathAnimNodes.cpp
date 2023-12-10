#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/MathAnimNodes.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMathExpressionAnimNode, 1, plRTTIDefaultAllocator<plMathExpressionAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Expression", GetExpression, SetExpression)->AddAttributes(new plDefaultValueAttribute("a*a + (b-c) / abs(d)")),
    PLASMA_MEMBER_PROPERTY("a", m_ValueAPin)->AddAttributes(new plHiddenAttribute),
    PLASMA_MEMBER_PROPERTY("b", m_ValueBPin)->AddAttributes(new plHiddenAttribute),
    PLASMA_MEMBER_PROPERTY("c", m_ValueCPin)->AddAttributes(new plHiddenAttribute),
    PLASMA_MEMBER_PROPERTY("d", m_ValueDPin)->AddAttributes(new plHiddenAttribute),
    PLASMA_MEMBER_PROPERTY("Result", m_ResultPin)->AddAttributes(new plHiddenAttribute),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Math"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Lime)),
    new plTitleAttribute("= {Expression}"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plMathExpressionAnimNode::plMathExpressionAnimNode() = default;
plMathExpressionAnimNode::~plMathExpressionAnimNode() = default;

void plMathExpressionAnimNode::SetExpression(plString sExpr)
{
  m_sExpression = sExpr;
}

plString plMathExpressionAnimNode::GetExpression() const
{
  return m_sExpression;
}

plResult plMathExpressionAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sExpression;
  PLASMA_SUCCEED_OR_RETURN(m_ValueAPin.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_ValueBPin.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_ValueCPin.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_ValueDPin.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_ResultPin.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plMathExpressionAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sExpression;
  PLASMA_SUCCEED_OR_RETURN(m_ValueAPin.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_ValueBPin.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_ValueCPin.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_ValueDPin.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_ResultPin.Deserialize(stream));

  return PLASMA_SUCCESS;
}

static plHashedString s_sA = plMakeHashedString("a");
static plHashedString s_sB = plMakeHashedString("b");
static plHashedString s_sC = plMakeHashedString("c");
static plHashedString s_sD = plMakeHashedString("d");

void plMathExpressionAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  if (pInstance->m_mExpression.GetExpressionString().IsEmpty())
  {
    pInstance->m_mExpression.Reset(m_sExpression);
  }

  if (!pInstance->m_mExpression.IsValid())
  {
    m_ResultPin.SetNumber(ref_graph, 0);
    return;
  }

  plMathExpression::Input inputs[] =
    {
      {s_sA, static_cast<float>(m_ValueAPin.GetNumber(ref_graph))},
      {s_sB, static_cast<float>(m_ValueBPin.GetNumber(ref_graph))},
      {s_sC, static_cast<float>(m_ValueCPin.GetNumber(ref_graph))},
      {s_sD, static_cast<float>(m_ValueDPin.GetNumber(ref_graph))},
    };

  float result = pInstance->m_mExpression.Evaluate(inputs);
  m_ResultPin.SetNumber(ref_graph, result);
}

bool plMathExpressionAnimNode::GetInstanceDataDesc(plInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCompareNumberAnimNode, 1, plRTTIDefaultAllocator<plCompareNumberAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ReferenceValue", m_fReferenceValue),
    PLASMA_ENUM_MEMBER_PROPERTY("Comparison", plComparisonOperator, m_Comparison),

    PLASMA_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("InNumber", m_InNumber)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("InReference", m_InReference)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Logic"),
    new plTitleAttribute("Compare: Number {Comparison} {ReferenceValue}"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Lime)),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plCompareNumberAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fReferenceValue;
  stream << m_Comparison;

  PLASMA_SUCCEED_OR_RETURN(m_InNumber.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InReference.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plCompareNumberAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fReferenceValue;
  stream >> m_Comparison;

  PLASMA_SUCCEED_OR_RETURN(m_InNumber.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InReference.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));

  return PLASMA_SUCCESS;
}

void plCompareNumberAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  const bool bIsTrue = plComparisonOperator::Compare<double>(m_Comparison, m_InNumber.GetNumber(ref_graph), m_InReference.GetNumber(ref_graph, m_fReferenceValue));

  m_OutIsTrue.SetBool(ref_graph, bIsTrue);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plBoolToNumberAnimNode, 1, plRTTIDefaultAllocator<plBoolToNumberAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("FalseValue", m_fFalseValue)->AddAttributes(new plDefaultValueAttribute(0.0)),
    PLASMA_MEMBER_PROPERTY("TrueValue", m_fTrueValue)->AddAttributes(new plDefaultValueAttribute(1.0)),
    PLASMA_MEMBER_PROPERTY("InValue", m_InValue)->AddAttributes(new plHiddenAttribute),
    PLASMA_MEMBER_PROPERTY("OutNumber", m_OutNumber)->AddAttributes(new plHiddenAttribute),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Logic"),
    new plTitleAttribute("Bool To Number"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plBoolToNumberAnimNode::plBoolToNumberAnimNode() = default;
plBoolToNumberAnimNode::~plBoolToNumberAnimNode() = default;

plResult plBoolToNumberAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fFalseValue;
  stream << m_fTrueValue;

  PLASMA_SUCCEED_OR_RETURN(m_InValue.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutNumber.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plBoolToNumberAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fFalseValue;
  stream >> m_fTrueValue;

  PLASMA_SUCCEED_OR_RETURN(m_InValue.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutNumber.Deserialize(stream));

  return PLASMA_SUCCESS;
}

void plBoolToNumberAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  m_OutNumber.SetNumber(ref_graph, m_InValue.GetBool(ref_graph) ? m_fTrueValue : m_fFalseValue);
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_MathAnimNodes);

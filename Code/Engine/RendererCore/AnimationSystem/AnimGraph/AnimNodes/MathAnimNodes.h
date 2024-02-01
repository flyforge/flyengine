#pragma once

#include <Foundation/CodeUtils/MathExpression.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class PL_RENDERERCORE_DLL plMathExpressionAnimNode : public plAnimGraphNode
{
  PL_ADD_DYNAMIC_REFLECTION(plMathExpressionAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // plLogicAndAnimNode

public:
  plMathExpressionAnimNode();
  ~plMathExpressionAnimNode();

  void SetExpression(plString sExpr);
  plString GetExpression() const;

private:
  plAnimGraphNumberInputPin m_ValueAPin;  // [ property ]
  plAnimGraphNumberInputPin m_ValueBPin;  // [ property ]
  plAnimGraphNumberInputPin m_ValueCPin;  // [ property ]
  plAnimGraphNumberInputPin m_ValueDPin;  // [ property ]
  plAnimGraphNumberOutputPin m_ResultPin; // [ property ]

  plString m_sExpression;

  struct InstanceData
  {
    plMathExpression m_mExpression;
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_RENDERERCORE_DLL plCompareNumberAnimNode : public plAnimGraphNode
{
  PL_ADD_DYNAMIC_REFLECTION(plCompareNumberAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plCompareNumberAnimNode

public:
  double m_fReferenceValue = 0.0f;           // [ property ]
  plEnum<plComparisonOperator> m_Comparison; // [ property ]

private:
  plAnimGraphNumberInputPin m_InNumber;    // [ property ]
  plAnimGraphNumberInputPin m_InReference; // [ property ]
  plAnimGraphBoolOutputPin m_OutIsTrue;    // [ property ]
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_RENDERERCORE_DLL plBoolToNumberAnimNode : public plAnimGraphNode
{
  PL_ADD_DYNAMIC_REFLECTION(plBoolToNumberAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plBoolToNumberAnimNode

public:
  plBoolToNumberAnimNode();
  ~plBoolToNumberAnimNode();

  double m_fFalseValue = 0.0f;
  double m_fTrueValue = 1.0f;

private:
  plAnimGraphBoolInputPin m_InValue;      // [ property ]
  plAnimGraphNumberOutputPin m_OutNumber; // [ property ]
};

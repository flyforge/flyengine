#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class PL_RENDERERCORE_DLL plLogicAndAnimNode : public plAnimGraphNode
{
  PL_ADD_DYNAMIC_REFLECTION(plLogicAndAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plLogicAndAnimNode

public:
  plLogicAndAnimNode();
  ~plLogicAndAnimNode();

private:
  plUInt8 m_uiBoolCount = 2;                          // [ property ]
  plHybridArray<plAnimGraphBoolInputPin, 2> m_InBool; // [ property ]
  plAnimGraphBoolOutputPin m_OutIsTrue;               // [ property ]
  plAnimGraphBoolOutputPin m_OutIsFalse;              // [ property ]
};

class PL_RENDERERCORE_DLL plLogicEventAndAnimNode : public plAnimGraphNode
{
  PL_ADD_DYNAMIC_REFLECTION(plLogicEventAndAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plLogicEventAndAnimNode

public:
  plLogicEventAndAnimNode();
  ~plLogicEventAndAnimNode();

private:
  plAnimGraphTriggerInputPin m_InActivate;      // [ property ]
  plAnimGraphBoolInputPin m_InBool;             // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnActivated; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_RENDERERCORE_DLL plLogicOrAnimNode : public plAnimGraphNode
{
  PL_ADD_DYNAMIC_REFLECTION(plLogicOrAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plLogicOrAnimNode

public:
  plLogicOrAnimNode();
  ~plLogicOrAnimNode();

private:
  plUInt8 m_uiBoolCount = 2;                          // [ property ]
  plHybridArray<plAnimGraphBoolInputPin, 2> m_InBool; // [ property ]
  plAnimGraphBoolOutputPin m_OutIsTrue;               // [ property ]
  plAnimGraphBoolOutputPin m_OutIsFalse;              // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_RENDERERCORE_DLL plLogicNotAnimNode : public plAnimGraphNode
{
  PL_ADD_DYNAMIC_REFLECTION(plLogicNotAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plLogicNotAnimNode

public:
  plLogicNotAnimNode();
  ~plLogicNotAnimNode();

private:
  plAnimGraphBoolInputPin m_InBool;   // [ property ]
  plAnimGraphBoolOutputPin m_OutBool; // [ property ]
};

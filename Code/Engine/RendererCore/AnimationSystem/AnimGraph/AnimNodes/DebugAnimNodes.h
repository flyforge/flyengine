#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class PLASMA_RENDERERCORE_DLL plLogAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLogAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plLogAnimNode

protected:
  plString m_sText;                                        // [ property ]
  plAnimGraphTriggerInputPin m_InActivate;                 // [ property ]
  plUInt8 m_uiNumberCount = 1;                             // [ property ]
  plHybridArray<plAnimGraphNumberInputPin, 2> m_InNumbers; // [ property ]
};

class PLASMA_RENDERERCORE_DLL plLogInfoAnimNode : public plLogAnimNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLogInfoAnimNode, plLogAnimNode);

  //////////////////////////////////////////////////////////////////////////
  // plLogAnimNode

protected:
  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;
};

class PLASMA_RENDERERCORE_DLL plLogErrorAnimNode : public plLogAnimNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLogErrorAnimNode, plLogAnimNode);

  //////////////////////////////////////////////////////////////////////////
  // plLogAnimNode

protected:
  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;
};

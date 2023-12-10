#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class PLASMA_RENDERERCORE_DLL plSwitchBoneWeightsAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSwitchBoneWeightsAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plSwitchBoneWeightsAnimNode

private:
  plAnimGraphNumberInputPin m_InIndex;                          // [ property ]
  plUInt8 m_uiWeightsCount = 0;                                 // [ property ]
  plHybridArray<plAnimGraphBoneWeightsInputPin, 2> m_InWeights; // [ property ]
  plAnimGraphBoneWeightsOutputPin m_OutWeights;                 // [ property ]
};

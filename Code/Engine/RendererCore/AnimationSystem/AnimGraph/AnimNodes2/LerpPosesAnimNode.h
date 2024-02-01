#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class PL_RENDERERCORE_DLL plLerpPosesAnimNode : public plAnimGraphNode
{
  PL_ADD_DYNAMIC_REFLECTION(plLerpPosesAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plLerpPosesAnimNode

public:
  plLerpPosesAnimNode();
  ~plLerpPosesAnimNode();

  float m_fLerp = 0.5f; // [ property ]

private:
  plUInt8 m_uiPosesCount = 0;                               // [ property ]
  plHybridArray<plAnimGraphLocalPoseInputPin, 2> m_InPoses; // [ property ]
  plAnimGraphNumberInputPin m_InLerp;                       // [ property ]
  plAnimGraphLocalPoseOutputPin m_OutPose;                  // [ property ]
};

#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class PL_RENDERERCORE_DLL plSwitchPoseAnimNode : public plAnimGraphNode
{
  PL_ADD_DYNAMIC_REFLECTION(plSwitchPoseAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // plSelectPoseAnimNode

private:
  plTime m_TransitionDuration = plTime::MakeFromMilliseconds(200); // [ property ]
  plUInt8 m_uiPosesCount = 0;                               // [ property ]
  plHybridArray<plAnimGraphLocalPoseInputPin, 4> m_InPoses; // [ property ]
  plAnimGraphNumberInputPin m_InIndex;                      // [ property ]
  plAnimGraphLocalPoseOutputPin m_OutPose;                  // [ property ]

  struct InstanceData
  {
    plTime m_TransitionTime;
    plInt8 m_iTransitionFromIndex = -1;
    plInt8 m_iTransitionToIndex = -1;
  };
};

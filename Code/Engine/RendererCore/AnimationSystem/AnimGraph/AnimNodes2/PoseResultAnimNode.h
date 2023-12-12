#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class PLASMA_RENDERERCORE_DLL plPoseResultAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPoseResultAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // plPoseResultAnimNode

public:
  plPoseResultAnimNode();
  ~plPoseResultAnimNode();

private:
  plTime m_FadeDuration = plTime::Milliseconds(200); // [ property ]

  plAnimGraphLocalPoseInputPin m_InPose;         // [ property ]
  plAnimGraphNumberInputPin m_InTargetWeight;    // [ property ]
  plAnimGraphNumberInputPin m_InFadeDuration;    // [ property ]
  plAnimGraphBoneWeightsInputPin m_InWeights;    // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnFadedOut;   // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnFadedIn;    // [ property ]
  plAnimGraphNumberOutputPin m_OutCurrentWeight; // [ property ]

  struct InstanceData
  {
    float m_fStartWeight = 1.0f;
    float m_fEndWeight = 1.0f;
    plTime m_PlayTime;
    plTime m_EndTime;
  };
};

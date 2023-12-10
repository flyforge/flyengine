#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

class PLASMA_RENDERERCORE_DLL plSampleFrameAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSampleFrameAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plSampleFrameAnimNode

public:
  void SetClip(const char* szClip);
  const char* GetClip() const;

  plHashedString m_sClip;                   // [ property ]
  float m_fNormalizedSamplePosition = 0.0f; // [ property ]

private:
  plAnimGraphNumberInputPin m_InNormalizedSamplePosition; // [ property ]
  plAnimGraphNumberInputPin m_InAbsoluteSamplePosition;   // [ property ]
  plAnimGraphLocalPoseOutputPin m_OutPose;                // [ property ]
};

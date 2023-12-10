#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

class PLASMA_RENDERERCORE_DLL plSampleAnimClipAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSampleAnimClipAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // plSampleAnimClipAnimNode

  void SetClip(const char* szClip);
  const char* GetClip() const;

public:
  plSampleAnimClipAnimNode();
  ~plSampleAnimClipAnimNode();

private:
  plHashedString m_sClip;          // [ property ]
  bool m_bLoop = true;             // [ property ]
  bool m_bApplyRootMotion = false; // [ property ]
  float m_fPlaybackSpeed = 1.0f;   // [ property ]

  plAnimGraphTriggerInputPin m_InStart; // [ property ]
  plAnimGraphBoolInputPin m_InLoop;     // [ property ]
  plAnimGraphNumberInputPin m_InSpeed;  // [ property ]

  plAnimGraphLocalPoseOutputPin m_OutPose;     // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnStarted;  // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnFinished; // [ property ]

  struct InstanceState
  {
    bool m_bPlaying = false;
    plTime m_PlaybackTime;
  };
};

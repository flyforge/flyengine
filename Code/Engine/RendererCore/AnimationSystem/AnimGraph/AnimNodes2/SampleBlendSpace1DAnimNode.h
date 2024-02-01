#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

struct PL_RENDERERCORE_DLL plAnimationClip1D
{
  plHashedString m_sClip;
  float m_fPosition = 0.0f;
  float m_fSpeed = 1.0f;

  void SetAnimationFile(const char* szFile);
  const char* GetAnimationFile() const;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plAnimationClip1D);

class PL_RENDERERCORE_DLL plSampleBlendSpace1DAnimNode : public plAnimGraphNode
{
  PL_ADD_DYNAMIC_REFLECTION(plSampleBlendSpace1DAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // plSampleBlendSpace1DAnimNode

public:
  plSampleBlendSpace1DAnimNode();
  ~plSampleBlendSpace1DAnimNode();

private:
  plHybridArray<plAnimationClip1D, 4> m_Clips; // [ property ]
  bool m_bLoop = true;                         // [ property ]
  bool m_bApplyRootMotion = false;             // [ property ]
  float m_fPlaybackSpeed = 1.0f;               // [ property ]

  plAnimGraphTriggerInputPin m_InStart;        // [ property ]
  plAnimGraphBoolInputPin m_InLoop;            // [ property ]
  plAnimGraphNumberInputPin m_InSpeed;         // [ property ]
  plAnimGraphNumberInputPin m_InLerp;          // [ property ]
  plAnimGraphLocalPoseOutputPin m_OutPose;     // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnStarted;  // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnFinished; // [ property ]


  struct InstanceState
  {
    plTime m_PlaybackTime;
    bool m_bPlaying = false;
  };
};

#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

struct PLASMA_RENDERERCORE_DLL plAnimationClip2D
{
  plHashedString m_sClip;
  plVec2 m_vPosition;

  void SetAnimationFile(const char* szFile);
  const char* GetAnimationFile() const;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plAnimationClip2D);

class PLASMA_RENDERERCORE_DLL plSampleBlendSpace2DAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSampleBlendSpace2DAnimNode, plAnimGraphNode);

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
  plSampleBlendSpace2DAnimNode();
  ~plSampleBlendSpace2DAnimNode();

  void SetCenterClipFile(const char* szFile);
  const char* GetCenterClipFile() const;

private:
  plHashedString m_sCenterClip;                       // [ property ]
  plHybridArray<plAnimationClip2D, 8> m_Clips;        // [ property ]
  plTime m_InputResponse = plTime::Milliseconds(100); // [ property ]
  bool m_bLoop = true;                                // [ property ]
  bool m_bApplyRootMotion = false;                    // [ property ]
  float m_fPlaybackSpeed = 1.0f;                      // [ property ]

  plAnimGraphTriggerInputPin m_InStart;        // [ property ]
  plAnimGraphBoolInputPin m_InLoop;            // [ property ]
  plAnimGraphNumberInputPin m_InSpeed;         // [ property ]
  plAnimGraphNumberInputPin m_InCoordX;        // [ property ]
  plAnimGraphNumberInputPin m_InCoordY;        // [ property ]
  plAnimGraphLocalPoseOutputPin m_OutPose;     // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnStarted;  // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnFinished; // [ property ]

  struct ClipToPlay
  {
    PLASMA_DECLARE_POD_TYPE();

    plUInt32 m_uiIndex;
    float m_fWeight = 1.0f;
  };

  struct InstanceState
  {
    bool m_bPlaying = false;
    plTime m_CenterPlaybackTime;
    float m_fOtherPlaybackPosNorm = 0.0f;
    float m_fLastValueX = 0.0f;
    float m_fLastValueY = 0.0f;
  };

  void UpdateCenterClipPlaybackTime(const plAnimController::AnimClipInfo& centerInfo, InstanceState* pState, plAnimGraphInstance& ref_graph, plTime tDiff, plAnimPoseEventTrackSampleMode& out_eventSamplingCenter) const;
  void PlayClips(plAnimController& ref_controller, const plAnimController::AnimClipInfo& centerInfo, InstanceState* pState, plAnimGraphInstance& ref_graph, plTime tDiff, plArrayPtr<ClipToPlay> clips, plUInt32 uiMaxWeightClip) const;
  void ComputeClipsAndWeights(const plAnimController::AnimClipInfo& centerInfo, const plVec2& p, plDynamicArray<ClipToPlay>& out_Clips, plUInt32& out_uiMaxWeightClip) const;
};

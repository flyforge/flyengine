#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

class PLASMA_RENDERERCORE_DLL plSampleAnimClipSequenceAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSampleAnimClipSequenceAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // plSampleAnimClipSequenceAnimNode

public:
  plSampleAnimClipSequenceAnimNode();
  ~plSampleAnimClipSequenceAnimNode();

  void SetStartClip(const char* szClip);
  const char* GetStartClip() const;

  plUInt32 Clips_GetCount() const;                            // [ property ]
  const char* Clips_GetValue(plUInt32 uiIndex) const;         // [ property ]
  void Clips_SetValue(plUInt32 uiIndex, const char* szValue); // [ property ]
  void Clips_Insert(plUInt32 uiIndex, const char* szValue);   // [ property ]
  void Clips_Remove(plUInt32 uiIndex);                        // [ property ]

  void SetEndClip(const char* szClip);
  const char* GetEndClip() const;

private:
  plHashedString m_sStartClip;              // [ property ]
  plHybridArray<plHashedString, 1> m_Clips; // [ property ]
  plHashedString m_sEndClip;                // [ property ]
  bool m_bApplyRootMotion = false;          // [ property ]
  bool m_bLoop = false;                     // [ property ]
  float m_fPlaybackSpeed = 1.0f;            // [ property ]

  plAnimGraphTriggerInputPin m_InStart;     // [ property ]
  plAnimGraphBoolInputPin m_InLoop;         // [ property ]
  plAnimGraphNumberInputPin m_InSpeed;      // [ property ]
  plAnimGraphNumberInputPin m_ClipIndexPin; // [ property ]

  plAnimGraphLocalPoseOutputPin m_OutPose;          // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnMiddleStarted; // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnEndStarted;    // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnFinished;      // [ property ]

  struct InstanceState
  {
    plTime m_PlaybackTime;
    plUInt8 m_uiState = 0; // 0 = off, 1 = start, 2 = middle, 3 = end
    plUInt8 m_uiMiddleClipIdx = 0;
  };
};

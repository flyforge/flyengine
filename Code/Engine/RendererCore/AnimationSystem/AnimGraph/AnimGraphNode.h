#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Time/Time.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

class plSkeletonResource;
class plGameObject;
class plAnimGraphInstance;
class plAnimController;
class plStreamWriter;
class plStreamReader;
struct plAnimGraphPinDataLocalTransforms;
struct plAnimGraphPinDataBoneWeights;
class plAnimationClipResource;
struct plInstanceDataDesc;

using plAnimationClipResourceHandle = plTypedResourceHandle<class plAnimationClipResource>;

namespace ozz
{
  namespace animation
  {
    class Animation;
  }
} // namespace ozz

/// \brief Base class for all nodes in an plAnimGraphInstance
///
/// These nodes are used to configure which skeletal animations can be played on an object,
/// and how they would be played back exactly.
/// The nodes implement different functionality. For example logic nodes are used to figure out how to play an animation,
/// other nodes then sample and combining animation poses, and yet other nodes can inform the user about events
/// or they write state back to the animation graph's blackboard.
class PL_RENDERERCORE_DLL plAnimGraphNode : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphNode, plReflectedClass);

public:
  plAnimGraphNode();
  virtual ~plAnimGraphNode();

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

  const char* GetCustomNodeTitle() const { return m_sCustomNodeTitle.GetString(); }
  void SetCustomNodeTitle(const char* szSz) { m_sCustomNodeTitle.Assign(szSz); }

protected:
  friend class plAnimGraphInstance;
  friend class plAnimGraph;
  friend class plAnimGraphResource;

  plHashedString m_sCustomNodeTitle;
  plUInt32 m_uiInstanceDataOffset = plInvalidIndex;

  virtual plResult SerializeNode(plStreamWriter& stream) const = 0;
  virtual plResult DeserializeNode(plStreamReader& stream) = 0;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const = 0;
  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) const { return false; }
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct PL_RENDERERCORE_DLL plAnimState
{
  enum class State
  {
    Off,
    StartedRampUp,
    RampingUp,
    Running,
    StartedRampDown,
    RampingDown,
    Finished,
  };

  // Properties:
  plTime m_FadeIn;                  // [ property ]
  plTime m_FadeOut;                 // [ property ]
  bool m_bImmediateFadeIn = false;  // [ property ]
  bool m_bImmediateFadeOut = false; // [ property ]
  bool m_bLoop = false;             // [ property ]
  float m_fPlaybackSpeed = 1.0f;    // [ property ]
  bool m_bApplyRootMotion = false;  // [ property ]

  // Inputs:
  bool m_bTriggerActive = false;
  float m_fPlaybackSpeedFactor = 1.0f;
  plTime m_Duration;
  plTime m_DurationOfQueued;

  bool WillStateBeOff(bool bTriggerActive) const;
  void UpdateState(plTime diff);
  State GetCurrentState() const { return m_State; }
  float GetWeight() const { return m_fCurWeight; }
  float GetNormalizedPlaybackPosition() const { return m_fNormalizedPlaybackPosition; }
  bool HasTransitioned() const { return m_bHasTransitioned; }
  bool HasLoopedStart() const { return m_bHasLoopedStart; }
  bool HasLoopedEnd() const { return m_bHasLoopedEnd; }
  float GetFinalSpeed() const { return m_fPlaybackSpeed * m_fPlaybackSpeedFactor; }

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);

private:
  void RampWeightUpOrDown(float& inout_fWeight, float fTargetWeight, plTime tDiff) const;

  State m_State = State::Off;
  float m_fNormalizedPlaybackPosition = 0.0f;
  bool m_bRequireLoopForRampDown = true;
  bool m_bHasTransitioned = false;
  bool m_bHasLoopedStart = false;
  bool m_bHasLoopedEnd = false;
  float m_fCurWeight = 0.0f;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plAnimState);

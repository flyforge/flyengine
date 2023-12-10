#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/RendererCoreDLL.h>

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/maths/soa_float.h>
#include <ozz/base/maths/soa_transform.h>

PLASMA_DEFINE_AS_POD_TYPE(ozz::math::SoaTransform);

class plSkeletonResource;
class plAnimPoseGenerator;
class plGameObject;

using plAnimationClipResourceHandle = plTypedResourceHandle<class plAnimationClipResource>;

using plAnimPoseGeneratorLocalPoseID = plUInt32;
using plAnimPoseGeneratorModelPoseID = plUInt32;
using plAnimPoseGeneratorCommandID = plUInt32;

/// \brief The type of plAnimPoseGeneratorCommand
enum class plAnimPoseGeneratorCommandType
{
  Invalid,
  SampleTrack,
  RestPose,
  CombinePoses,
  LocalToModelPose,
  ModelPoseToOutput,
  SampleEventTrack,
};

enum class plAnimPoseEventTrackSampleMode : plUInt8
{
  None,         ///< Don't sample the event track at all
  OnlyBetween,  ///< Sample the event track only between PrevSamplePos and SamplePos
  LoopAtEnd,    ///< Sample the event track between PrevSamplePos and End, then Start and SamplePos
  LoopAtStart,  ///< Sample the event track between PrevSamplePos and Start, then End and SamplePos
  BounceAtEnd,  ///< Sample the event track between PrevSamplePos and End, then End and SamplePos
  BounceAtStart ///< Sample the event track between PrevSamplePos and Start, then Start and SamplePos
};

/// \brief Base class for all pose generator commands
///
/// All commands have a unique command ID with which they are referenced.
/// All commands can have zero or N other commands set as *inputs*.
/// Every type of command only accepts certain types and amount of inputs.
///
/// The pose generation graph is built by allocating commands on the graph and then setting up
/// which command is an input to which other node.
/// A command can be an input to multiple other commands. It will be evaluated only once.
struct PLASMA_RENDERERCORE_DLL plAnimPoseGeneratorCommand
{
  plHybridArray<plAnimPoseGeneratorCommandID, 4> m_Inputs;

  plAnimPoseGeneratorCommandID GetCommandID() const { return m_CommandID; }
  plAnimPoseGeneratorCommandType GetType() const { return m_Type; }

private:
  friend class plAnimPoseGenerator;

  bool m_bExecuted = false;
  plAnimPoseGeneratorCommandID m_CommandID = plInvalidIndex;
  plAnimPoseGeneratorCommandType m_Type = plAnimPoseGeneratorCommandType::Invalid;
};

/// \brief Returns the rest pose (also often called 'bind pose').
///
/// The command has to be added as an input to one of
/// * plAnimPoseGeneratorCommandCombinePoses
/// * plAnimPoseGeneratorCommandLocalToModelPose
struct PLASMA_RENDERERCORE_DLL plAnimPoseGeneratorCommandRestPose final : public plAnimPoseGeneratorCommand
{
private:
  friend class plAnimPoseGenerator;

  plAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = plInvalidIndex;
};

/// \brief Samples an animation clip at a given time and optionally also its event track.
///
/// The command has to be added as an input to one of
/// * plAnimPoseGeneratorCommandCombinePoses
/// * plAnimPoseGeneratorCommandLocalToModelPose
///
/// If the event track shall be sampled as well, event messages are sent to the plGameObject for which the pose is generated.
///
/// This command can optionally have input commands of type plAnimPoseGeneratorCommandSampleEventTrack.
struct PLASMA_RENDERERCORE_DLL plAnimPoseGeneratorCommandSampleTrack final : public plAnimPoseGeneratorCommand
{
  plAnimationClipResourceHandle m_hAnimationClip;
  float m_fNormalizedSamplePos;
  float m_fPreviousNormalizedSamplePos;

  plAnimPoseEventTrackSampleMode m_EventSampling = plAnimPoseEventTrackSampleMode::None;

private:
  friend class plAnimPoseGenerator;

  bool m_bAdditive = false;
  plUInt32 m_uiUniqueID = 0;
  plAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = plInvalidIndex;
};

/// \brief Combines all the local space poses that are given as input into one local pose.
///
/// The input commands must be of type
/// * plAnimPoseGeneratorCommandSampleTrack
/// * plAnimPoseGeneratorCommandCombinePoses
/// * plAnimPoseGeneratorCommandRestPose
///
/// Every input pose gets both an overall weight, as well as optionally a per-bone weight mask.
/// If a per-bone mask is used, the respective input pose will only affect those bones.
struct PLASMA_RENDERERCORE_DLL plAnimPoseGeneratorCommandCombinePoses final : public plAnimPoseGeneratorCommand
{
  plHybridArray<float, 4> m_InputWeights;
  plHybridArray<plArrayPtr<const ozz::math::SimdFloat4>, 4> m_InputBoneWeights;

private:
  friend class plAnimPoseGenerator;

  plAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = plInvalidIndex;
};

/// \brief Accepts a single input in local space and converts it to model space.
///
/// The input command must be of type
/// * plAnimPoseGeneratorCommandSampleTrack
/// * plAnimPoseGeneratorCommandCombinePoses
/// * plAnimPoseGeneratorCommandRestPose
struct PLASMA_RENDERERCORE_DLL plAnimPoseGeneratorCommandLocalToModelPose final : public plAnimPoseGeneratorCommand
{
  plGameObject* m_pSendLocalPoseMsgTo = nullptr;

private:
  friend class plAnimPoseGenerator;

  plAnimPoseGeneratorModelPoseID m_ModelPoseOutput = plInvalidIndex;
};

/// \brief Accepts a single input command that outputs a model space pose and forwards it to the plGameObject for which the pose is generated.
///
/// The input command must be of type
/// * plAnimPoseGeneratorCommandLocalToModelPose
///
/// Every graph should have exactly one of these nodes. Commands that are not (indirectly) connected to an
/// output node will not be evaluated and won't have any effect.
struct PLASMA_RENDERERCORE_DLL plAnimPoseGeneratorCommandModelPoseToOutput final : public plAnimPoseGeneratorCommand
{
};

/// \brief Samples the event track of an animation clip but doesn't generate an animation pose.
///
/// Commands of this type can be added as inputs to commands of type
/// * plAnimPoseGeneratorCommandSampleTrack
/// * plAnimPoseGeneratorCommandSampleEventTrack
///
/// They are used to sample event tracks only.
struct PLASMA_RENDERERCORE_DLL plAnimPoseGeneratorCommandSampleEventTrack final : public plAnimPoseGeneratorCommand
{
  plAnimationClipResourceHandle m_hAnimationClip;
  float m_fNormalizedSamplePos;
  float m_fPreviousNormalizedSamplePos;

  plAnimPoseEventTrackSampleMode m_EventSampling = plAnimPoseEventTrackSampleMode::None;

private:
  friend class plAnimPoseGenerator;

  plUInt32 m_uiUniqueID = 0;
};

class PLASMA_RENDERERCORE_DLL plAnimPoseGenerator final
{
public:
  plAnimPoseGenerator();
  ~plAnimPoseGenerator();

  void Reset(const plSkeletonResource* pSkeleton);

  plAnimPoseGeneratorCommandSampleTrack& AllocCommandSampleTrack(plUInt32 uiDeterministicID);
  plAnimPoseGeneratorCommandRestPose& AllocCommandRestPose();
  plAnimPoseGeneratorCommandCombinePoses& AllocCommandCombinePoses();
  plAnimPoseGeneratorCommandLocalToModelPose& AllocCommandLocalToModelPose();
  plAnimPoseGeneratorCommandModelPoseToOutput& AllocCommandModelPoseToOutput();
  plAnimPoseGeneratorCommandSampleEventTrack& AllocCommandSampleEventTrack();

  const plAnimPoseGeneratorCommand& GetCommand(plAnimPoseGeneratorCommandID id) const;
  plAnimPoseGeneratorCommand& GetCommand(plAnimPoseGeneratorCommandID id);

  plArrayPtr<plMat4> GeneratePose(const plGameObject* pSendAnimationEventsTo);

private:
  void Validate() const;

  void Execute(plAnimPoseGeneratorCommand& cmd, const plGameObject* pSendAnimationEventsTo);
  void ExecuteCmd(plAnimPoseGeneratorCommandSampleTrack& cmd, const plGameObject* pSendAnimationEventsTo);
  void ExecuteCmd(plAnimPoseGeneratorCommandRestPose& cmd);
  void ExecuteCmd(plAnimPoseGeneratorCommandCombinePoses& cmd);
  void ExecuteCmd(plAnimPoseGeneratorCommandLocalToModelPose& cmd, const plGameObject* pSendAnimationEventsTo);
  void ExecuteCmd(plAnimPoseGeneratorCommandModelPoseToOutput& cmd);
  void ExecuteCmd(plAnimPoseGeneratorCommandSampleEventTrack& cmd, const plGameObject* pSendAnimationEventsTo);
  void SampleEventTrack(const plAnimationClipResource* pResource, plAnimPoseEventTrackSampleMode mode, const plGameObject* pSendAnimationEventsTo, float fPrevPos, float fCurPos);

  plArrayPtr<ozz::math::SoaTransform> AcquireLocalPoseTransforms(plAnimPoseGeneratorLocalPoseID id);
  plArrayPtr<plMat4> AcquireModelPoseTransforms(plAnimPoseGeneratorModelPoseID id);

  const plSkeletonResource* m_pSkeleton = nullptr;

  plAnimPoseGeneratorLocalPoseID m_LocalPoseCounter = 0;
  plAnimPoseGeneratorModelPoseID m_ModelPoseCounter = 0;

  plArrayPtr<plMat4> m_OutputPose;

  plHybridArray<plArrayPtr<ozz::math::SoaTransform>, 8> m_UsedLocalTransforms;
  plHybridArray<plDynamicArray<plMat4, plAlignedAllocatorWrapper>, 2> m_UsedModelTransforms;

  plHybridArray<plAnimPoseGeneratorCommandSampleTrack, 4> m_CommandsSampleTrack;
  plHybridArray<plAnimPoseGeneratorCommandRestPose, 1> m_CommandsRestPose;
  plHybridArray<plAnimPoseGeneratorCommandCombinePoses, 1> m_CommandsCombinePoses;
  plHybridArray<plAnimPoseGeneratorCommandLocalToModelPose, 1> m_CommandsLocalToModelPose;
  plHybridArray<plAnimPoseGeneratorCommandModelPoseToOutput, 1> m_CommandsModelPoseToOutput;
  plHybridArray<plAnimPoseGeneratorCommandSampleEventTrack, 2> m_CommandsSampleEventTrack;

  plArrayMap<plUInt32, ozz::animation::SamplingJob::Context*> m_SamplingCaches;
};

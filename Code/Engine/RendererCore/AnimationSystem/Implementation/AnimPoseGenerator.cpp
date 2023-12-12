#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/span.h>

void plAnimPoseGenerator::Reset(const plSkeletonResource* pSkeleton)
{
  m_pSkeleton = pSkeleton;
  m_LocalPoseCounter = 0;
  m_ModelPoseCounter = 0;

  m_CommandsSampleTrack.Clear();
  m_CommandsRestPose.Clear();
  m_CommandsCombinePoses.Clear();
  m_CommandsLocalToModelPose.Clear();
  m_CommandsModelPoseToOutput.Clear();

  m_UsedLocalTransforms.Clear();

  m_OutputPose.Clear();

  // don't clear these arrays, they are reused
  // m_UsedModelTransforms.Clear();
  // m_SamplingCaches.Clear();
}

static PLASMA_ALWAYS_INLINE plAnimPoseGeneratorCommandID CreateCommandID(plAnimPoseGeneratorCommandType type, plUInt32 uiIndex)
{
  return (static_cast<plUInt32>(type) << 24u) | uiIndex;
}

static PLASMA_ALWAYS_INLINE plUInt32 GetCommandIndex(plAnimPoseGeneratorCommandID id)
{
  return static_cast<plUInt32>(id) & 0x00FFFFFFu;
}

static PLASMA_ALWAYS_INLINE plAnimPoseGeneratorCommandType GetCommandType(plAnimPoseGeneratorCommandID id)
{
  return static_cast<plAnimPoseGeneratorCommandType>(static_cast<plUInt32>(id) >> 24u);
}

plAnimPoseGeneratorCommandSampleTrack& plAnimPoseGenerator::AllocCommandSampleTrack(plUInt32 uiDeterministicID)
{
  auto& cmd = m_CommandsSampleTrack.ExpandAndGetRef();
  cmd.m_Type = plAnimPoseGeneratorCommandType::SampleTrack;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsSampleTrack.GetCount() - 1);
  cmd.m_LocalPoseOutput = m_LocalPoseCounter++;
  cmd.m_uiUniqueID = uiDeterministicID;

  return cmd;
}

plAnimPoseGeneratorCommandRestPose& plAnimPoseGenerator::AllocCommandRestPose()
{
  auto& cmd = m_CommandsRestPose.ExpandAndGetRef();
  cmd.m_Type = plAnimPoseGeneratorCommandType::RestPose;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsRestPose.GetCount() - 1);
  cmd.m_LocalPoseOutput = m_LocalPoseCounter++;

  return cmd;
}

plAnimPoseGeneratorCommandCombinePoses& plAnimPoseGenerator::AllocCommandCombinePoses()
{
  auto& cmd = m_CommandsCombinePoses.ExpandAndGetRef();
  cmd.m_Type = plAnimPoseGeneratorCommandType::CombinePoses;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsCombinePoses.GetCount() - 1);
  cmd.m_LocalPoseOutput = m_LocalPoseCounter++;

  return cmd;
}

plAnimPoseGeneratorCommandLocalToModelPose& plAnimPoseGenerator::AllocCommandLocalToModelPose()
{
  auto& cmd = m_CommandsLocalToModelPose.ExpandAndGetRef();
  cmd.m_Type = plAnimPoseGeneratorCommandType::LocalToModelPose;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsLocalToModelPose.GetCount() - 1);
  cmd.m_ModelPoseOutput = m_ModelPoseCounter++;

  return cmd;
}

plAnimPoseGeneratorCommandModelPoseToOutput& plAnimPoseGenerator::AllocCommandModelPoseToOutput()
{
  auto& cmd = m_CommandsModelPoseToOutput.ExpandAndGetRef();
  cmd.m_Type = plAnimPoseGeneratorCommandType::ModelPoseToOutput;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsModelPoseToOutput.GetCount() - 1);

  return cmd;
}

plAnimPoseGeneratorCommandSampleEventTrack& plAnimPoseGenerator::AllocCommandSampleEventTrack()
{
  auto& cmd = m_CommandsSampleEventTrack.ExpandAndGetRef();
  cmd.m_Type = plAnimPoseGeneratorCommandType::SampleEventTrack;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsSampleEventTrack.GetCount() - 1);

  return cmd;
}

plAnimPoseGenerator::plAnimPoseGenerator() = default;

plAnimPoseGenerator::~plAnimPoseGenerator()
{
  for (plUInt32 i = 0; i < m_SamplingCaches.GetCount(); ++i)
  {
    PLASMA_DEFAULT_DELETE(m_SamplingCaches.GetValue(i));
  }
  m_SamplingCaches.Clear();
}

void plAnimPoseGenerator::Validate() const
{
  PLASMA_ASSERT_DEV(m_CommandsModelPoseToOutput.GetCount() <= 1, "Only one output node may exist");

  for (auto& cmd : m_CommandsSampleTrack)
  {
    PLASMA_ASSERT_DEV(cmd.m_hAnimationClip.IsValid(), "Invalid animation clips are not allowed.");
    // PLASMA_ASSERT_DEV(cmd.m_Inputs.IsEmpty(), "Track samplers can't have inputs.");
    PLASMA_ASSERT_DEV(cmd.m_LocalPoseOutput != plInvalidIndex, "Output pose not allocated.");
  }

  for (auto& cmd : m_CommandsCombinePoses)
  {
    // PLASMA_ASSERT_DEV(cmd.m_Inputs.GetCount() >= 1, "Must combine at least one pose.");
    PLASMA_ASSERT_DEV(cmd.m_LocalPoseOutput != plInvalidIndex, "Output pose not allocated.");
    PLASMA_ASSERT_DEV(cmd.m_Inputs.GetCount() == cmd.m_InputWeights.GetCount(), "Number of inputs and weights must match.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      PLASMA_ASSERT_DEV(type == plAnimPoseGeneratorCommandType::SampleTrack || type == plAnimPoseGeneratorCommandType::CombinePoses || type == plAnimPoseGeneratorCommandType::RestPose, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsLocalToModelPose)
  {
    PLASMA_ASSERT_DEV(cmd.m_Inputs.GetCount() == 1, "Exactly one input must be provided.");
    PLASMA_ASSERT_DEV(cmd.m_ModelPoseOutput != plInvalidIndex, "Output pose not allocated.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      PLASMA_ASSERT_DEV(type == plAnimPoseGeneratorCommandType::SampleTrack || type == plAnimPoseGeneratorCommandType::CombinePoses || type == plAnimPoseGeneratorCommandType::RestPose, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsModelPoseToOutput)
  {
    PLASMA_ASSERT_DEV(cmd.m_Inputs.GetCount() == 1, "Exactly one input must be provided.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      PLASMA_ASSERT_DEV(type == plAnimPoseGeneratorCommandType::LocalToModelPose, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsSampleEventTrack)
  {
    PLASMA_ASSERT_DEV(cmd.m_hAnimationClip.IsValid(), "Invalid animation clips are not allowed.");
  }
}

const plAnimPoseGeneratorCommand& plAnimPoseGenerator::GetCommand(plAnimPoseGeneratorCommandID id) const
{
  return const_cast<plAnimPoseGenerator*>(this)->GetCommand(id);
}

plAnimPoseGeneratorCommand& plAnimPoseGenerator::GetCommand(plAnimPoseGeneratorCommandID id)
{
  PLASMA_ASSERT_DEV(id != plInvalidIndex, "Invalid command ID");

  switch (GetCommandType(id))
  {
    case plAnimPoseGeneratorCommandType::SampleTrack:
      return m_CommandsSampleTrack[GetCommandIndex(id)];

    case plAnimPoseGeneratorCommandType::RestPose:
      return m_CommandsRestPose[GetCommandIndex(id)];

    case plAnimPoseGeneratorCommandType::CombinePoses:
      return m_CommandsCombinePoses[GetCommandIndex(id)];

    case plAnimPoseGeneratorCommandType::LocalToModelPose:
      return m_CommandsLocalToModelPose[GetCommandIndex(id)];

    case plAnimPoseGeneratorCommandType::ModelPoseToOutput:
      return m_CommandsModelPoseToOutput[GetCommandIndex(id)];

    case plAnimPoseGeneratorCommandType::SampleEventTrack:
      return m_CommandsSampleEventTrack[GetCommandIndex(id)];

      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  PLASMA_REPORT_FAILURE("Invalid command ID");
  return m_CommandsSampleTrack[0];
}

plArrayPtr<plMat4> plAnimPoseGenerator::GeneratePose(const plGameObject* pSendAnimationEventsTo /*= nullptr*/)
{
  Validate();

  for (auto& cmd : m_CommandsModelPoseToOutput)
  {
    Execute(cmd, pSendAnimationEventsTo);
  }

  auto pPose = m_OutputPose;

  // TODO: clear temp data

  return pPose;
}

void plAnimPoseGenerator::Execute(plAnimPoseGeneratorCommand& cmd, const plGameObject* pSendAnimationEventsTo)
{
  if (cmd.m_bExecuted)
    return;

  // TODO: validate for circular dependencies
  cmd.m_bExecuted = true;

  for (auto id : cmd.m_Inputs)
  {
    Execute(GetCommand(id), pSendAnimationEventsTo);
  }

  // TODO: build a task graph and execute multi-threaded

  switch (cmd.GetType())
  {
    case plAnimPoseGeneratorCommandType::SampleTrack:
      ExecuteCmd(static_cast<plAnimPoseGeneratorCommandSampleTrack&>(cmd), pSendAnimationEventsTo);
      break;

    case plAnimPoseGeneratorCommandType::RestPose:
      ExecuteCmd(static_cast<plAnimPoseGeneratorCommandRestPose&>(cmd));
      break;

    case plAnimPoseGeneratorCommandType::CombinePoses:
      ExecuteCmd(static_cast<plAnimPoseGeneratorCommandCombinePoses&>(cmd));
      break;

    case plAnimPoseGeneratorCommandType::LocalToModelPose:
      ExecuteCmd(static_cast<plAnimPoseGeneratorCommandLocalToModelPose&>(cmd), pSendAnimationEventsTo);
      break;

    case plAnimPoseGeneratorCommandType::ModelPoseToOutput:
      ExecuteCmd(static_cast<plAnimPoseGeneratorCommandModelPoseToOutput&>(cmd));
      break;

    case plAnimPoseGeneratorCommandType::SampleEventTrack:
      ExecuteCmd(static_cast<plAnimPoseGeneratorCommandSampleEventTrack&>(cmd), pSendAnimationEventsTo);
      break;

      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void plAnimPoseGenerator::ExecuteCmd(plAnimPoseGeneratorCommandSampleTrack& cmd, const plGameObject* pSendAnimationEventsTo)
{
  plResourceLock<plAnimationClipResource> pResource(cmd.m_hAnimationClip, plResourceAcquireMode::BlockTillLoaded);

  const ozz::animation::Animation& ozzAnim = pResource->GetDescriptor().GetMappedOzzAnimation(*m_pSkeleton);

  cmd.m_bAdditive = pResource->GetDescriptor().m_bAdditive;

  auto transforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  auto& pSampler = m_SamplingCaches[cmd.m_uiUniqueID];

  if (pSampler == nullptr)
  {
    pSampler = PLASMA_DEFAULT_NEW(ozz::animation::SamplingJob::Context);
  }

  if (pSampler->max_tracks() != ozzAnim.num_tracks())
  {
    pSampler->Resize(ozzAnim.num_tracks());
  }

  ozz::animation::SamplingJob job;
  job.animation = &ozzAnim;
  job.context = pSampler;
  job.ratio = cmd.m_fNormalizedSamplePos;
  job.output = ozz::span<ozz::math::SoaTransform>(transforms.GetPtr(), transforms.GetCount());

  if (!job.Validate())
    return;

  PLASMA_ASSERT_DEBUG(job.Validate(), "");
  job.Run();

  SampleEventTrack(pResource.GetPointer(), cmd.m_EventSampling, pSendAnimationEventsTo, cmd.m_fPreviousNormalizedSamplePos, cmd.m_fNormalizedSamplePos);
}

void plAnimPoseGenerator::ExecuteCmd(plAnimPoseGeneratorCommandRestPose& cmd)
{
  auto transforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  const auto restPose = m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_rest_poses();

  transforms.CopyFrom(plArrayPtr<const ozz::math::SoaTransform>(restPose.begin(), (plUInt32)restPose.size()));
}

void plAnimPoseGenerator::ExecuteCmd(plAnimPoseGeneratorCommandCombinePoses& cmd)
{
  auto transforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  plHybridArray<ozz::animation::BlendingJob::Layer, 8> bl;
  plHybridArray<ozz::animation::BlendingJob::Layer, 8> blAdd;

  for (plUInt32 i = 0; i < cmd.m_Inputs.GetCount(); ++i)
  {
    const auto& cmdIn = GetCommand(cmd.m_Inputs[i]);

    if (cmdIn.GetType() == plAnimPoseGeneratorCommandType::SampleEventTrack)
      continue;

    ozz::animation::BlendingJob::Layer* layer = nullptr;

    switch (cmdIn.GetType())
    {
      case plAnimPoseGeneratorCommandType::SampleTrack:
      {
        if (static_cast<const plAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_bAdditive)
        {
          layer = &blAdd.ExpandAndGetRef();
        }
        else
        {
          layer = &bl.ExpandAndGetRef();
        }

        auto transform = AcquireLocalPoseTransforms(static_cast<const plAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_LocalPoseOutput);
        layer->transform = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
      }
      break;

      case plAnimPoseGeneratorCommandType::RestPose:
      {
        layer = &bl.ExpandAndGetRef();

        auto transform = AcquireLocalPoseTransforms(static_cast<const plAnimPoseGeneratorCommandRestPose&>(cmdIn).m_LocalPoseOutput);
        layer->transform = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
      }
      break;

      case plAnimPoseGeneratorCommandType::CombinePoses:
      {
        layer = &bl.ExpandAndGetRef();

        auto transform = AcquireLocalPoseTransforms(static_cast<const plAnimPoseGeneratorCommandCombinePoses&>(cmdIn).m_LocalPoseOutput);
        layer->transform = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
      }
      break;

        PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    layer->weight = cmd.m_InputWeights[i];

    if (cmd.m_InputBoneWeights.GetCount() > i && !cmd.m_InputBoneWeights[i].IsEmpty())
    {
      layer->joint_weights = ozz::span(cmd.m_InputBoneWeights[i].GetPtr(), cmd.m_InputBoneWeights[i].GetEndPtr());
    }
  }

  ozz::animation::BlendingJob job;
  job.threshold = 1.0f;
  job.layers = ozz::span<const ozz::animation::BlendingJob::Layer>(begin(bl), end(bl));
  job.additive_layers = ozz::span<const ozz::animation::BlendingJob::Layer>(begin(blAdd), end(blAdd));
  job.rest_pose = m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_rest_poses();
  job.output = ozz::span<ozz::math::SoaTransform>(transforms.GetPtr(), transforms.GetCount());
  PLASMA_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

void plAnimPoseGenerator::ExecuteCmd(plAnimPoseGeneratorCommandLocalToModelPose& cmd, const plGameObject* pSendAnimationEventsTo)
{
  ozz::animation::LocalToModelJob job;

  const auto& cmdIn = GetCommand(cmd.m_Inputs[0]);

  switch (cmdIn.GetType())
  {
    case plAnimPoseGeneratorCommandType::SampleTrack:
    {
      auto transform = AcquireLocalPoseTransforms(static_cast<const plAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_LocalPoseOutput);
      job.input = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
    }
    break;

    case plAnimPoseGeneratorCommandType::RestPose:
    {
      auto transform = AcquireLocalPoseTransforms(static_cast<const plAnimPoseGeneratorCommandRestPose&>(cmdIn).m_LocalPoseOutput);
      job.input = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
    }
    break;

    case plAnimPoseGeneratorCommandType::CombinePoses:
    {
      auto transform = AcquireLocalPoseTransforms(static_cast<const plAnimPoseGeneratorCommandCombinePoses&>(cmdIn).m_LocalPoseOutput);
      job.input = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
    }
    break;

      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (cmd.m_pSendLocalPoseMsgTo || pSendAnimationEventsTo)
  {
    plMsgAnimationPosePreparing msg;
    msg.m_pSkeleton = &m_pSkeleton->GetDescriptor().m_Skeleton;
    msg.m_LocalTransforms = plMakeArrayPtr(const_cast<ozz::math::SoaTransform*>(job.input.data()), (plUInt32)job.input.size());

    if (pSendAnimationEventsTo)
      pSendAnimationEventsTo->SendMessageRecursive(msg);
    else
      cmd.m_pSendLocalPoseMsgTo->SendMessageRecursive(msg);
  }

  auto transforms = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);

  job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(transforms.GetPtr()), transforms.GetCount());
  job.skeleton = &m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
  PLASMA_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

void plAnimPoseGenerator::ExecuteCmd(plAnimPoseGeneratorCommandModelPoseToOutput& cmd)
{
  const auto& cmdIn = GetCommand(cmd.m_Inputs[0]);

  switch (cmdIn.GetType())
  {
    case plAnimPoseGeneratorCommandType::LocalToModelPose:
      m_OutputPose = AcquireModelPoseTransforms(static_cast<const plAnimPoseGeneratorCommandLocalToModelPose&>(cmdIn).m_ModelPoseOutput);
      break;

      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void plAnimPoseGenerator::ExecuteCmd(plAnimPoseGeneratorCommandSampleEventTrack& cmd, const plGameObject* pSendAnimationEventsTo)
{
  plResourceLock<plAnimationClipResource> pResource(cmd.m_hAnimationClip, plResourceAcquireMode::BlockTillLoaded);

  SampleEventTrack(pResource.GetPointer(), cmd.m_EventSampling, pSendAnimationEventsTo, cmd.m_fPreviousNormalizedSamplePos, cmd.m_fNormalizedSamplePos);
}

void plAnimPoseGenerator::SampleEventTrack(const plAnimationClipResource* pResource, plAnimPoseEventTrackSampleMode mode, const plGameObject* pSendAnimationEventsTo, float fPrevPos, float fCurPos)
{
  const auto& et = pResource->GetDescriptor().m_EventTrack;

  if (mode == plAnimPoseEventTrackSampleMode::None || et.IsEmpty())
    return;

  const plTime duration = pResource->GetDescriptor().GetDuration();

  const plTime tPrev = fPrevPos * duration;
  const plTime tNow = fCurPos * duration;
  const plTime tStart = plTime::Zero();
  const plTime tEnd = duration + plTime::Seconds(1.0); // sampling position is EXCLUSIVE

  plHybridArray<plHashedString, 16> events;

  switch (mode)
  {
    case plAnimPoseEventTrackSampleMode::OnlyBetween:
      et.Sample(tPrev, tNow, events);
      break;

    case plAnimPoseEventTrackSampleMode::LoopAtEnd:
      et.Sample(tPrev, tEnd, events);
      et.Sample(tStart, tNow, events);
      break;

    case plAnimPoseEventTrackSampleMode::LoopAtStart:
      et.Sample(tPrev, tStart, events);
      et.Sample(tStart, tNow, events);
      break;

    case plAnimPoseEventTrackSampleMode::BounceAtEnd:
      et.Sample(tPrev, tEnd, events);
      et.Sample(tEnd, tNow, events);
      break;

    case plAnimPoseEventTrackSampleMode::BounceAtStart:
      et.Sample(tPrev, tStart, events);
      et.Sample(tStart, tNow, events);
      break;

      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  plMsgGenericEvent msg;

  for (const auto& hs : events)
  {
    msg.m_sMessage = hs;

    pSendAnimationEventsTo->SendEventMessage(msg, nullptr);
  }
}

plArrayPtr<ozz::math::SoaTransform> plAnimPoseGenerator::AcquireLocalPoseTransforms(plAnimPoseGeneratorLocalPoseID id)
{
  m_UsedLocalTransforms.EnsureCount(id + 1);

  if (m_UsedLocalTransforms[id].IsEmpty())
  {
    using T = ozz::math::SoaTransform;
    const plUInt32 num = m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints();
    m_UsedLocalTransforms[id] = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), T, num);
  }

  return m_UsedLocalTransforms[id];
}

plArrayPtr<plMat4> plAnimPoseGenerator::AcquireModelPoseTransforms(plAnimPoseGeneratorModelPoseID id)
{
  m_UsedModelTransforms.EnsureCount(id + 1);

  m_UsedModelTransforms[id].SetCountUninitialized(m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_joints());

  return m_UsedModelTransforms[id];
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_AnimPoseGenerator);

#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SampleAnimClipSequenceAnimNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSampleAnimClipSequenceAnimNode, 1, plRTTIDefaultAllocator<plSampleAnimClipSequenceAnimNode>)
  {
    PLASMA_BEGIN_PROPERTIES
    {
      PLASMA_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, {})),
      PLASMA_MEMBER_PROPERTY("Loop", m_bLoop),
      //PLASMA_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),
      PLASMA_ACCESSOR_PROPERTY("StartClip", GetStartClip, SetStartClip)->AddAttributes(new plDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      PLASMA_ARRAY_ACCESSOR_PROPERTY("MiddleClips", Clips_GetCount, Clips_GetValue, Clips_SetValue, Clips_Insert, Clips_Remove)->AddAttributes(new plDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      PLASMA_ACCESSOR_PROPERTY("EndClip", GetEndClip, SetEndClip)->AddAttributes(new plDynamicStringEnumAttribute("AnimationClipMappingEnum")),

      PLASMA_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new plHiddenAttribute()),
      PLASMA_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new plHiddenAttribute()),
      PLASMA_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new plHiddenAttribute()),
      PLASMA_MEMBER_PROPERTY("ClipIndex", m_ClipIndexPin)->AddAttributes(new plHiddenAttribute()),

      PLASMA_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new plHiddenAttribute()),
      PLASMA_MEMBER_PROPERTY("OutOnMiddleStarted", m_OutOnMiddleStarted)->AddAttributes(new plHiddenAttribute()),
      PLASMA_MEMBER_PROPERTY("OutOnEndStarted", m_OutOnEndStarted)->AddAttributes(new plHiddenAttribute()),
      PLASMA_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new plHiddenAttribute()),
    }
    PLASMA_END_PROPERTIES;
    PLASMA_BEGIN_ATTRIBUTES
    {
      new plCategoryAttribute("Pose Generation"),
      new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Blue)),
      new plTitleAttribute("Sample Sequence: '{StartClip}' '{Clip}' '{EndClip}'"),
    }
    PLASMA_END_ATTRIBUTES;
  }
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSampleAnimClipSequenceAnimNode::plSampleAnimClipSequenceAnimNode() = default;
plSampleAnimClipSequenceAnimNode::~plSampleAnimClipSequenceAnimNode() = default;

plResult plSampleAnimClipSequenceAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sStartClip;
  PLASMA_SUCCEED_OR_RETURN(stream.WriteArray(m_Clips));
  stream << m_sEndClip;
  stream << m_bApplyRootMotion;
  stream << m_bLoop;
  stream << m_fPlaybackSpeed;

  PLASMA_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_ClipIndexPin.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnMiddleStarted.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnEndStarted.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plSampleAnimClipSequenceAnimNode::DeserializeNode(plStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sStartClip;
  PLASMA_SUCCEED_OR_RETURN(stream.ReadArray(m_Clips));
  stream >> m_sEndClip;
  stream >> m_bApplyRootMotion;
  stream >> m_bLoop;
  stream >> m_fPlaybackSpeed;

  PLASMA_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_ClipIndexPin.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnMiddleStarted.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnEndStarted.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return PLASMA_SUCCESS;
}

plUInt32 plSampleAnimClipSequenceAnimNode::Clips_GetCount() const
{
  return m_Clips.GetCount();
}

const char* plSampleAnimClipSequenceAnimNode::Clips_GetValue(plUInt32 uiIndex) const
{
  return m_Clips[uiIndex];
}

void plSampleAnimClipSequenceAnimNode::Clips_SetValue(plUInt32 uiIndex, const char* szValue)
{
  m_Clips[uiIndex].Assign(szValue);
}

void plSampleAnimClipSequenceAnimNode::Clips_Insert(plUInt32 uiIndex, const char* szValue)
{
  plHashedString s;
  s.Assign(szValue);
  m_Clips.Insert(s, uiIndex);
}

void plSampleAnimClipSequenceAnimNode::Clips_Remove(plUInt32 uiIndex)
{
  m_Clips.RemoveAtAndCopy(uiIndex);
}

void plSampleAnimClipSequenceAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected())
    return;

  InstanceState* pState = ref_graph.GetAnimNodeInstanceData<InstanceState>(*this);

  if ((!m_InStart.IsConnected() && pState->m_uiState == 0) || m_InStart.IsTriggered(ref_graph))
  {
    pState->m_PlaybackTime = plTime::MakeZero();
    pState->m_uiState = 1;
  }

  if (pState->m_uiState == 0)
    return;

  const bool bLoop = m_InLoop.GetBool(ref_graph, m_bLoop);

  // currently we only support playing clips forwards
  const float fPlaySpeed = plMath::Max(0.0f, static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed)));

  plTime tPrevSamplePos = pState->m_PlaybackTime;
  pState->m_PlaybackTime += tDiff * fPlaySpeed;

  plAnimationClipResourceHandle hCurClip;
  plTime tCurDuration;

  while (pState->m_uiState != 0)
  {
    if (pState->m_uiState == 1)
    {
      const auto& startClip = ref_controller.GetAnimationClipInfo(m_sStartClip);

      if (!startClip.m_hClip.IsValid())
      {
        if (!m_Clips.IsEmpty())
        {
          pState->m_uiMiddleClipIdx = m_ClipIndexPin.GetNumber(ref_graph, 0xFF);
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }

        pState->m_uiState = 2;
        m_OutOnMiddleStarted.SetTriggered(ref_graph);
        continue;
      }

      plResourceLock<plAnimationClipResource> pAnimClip(startClip.m_hClip, plResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != plResourceAcquireResult::Final)
      {
        if (!m_Clips.IsEmpty())
        {
          pState->m_uiMiddleClipIdx = m_ClipIndexPin.GetNumber(ref_graph, 0xFF);
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }

        pState->m_uiState = 2;
        m_OutOnMiddleStarted.SetTriggered(ref_graph);
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      PLASMA_ASSERT_DEBUG(tCurDuration >= plTime::MakeFromMilliseconds(5), "Too short clip");

      if (pState->m_PlaybackTime >= tCurDuration)
      {
        // TODO: sample anim events of previous clip
        m_OutOnMiddleStarted.SetTriggered(ref_graph);
        tPrevSamplePos = plTime::MakeZero();
        pState->m_PlaybackTime -= tCurDuration;
        pState->m_uiState = 2;

        if (!m_Clips.IsEmpty())
        {
          pState->m_uiMiddleClipIdx = m_ClipIndexPin.GetNumber(ref_graph, 0xFF);
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }
        continue;
      }

      hCurClip = startClip.m_hClip;
      break;
    }

    if (pState->m_uiState == 2)
    {
      const auto& clipInfo = ref_controller.GetAnimationClipInfo(m_Clips[pState->m_uiMiddleClipIdx]);

      if (m_Clips.IsEmpty() || !clipInfo.m_hClip.IsValid())
      {
        pState->m_uiState = 3;
        m_OutOnEndStarted.SetTriggered(ref_graph);
        continue;
      }

      plResourceLock<plAnimationClipResource> pAnimClip(clipInfo.m_hClip, plResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != plResourceAcquireResult::Final)
      {
        pState->m_uiState = 3;
        m_OutOnEndStarted.SetTriggered(ref_graph);
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      PLASMA_ASSERT_DEBUG(tCurDuration >= plTime::MakeFromMilliseconds(5), "Too short clip");

      if (pState->m_PlaybackTime >= tCurDuration)
      {
        // TODO: sample anim events of previous clip
        tPrevSamplePos = plTime::MakeZero();
        pState->m_PlaybackTime -= tCurDuration;

        if (bLoop)
        {
          m_OutOnMiddleStarted.SetTriggered(ref_graph);
          pState->m_uiState = 2;

          pState->m_uiMiddleClipIdx = m_ClipIndexPin.GetNumber(ref_graph, 0xFF);
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }
        else
        {
          m_OutOnEndStarted.SetTriggered(ref_graph);
          pState->m_uiState = 3;
        }
        continue;
      }

      hCurClip = clipInfo.m_hClip;
      break;
    }

    if (pState->m_uiState == 3)
    {
      const auto& endClip = ref_controller.GetAnimationClipInfo(m_sEndClip);

      if (!endClip.m_hClip.IsValid())
      {
        pState->m_uiState = 0;
        m_OutOnFinished.SetTriggered(ref_graph);
        continue;
      }

      plResourceLock<plAnimationClipResource> pAnimClip(endClip.m_hClip, plResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != plResourceAcquireResult::Final)
      {
        pState->m_uiState = 0;
        m_OutOnFinished.SetTriggered(ref_graph);
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      PLASMA_ASSERT_DEBUG(tCurDuration >= plTime::MakeFromMilliseconds(5), "Too short clip");

      if (pState->m_PlaybackTime >= tCurDuration)
      {
        // TODO: sample anim events of previous clip
        m_OutOnFinished.SetTriggered(ref_graph);
        pState->m_uiState = 0;
        continue;
      }

      hCurClip = endClip.m_hClip;
      break;
    }
  }

  if (!hCurClip.IsValid())
    return;

  const float fInvDuration = 1.0f / tCurDuration.AsFloatInSeconds();

  const void* pThis = this;
  auto& cmd = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(plHashingUtils::xxHash32(&pThis, sizeof(pThis)));
  cmd.m_EventSampling = plAnimPoseEventTrackSampleMode::OnlyBetween;

  cmd.m_hAnimationClip = hCurClip;
  cmd.m_fPreviousNormalizedSamplePos = plMath::Clamp(tPrevSamplePos.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);
  cmd.m_fNormalizedSamplePos = plMath::Clamp(pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);

  {
    plAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

    pLocalTransforms->m_pWeights = nullptr;
    pLocalTransforms->m_bUseRootMotion = false; // m_bApplyRootMotion;
    pLocalTransforms->m_fOverallWeight = 1.0f;
    // pLocalTransforms->m_vRootMotion = pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * fPlaySpeed;
    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    m_OutPose.SetPose(ref_graph, pLocalTransforms);
  }
}

void plSampleAnimClipSequenceAnimNode::SetStartClip(const char* szClip)
{
  m_sStartClip.Assign(szClip);
}

const char* plSampleAnimClipSequenceAnimNode::GetStartClip() const
{
  return m_sStartClip;
}

void plSampleAnimClipSequenceAnimNode::SetEndClip(const char* szClip)
{
  m_sEndClip.Assign(szClip);
}

const char* plSampleAnimClipSequenceAnimNode::GetEndClip() const
{
  return m_sEndClip;
}

bool plSampleAnimClipSequenceAnimNode::GetInstanceDataDesc(plInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceState>();
  return true;
}

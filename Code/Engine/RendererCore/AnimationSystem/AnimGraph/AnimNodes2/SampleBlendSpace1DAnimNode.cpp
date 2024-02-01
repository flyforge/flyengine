#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SampleBlendSpace1DAnimNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plAnimationClip1D, plNoBase, 1, plRTTIDefaultAllocator<plAnimationClip1D>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Clip", GetAnimationFile, SetAnimationFile)->AddAttributes(new plDynamicStringEnumAttribute("AnimationClipMappingEnum")),
    PL_MEMBER_PROPERTY("Position", m_fPosition),
    PL_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new plDefaultValueAttribute(1.0f)),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSampleBlendSpace1DAnimNode, 1, plRTTIDefaultAllocator<plSampleBlendSpace1DAnimNode>)
  {
    PL_BEGIN_PROPERTIES
    {
      PL_MEMBER_PROPERTY("Loop", m_bLoop)->AddAttributes(new plDefaultValueAttribute(true)),
      PL_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, {})),
      PL_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),
      PL_ARRAY_MEMBER_PROPERTY("Clips", m_Clips),

      PL_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new plHiddenAttribute()),
      PL_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new plHiddenAttribute()),
      PL_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new plHiddenAttribute()),
      PL_MEMBER_PROPERTY("InLerp", m_InLerp)->AddAttributes(new plHiddenAttribute()),

      PL_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new plHiddenAttribute()),
      PL_MEMBER_PROPERTY("OutOnStarted", m_OutOnStarted)->AddAttributes(new plHiddenAttribute()),
      PL_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new plHiddenAttribute()),
    }
    PL_END_PROPERTIES;
    PL_BEGIN_ATTRIBUTES
    {
      new plCategoryAttribute("Pose Generation"),
      new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Blue)),
      new plTitleAttribute("BlendSpace 1D: '{Clips[0]}' '{Clips[1]}' '{Clips[2]}'"),
    }
    PL_END_ATTRIBUTES;
  }
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plAnimationClip1D::SetAnimationFile(const char* szFile)
{
  m_sClip.Assign(szFile);
}

const char* plAnimationClip1D::GetAnimationFile() const
{
  return m_sClip;
}

plSampleBlendSpace1DAnimNode::plSampleBlendSpace1DAnimNode() = default;
plSampleBlendSpace1DAnimNode::~plSampleBlendSpace1DAnimNode() = default;

plResult plSampleBlendSpace1DAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_Clips.GetCount();
  for (plUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream << m_Clips[i].m_sClip;
    stream << m_Clips[i].m_fPosition;
    stream << m_Clips[i].m_fSpeed;
  }

  stream << m_bLoop;
  stream << m_bApplyRootMotion;
  stream << m_fPlaybackSpeed;

  PL_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_InLerp.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutOnStarted.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return PL_SUCCESS;
}

plResult plSampleBlendSpace1DAnimNode::DeserializeNode(plStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  plUInt32 num = 0;
  stream >> num;
  m_Clips.SetCount(num);
  for (plUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream >> m_Clips[i].m_sClip;
    stream >> m_Clips[i].m_fPosition;
    stream >> m_Clips[i].m_fSpeed;
  }

  stream >> m_bLoop;
  stream >> m_bApplyRootMotion;
  stream >> m_fPlaybackSpeed;

  PL_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_InLerp.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutOnStarted.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return PL_SUCCESS;
}

void plSampleBlendSpace1DAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected() || !m_InLerp.IsConnected() || m_Clips.IsEmpty())
    return;

  InstanceState* pState = ref_graph.GetAnimNodeInstanceData<InstanceState>(*this);

  if ((!m_InStart.IsConnected() && !pState->m_bPlaying) || m_InStart.IsTriggered(ref_graph))
  {
    pState->m_PlaybackTime = plTime::MakeZero();
    pState->m_bPlaying = true;

    m_OutOnStarted.SetTriggered(ref_graph);
  }

  if (!pState->m_bPlaying)
    return;

  const bool bLoop = m_InLoop.GetBool(ref_graph, m_bLoop);

  plUInt32 uiClip1 = 0;
  plUInt32 uiClip2 = 0;

  const float fLerpPos = (float)m_InLerp.GetNumber(ref_graph);

  if (m_Clips.GetCount() > 1)
  {
    float fDist1 = 1000000.0f;
    float fDist2 = 1000000.0f;

    for (plUInt32 i = 0; i < m_Clips.GetCount(); ++i)
    {
      const float dist = plMath::Abs(m_Clips[i].m_fPosition - fLerpPos);

      if (dist < fDist1)
      {
        fDist2 = fDist1;
        uiClip2 = uiClip1;

        fDist1 = dist;
        uiClip1 = i;
      }
      else if (dist < fDist2)
      {
        fDist2 = dist;
        uiClip2 = i;
      }
    }

    if (plMath::IsZero(fDist1, plMath::SmallEpsilon<float>()))
    {
      uiClip2 = uiClip1;
    }
  }

  const auto& clip1 = ref_controller.GetAnimationClipInfo(m_Clips[uiClip1].m_sClip);
  const auto& clip2 = ref_controller.GetAnimationClipInfo(m_Clips[uiClip2].m_sClip);

  if (!clip1.m_hClip.IsValid() || !clip2.m_hClip.IsValid())
    return;

  float fLerpFactor = 0.0f;

  if (uiClip1 != uiClip2)
  {
    const float len = m_Clips[uiClip2].m_fPosition - m_Clips[uiClip1].m_fPosition;
    fLerpFactor = (fLerpPos - m_Clips[uiClip1].m_fPosition) / len;

    // clamp and reduce to single sample when possible
    if (fLerpFactor <= 0.0f)
    {
      fLerpFactor = 0.0f;
      uiClip2 = uiClip1;
    }
    else if (fLerpFactor >= 1.0f)
    {
      fLerpFactor = 1.0f;
      uiClip1 = uiClip2;
    }
  }

  plResourceLock<plAnimationClipResource> pAnimClip1(clip1.m_hClip, plResourceAcquireMode::BlockTillLoaded);
  plResourceLock<plAnimationClipResource> pAnimClip2(clip2.m_hClip, plResourceAcquireMode::BlockTillLoaded);

  if (pAnimClip1.GetAcquireResult() != plResourceAcquireResult::Final || pAnimClip2.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  const float fAvgClipSpeed = plMath::Lerp(m_Clips[uiClip1].m_fSpeed, m_Clips[uiClip2].m_fSpeed, fLerpFactor);
  const float fSpeed = static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed)) * fAvgClipSpeed;

  const auto& animDesc1 = pAnimClip1->GetDescriptor();
  const auto& animDesc2 = pAnimClip2->GetDescriptor();

  const plTime avgDuration = plMath::Lerp(animDesc1.GetDuration(), animDesc2.GetDuration(), fLerpFactor);
  const float fInvDuration = 1.0f / avgDuration.AsFloatInSeconds();

  const plTime tPrevPlayback = pState->m_PlaybackTime;
  pState->m_PlaybackTime += tDiff * fSpeed;

  plAnimPoseEventTrackSampleMode eventSampling = plAnimPoseEventTrackSampleMode::OnlyBetween;

  if (pState->m_PlaybackTime >= avgDuration)
  {
    if (bLoop)
    {
      pState->m_PlaybackTime -= avgDuration;
      eventSampling = plAnimPoseEventTrackSampleMode::LoopAtEnd;
      m_OutOnStarted.SetTriggered(ref_graph);
    }
    else
    {
      pState->m_PlaybackTime = avgDuration;
      pState->m_bPlaying = false;
      m_OutOnFinished.SetTriggered(ref_graph);
    }
  }

  plAnimGraphPinDataLocalTransforms* pOutputTransform = ref_controller.AddPinDataLocalTransforms();

  auto& poseGen = ref_controller.GetPoseGenerator();

  if (clip1.m_hClip == clip2.m_hClip)
  {
    const void* pThis = this;
    auto& cmd = poseGen.AllocCommandSampleTrack(plHashingUtils::xxHash32(&pThis, sizeof(pThis), 0));
    cmd.m_hAnimationClip = clip1.m_hClip;
    cmd.m_fPreviousNormalizedSamplePos = tPrevPlayback.AsFloatInSeconds() * fInvDuration;
    cmd.m_fNormalizedSamplePos = pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration;
    cmd.m_EventSampling = eventSampling;

    pOutputTransform->m_CommandID = cmd.GetCommandID();
  }
  else
  {
    auto& cmdCmb = poseGen.AllocCommandCombinePoses();
    pOutputTransform->m_CommandID = cmdCmb.GetCommandID();

    // sample animation 1
    {
      const void* pThis = this;
      auto& cmd = poseGen.AllocCommandSampleTrack(plHashingUtils::xxHash32(&pThis, sizeof(pThis), 0));
      cmd.m_hAnimationClip = clip1.m_hClip;
      cmd.m_fPreviousNormalizedSamplePos = tPrevPlayback.AsFloatInSeconds() * fInvDuration;
      cmd.m_fNormalizedSamplePos = pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration;
      cmd.m_EventSampling = fLerpFactor <= 0.5f ? eventSampling : plAnimPoseEventTrackSampleMode::None; // only the stronger influence will trigger events

      cmdCmb.m_Inputs.PushBack(cmd.GetCommandID());
      cmdCmb.m_InputWeights.PushBack(1.0f - fLerpFactor);
    }

    // sample animation 2
    {
      const void* pThis = this;
      auto& cmd = poseGen.AllocCommandSampleTrack(plHashingUtils::xxHash32(&pThis, sizeof(pThis), 1));
      cmd.m_hAnimationClip = clip2.m_hClip;
      cmd.m_fPreviousNormalizedSamplePos = tPrevPlayback.AsFloatInSeconds() * fInvDuration;
      cmd.m_fNormalizedSamplePos = pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration;
      cmd.m_EventSampling = fLerpFactor > 0.5f ? eventSampling : plAnimPoseEventTrackSampleMode::None; // only the stronger influence will trigger events

      cmdCmb.m_Inputs.PushBack(cmd.GetCommandID());
      cmdCmb.m_InputWeights.PushBack(fLerpFactor);
    }
  }

  // send to output
  {
    if (m_bApplyRootMotion)
    {
      pOutputTransform->m_bUseRootMotion = true;

      pOutputTransform->m_vRootMotion = plMath::Lerp(animDesc1.m_vConstantRootMotion, animDesc2.m_vConstantRootMotion, fLerpFactor) * tDiff.AsFloatInSeconds() * fSpeed;
    }

    m_OutPose.SetPose(ref_graph, pOutputTransform);
  }
}

bool plSampleBlendSpace1DAnimNode::GetInstanceDataDesc(plInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceState>();
  return true;
}


PL_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes2_SampleBlendSpace1DAnimNode);


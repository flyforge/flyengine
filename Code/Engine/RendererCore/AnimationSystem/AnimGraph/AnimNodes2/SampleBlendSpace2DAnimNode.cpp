#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SampleBlendSpace2DAnimNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plAnimationClip2D, plNoBase, 1, plRTTIDefaultAllocator<plAnimationClip2D>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Clip", GetAnimationFile, SetAnimationFile)->AddAttributes(new plDynamicStringEnumAttribute("AnimationClipMappingEnum")),
    PL_MEMBER_PROPERTY("Position", m_vPosition),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSampleBlendSpace2DAnimNode, 1, plRTTIDefaultAllocator<plSampleBlendSpace2DAnimNode>)
  {
    PL_BEGIN_PROPERTIES
    {
      PL_MEMBER_PROPERTY("Loop", m_bLoop)->AddAttributes(new plDefaultValueAttribute(true)),
      PL_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, {})),
      PL_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),
      PL_MEMBER_PROPERTY("InputResponse", m_InputResponse)->AddAttributes(new plDefaultValueAttribute(plTime::MakeFromMilliseconds(100))),
    PL_ACCESSOR_PROPERTY("CenterClip", GetCenterClipFile, SetCenterClipFile)->AddAttributes(new plDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      PL_ARRAY_MEMBER_PROPERTY("Clips", m_Clips),

      PL_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new plHiddenAttribute()),
      PL_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new plHiddenAttribute()),
      PL_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new plHiddenAttribute()),
      PL_MEMBER_PROPERTY("X", m_InCoordX)->AddAttributes(new plHiddenAttribute()),
      PL_MEMBER_PROPERTY("Y", m_InCoordY)->AddAttributes(new plHiddenAttribute()),

      PL_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new plHiddenAttribute()),
      PL_MEMBER_PROPERTY("OutOnStarted", m_OutOnStarted)->AddAttributes(new plHiddenAttribute()),
      PL_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new plHiddenAttribute()),
    }
    PL_END_PROPERTIES;
    PL_BEGIN_ATTRIBUTES
    {
      new plCategoryAttribute("Pose Generation"),
      new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Blue)),
      new plTitleAttribute("BlendSpace 2D: '{CenterClip}'"),
    }
    PL_END_ATTRIBUTES;
  }
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plAnimationClip2D::SetAnimationFile(const char* szFile)
{
  m_sClip.Assign(szFile);
}

const char* plAnimationClip2D::GetAnimationFile() const
{
  return m_sClip;
}

plSampleBlendSpace2DAnimNode::plSampleBlendSpace2DAnimNode() = default;
plSampleBlendSpace2DAnimNode::~plSampleBlendSpace2DAnimNode() = default;

plResult plSampleBlendSpace2DAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sCenterClip;

  stream << m_Clips.GetCount();
  for (plUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream << m_Clips[i].m_sClip;
    stream << m_Clips[i].m_vPosition;
  }

  stream << m_bLoop;
  stream << m_bApplyRootMotion;
  stream << m_fPlaybackSpeed;
  stream << m_InputResponse;

  PL_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_InCoordX.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_InCoordY.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutOnStarted.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return PL_SUCCESS;
}

plResult plSampleBlendSpace2DAnimNode::DeserializeNode(plStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sCenterClip;

  plUInt32 num = 0;
  stream >> num;
  m_Clips.SetCount(num);
  for (plUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream >> m_Clips[i].m_sClip;
    stream >> m_Clips[i].m_vPosition;
  }

  stream >> m_bLoop;
  stream >> m_bApplyRootMotion;
  stream >> m_fPlaybackSpeed;
  stream >> m_InputResponse;

  PL_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_InCoordX.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_InCoordY.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutOnStarted.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return PL_SUCCESS;
}

void plSampleBlendSpace2DAnimNode::SetCenterClipFile(const char* szFile)
{
  m_sCenterClip.Assign(szFile);
}

const char* plSampleBlendSpace2DAnimNode::GetCenterClipFile() const
{
  return m_sCenterClip;
}

void plSampleBlendSpace2DAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected() || (!m_InCoordX.IsConnected() && !m_InCoordY.IsConnected()) || m_Clips.IsEmpty())
    return;

  InstanceState* pState = ref_graph.GetAnimNodeInstanceData<InstanceState>(*this);

  if ((!m_InStart.IsConnected() && !pState->m_bPlaying) || m_InStart.IsTriggered(ref_graph))
  {
    pState->m_CenterPlaybackTime = plTime::MakeZero();
    pState->m_fOtherPlaybackPosNorm = 0.0f;
    pState->m_bPlaying = true;

    m_OutOnStarted.SetTriggered(ref_graph);
  }

  if (!pState->m_bPlaying)
    return;

  const float x = static_cast<float>(m_InCoordX.GetNumber(ref_graph));
  const float y = static_cast<float>(m_InCoordY.GetNumber(ref_graph));

  if (m_InputResponse.IsZeroOrNegative())
  {
    pState->m_fLastValueX = x;
    pState->m_fLastValueY = y;
  }
  else
  {
    const float lerp = static_cast<float>(plMath::Min(1.0, tDiff.GetSeconds() * (1.0 / m_InputResponse.GetSeconds())));
    pState->m_fLastValueX = plMath::Lerp(pState->m_fLastValueX, x, lerp);
    pState->m_fLastValueY = plMath::Lerp(pState->m_fLastValueY, y, lerp);
  }

  const auto& centerInfo = ref_controller.GetAnimationClipInfo(m_sCenterClip);

  plUInt32 uiMaxWeightClip = 0;
  plHybridArray<ClipToPlay, 8> clips;
  ComputeClipsAndWeights(centerInfo, plVec2(pState->m_fLastValueX, pState->m_fLastValueY), clips, uiMaxWeightClip);

  PlayClips(ref_controller, centerInfo, pState, ref_graph, tDiff, clips, uiMaxWeightClip);
}

void plSampleBlendSpace2DAnimNode::ComputeClipsAndWeights(const plAnimController::AnimClipInfo& centerInfo, const plVec2& p, plDynamicArray<ClipToPlay>& clips, plUInt32& out_uiMaxWeightClip) const
{
  out_uiMaxWeightClip = 0;
  float fMaxWeight = -1.0f;

  if (m_Clips.GetCount() == 1 && !centerInfo.m_hClip.IsValid())
  {
    clips.ExpandAndGetRef().m_uiIndex = 0;
  }
  else
  {
    // this algorithm is taken from http://runevision.com/thesis chapter 6.3 "Gradient Band Interpolation"
    // also see http://answers.unity.com/answers/1208837/view.html

    float fWeightNormalization = 0.0f;

    for (plUInt32 i = 0; i < m_Clips.GetCount(); ++i)
    {
      const plVec2 pi = m_Clips[i].m_vPosition;
      float fMinWeight = 1.0f;

      for (plUInt32 j = 0; j < m_Clips.GetCount(); ++j)
      {
        const plVec2 pj = m_Clips[j].m_vPosition;

        const float fLenSqr = (pi - pj).GetLengthSquared();
        const float fProjLenSqr = (pi - p).Dot(pi - pj);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight = plMath::Min(fMinWeight, fWeight);
      }

      // also check against center clip
      if (centerInfo.m_hClip.IsValid())
      {
        const float fLenSqr = pi.GetLengthSquared();
        const float fProjLenSqr = (pi - p).Dot(pi);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight = plMath::Min(fMinWeight, fWeight);
      }

      if (fMinWeight > 0.0f)
      {
        auto& c = clips.ExpandAndGetRef();
        c.m_uiIndex = i;
        c.m_fWeight = fMinWeight;

        fWeightNormalization += fMinWeight;
      }
    }

    // also compute weight for center clip
    if (centerInfo.m_hClip.IsValid())
    {
      float fMinWeight = 1.0f;

      for (plUInt32 j = 0; j < m_Clips.GetCount(); ++j)
      {
        const plVec2 pj = m_Clips[j].m_vPosition;

        const float fLenSqr = pj.GetLengthSquared();
        const float fProjLenSqr = (-p).Dot(-pj);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight = plMath::Min(fMinWeight, fWeight);
      }

      if (fMinWeight > 0.0f)
      {
        auto& c = clips.ExpandAndGetRef();
        c.m_uiIndex = 0xFFFFFFFF;
        c.m_fWeight = fMinWeight;

        fWeightNormalization += fMinWeight;
      }
    }

    fWeightNormalization = 1.0f / fWeightNormalization;

    for (plUInt32 i = 0; i < clips.GetCount(); ++i)
    {
      auto& c = clips[i];

      c.m_fWeight *= fWeightNormalization;

      if (c.m_fWeight > fMaxWeight)
      {
        fMaxWeight = c.m_fWeight;
        out_uiMaxWeightClip = i;
      }
    }
  }
}

void plSampleBlendSpace2DAnimNode::PlayClips(plAnimController& ref_controller, const plAnimController::AnimClipInfo& centerInfo, InstanceState* pState, plAnimGraphInstance& ref_graph, plTime tDiff, plArrayPtr<ClipToPlay> clips, plUInt32 uiMaxWeightClip) const
{
  const bool bLoop = m_InLoop.GetBool(ref_graph, m_bLoop);
  const float fSpeed = static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed));

  plTime tAvgDuration = plTime::MakeZero();

  plHybridArray<plAnimPoseGeneratorCommandSampleTrack*, 8> pSampleTrack;
  pSampleTrack.SetCountUninitialized(clips.GetCount());

  plVec3 vRootMotion = plVec3::MakeZero();
  plUInt32 uiNumAvgClips = 0;

  for (plUInt32 i = 0; i < clips.GetCount(); ++i)
  {
    const auto& c = clips[i];

    const plHashedString sClip = c.m_uiIndex >= 0xFF ? m_sCenterClip : m_Clips[c.m_uiIndex].m_sClip;

    const auto& clipInfo = ref_controller.GetAnimationClipInfo(sClip);

    plResourceLock<plAnimationClipResource> pClip(clipInfo.m_hClip, plResourceAcquireMode::BlockTillLoaded);

    if (c.m_uiIndex < 0xFF) // center clip should not contribute to the average time
    {
      ++uiNumAvgClips;
      tAvgDuration += pClip->GetDescriptor().GetDuration();
    }

    const void* pThis = this;
    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(plHashingUtils::xxHash32(&pThis, sizeof(pThis), i));
    cmd.m_hAnimationClip = clipInfo.m_hClip;
    cmd.m_fNormalizedSamplePos = pClip->GetDescriptor().GetDuration().AsFloatInSeconds(); // will be combined with actual pos below

    pSampleTrack[i] = &cmd;
    vRootMotion += pClip->GetDescriptor().m_vConstantRootMotion * c.m_fWeight;
  }

  if (uiNumAvgClips > 0)
  {
    tAvgDuration = tAvgDuration / uiNumAvgClips;
  }

  tAvgDuration = plMath::Max(tAvgDuration, plTime::MakeFromMilliseconds(16));

  const plTime fPrevCenterPlaybackPos = pState->m_CenterPlaybackTime;
  const float fPrevPlaybackPosNorm = pState->m_fOtherPlaybackPosNorm;

  plAnimPoseEventTrackSampleMode eventSamplingCenter = plAnimPoseEventTrackSampleMode::OnlyBetween;
  plAnimPoseEventTrackSampleMode eventSampling = plAnimPoseEventTrackSampleMode::OnlyBetween;

  const float fInvAvgDuration = 1.0f / tAvgDuration.AsFloatInSeconds();
  const float tDiffNorm = tDiff.AsFloatInSeconds() * fInvAvgDuration;

  // now that we know the duration, we can finally update the playback state
  pState->m_fOtherPlaybackPosNorm += tDiffNorm * fSpeed;
  while (pState->m_fOtherPlaybackPosNorm >= 1.0f)
  {
    if (bLoop)
    {
      pState->m_fOtherPlaybackPosNorm -= 1.0f;
      m_OutOnStarted.SetTriggered(ref_graph);
      eventSampling = plAnimPoseEventTrackSampleMode::LoopAtEnd;
    }
    else
    {
      pState->m_fOtherPlaybackPosNorm = 1.0f;
      pState->m_bPlaying = false;
      m_OutOnFinished.SetTriggered(ref_graph);
      break;
    }
  }

  UpdateCenterClipPlaybackTime(centerInfo, pState, ref_graph, tDiff, eventSamplingCenter);

  for (plUInt32 i = 0; i < clips.GetCount(); ++i)
  {
    if (pSampleTrack[i]->m_hAnimationClip == centerInfo.m_hClip)
    {
      pSampleTrack[i]->m_fPreviousNormalizedSamplePos = fPrevCenterPlaybackPos.AsFloatInSeconds() / pSampleTrack[i]->m_fNormalizedSamplePos;
      pSampleTrack[i]->m_fNormalizedSamplePos = pState->m_CenterPlaybackTime.AsFloatInSeconds() / pSampleTrack[i]->m_fNormalizedSamplePos;
      pSampleTrack[i]->m_EventSampling = uiMaxWeightClip == i ? eventSamplingCenter : plAnimPoseEventTrackSampleMode::None;
    }
    else
    {
      pSampleTrack[i]->m_fPreviousNormalizedSamplePos = fPrevPlaybackPosNorm;
      pSampleTrack[i]->m_fNormalizedSamplePos = pState->m_fOtherPlaybackPosNorm;
      pSampleTrack[i]->m_EventSampling = uiMaxWeightClip == i ? eventSampling : plAnimPoseEventTrackSampleMode::None;
    }
  }

  plAnimGraphPinDataLocalTransforms* pOutputTransform = ref_controller.AddPinDataLocalTransforms();

  if (m_bApplyRootMotion)
  {
    pOutputTransform->m_bUseRootMotion = true;

    const float fSpeed = static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed));

    pOutputTransform->m_vRootMotion = tDiff.AsFloatInSeconds() * vRootMotion * fSpeed;
  }

  if (clips.GetCount() == 1)
  {
    pOutputTransform->m_CommandID = pSampleTrack[0]->GetCommandID();
  }
  else
  {
    auto& cmdCmb = ref_controller.GetPoseGenerator().AllocCommandCombinePoses();
    pOutputTransform->m_CommandID = cmdCmb.GetCommandID();

    cmdCmb.m_InputWeights.SetCountUninitialized(clips.GetCount());
    cmdCmb.m_Inputs.SetCountUninitialized(clips.GetCount());

    for (plUInt32 i = 0; i < clips.GetCount(); ++i)
    {
      cmdCmb.m_InputWeights[i] = clips[i].m_fWeight;
      cmdCmb.m_Inputs[i] = pSampleTrack[i]->GetCommandID();
    }
  }

  m_OutPose.SetPose(ref_graph, pOutputTransform);
}

void plSampleBlendSpace2DAnimNode::UpdateCenterClipPlaybackTime(const plAnimController::AnimClipInfo& centerInfo, InstanceState* pState, plAnimGraphInstance& ref_graph, plTime tDiff, plAnimPoseEventTrackSampleMode& out_eventSamplingCenter) const
{
  const float fSpeed = static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed));

  if (centerInfo.m_hClip.IsValid())
  {
    plResourceLock<plAnimationClipResource> pClip(centerInfo.m_hClip, plResourceAcquireMode::BlockTillLoaded);

    const plTime tDur = pClip->GetDescriptor().GetDuration();

    pState->m_CenterPlaybackTime += tDiff * fSpeed;

    // always loop the center clip
    while (pState->m_CenterPlaybackTime > tDur)
    {
      pState->m_CenterPlaybackTime -= tDur;
      out_eventSamplingCenter = plAnimPoseEventTrackSampleMode::LoopAtEnd;
    }
  }
}

bool plSampleBlendSpace2DAnimNode::GetInstanceDataDesc(plInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceState>();
  return true;
}


PL_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes2_SampleBlendSpace2DAnimNode);


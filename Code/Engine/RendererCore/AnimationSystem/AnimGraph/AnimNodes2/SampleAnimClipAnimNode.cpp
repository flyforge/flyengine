#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SampleAnimClipAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSampleAnimClipAnimNode, 1, plRTTIDefaultAllocator<plSampleAnimClipAnimNode>)
  {
    PLASMA_BEGIN_PROPERTIES
    {
      PLASMA_MEMBER_PROPERTY("Loop", m_bLoop)->AddAttributes(new plDefaultValueAttribute(true)),
      PLASMA_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, {})),
      PLASMA_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),
      PLASMA_ACCESSOR_PROPERTY("Clip", GetClip, SetClip)->AddAttributes(new plDynamicStringEnumAttribute("AnimationClipMappingEnum")),

      PLASMA_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new plHiddenAttribute()),
      PLASMA_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new plHiddenAttribute()),
      PLASMA_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new plHiddenAttribute()),

      PLASMA_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new plHiddenAttribute()),
      PLASMA_MEMBER_PROPERTY("OutOnStarted", m_OutOnStarted)->AddAttributes(new plHiddenAttribute()),
      PLASMA_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new plHiddenAttribute()),
    }
    PLASMA_END_PROPERTIES;
    PLASMA_BEGIN_ATTRIBUTES
    {
      new plCategoryAttribute("Pose Generation"),
      new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Blue)),
      new plTitleAttribute("Sample Clip: '{Clip}'"),
    }
    PLASMA_END_ATTRIBUTES;
  }
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSampleAnimClipAnimNode::plSampleAnimClipAnimNode() = default;
plSampleAnimClipAnimNode::~plSampleAnimClipAnimNode() = default;

plResult plSampleAnimClipAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sClip;
  stream << m_bLoop;
  stream << m_bApplyRootMotion;
  stream << m_fPlaybackSpeed;

  PLASMA_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnStarted.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plSampleAnimClipAnimNode::DeserializeNode(plStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sClip;
  stream >> m_bLoop;
  stream >> m_bApplyRootMotion;
  stream >> m_fPlaybackSpeed;

  PLASMA_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnStarted.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return PLASMA_SUCCESS;
}

void plSampleAnimClipAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  PLASMA_PROFILE_SCOPE("AnimNode_Clip");
  const auto& clipInfo = ref_controller.GetAnimationClipInfo(m_sClip);

  if (!clipInfo.m_hClip.IsValid() || !m_OutPose.IsConnected())
    return;

  InstanceState* pState = ref_graph.GetAnimNodeInstanceData<InstanceState>(*this);

  if ((!m_InStart.IsConnected() && !pState->m_bPlaying) || m_InStart.IsTriggered(ref_graph))
  {
    pState->m_PlaybackTime = plTime::Zero();
    pState->m_bPlaying = true;

    m_OutOnStarted.SetTriggered(ref_graph);
  }

  if (!pState->m_bPlaying)
    return;

  plResourceLock<plAnimationClipResource> pAnimClip(clipInfo.m_hClip, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimClip.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  const plTime tDuration = pAnimClip->GetDescriptor().GetDuration();
  const float fInvDuration = 1.0f / tDuration.AsFloatInSeconds();

  // currently we only support playing clips forwards
  const float fPlaySpeed = plMath::Max(0.0f, static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed)));

  const plTime tPrevSamplePos = pState->m_PlaybackTime;
  pState->m_PlaybackTime += tDiff * fPlaySpeed;

  const bool bLoop = m_InLoop.GetBool(ref_graph, m_bLoop);

  const void* pThis = this;
  auto& cmd = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(plHashingUtils::xxHash32(&pThis, sizeof(pThis)));
  cmd.m_EventSampling = plAnimPoseEventTrackSampleMode::OnlyBetween;

  if (bLoop && pState->m_PlaybackTime > tDuration)
  {
    pState->m_PlaybackTime -= tDuration;
    cmd.m_EventSampling = plAnimPoseEventTrackSampleMode::LoopAtEnd;
    m_OutOnStarted.SetTriggered(ref_graph);
  }

  cmd.m_hAnimationClip = clipInfo.m_hClip;
  cmd.m_fPreviousNormalizedSamplePos = plMath::Clamp(tPrevSamplePos.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);
  cmd.m_fNormalizedSamplePos = plMath::Clamp(pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);

  {
    plAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

    pLocalTransforms->m_pWeights = nullptr;
    pLocalTransforms->m_bUseRootMotion = m_bApplyRootMotion;
    pLocalTransforms->m_fOverallWeight = 1.0f;
    pLocalTransforms->m_vRootMotion = pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * fPlaySpeed;
    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    m_OutPose.SetPose(ref_graph, pLocalTransforms);
  }

  if (cmd.m_fNormalizedSamplePos >= 1.0f && !bLoop)
  {
    m_OutOnFinished.SetTriggered(ref_graph);
    pState->m_bPlaying = false;
  }
}

void plSampleAnimClipAnimNode::SetClip(const char* szClip)
{
  m_sClip.Assign(szClip);
}

const char* plSampleAnimClipAnimNode::GetClip() const
{
  return m_sClip.GetData();
}

bool plSampleAnimClipAnimNode::GetInstanceDataDesc(plInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceState>();
  return true;
}

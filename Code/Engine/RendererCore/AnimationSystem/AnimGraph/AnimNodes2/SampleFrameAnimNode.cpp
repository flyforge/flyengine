#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SampleFrameAnimNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSampleFrameAnimNode, 1, plRTTIDefaultAllocator<plSampleFrameAnimNode>)
  {
    PL_BEGIN_PROPERTIES
    {
      PL_ACCESSOR_PROPERTY("Clip", GetClip, SetClip)->AddAttributes(new plDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      PL_MEMBER_PROPERTY("NormPos", m_fNormalizedSamplePosition)->AddAttributes(new plDefaultValueAttribute(0.0f), new plClampValueAttribute(0.0f, 1.0f)),

      PL_MEMBER_PROPERTY("InNormPos", m_InNormalizedSamplePosition)->AddAttributes(new plHiddenAttribute()),
      PL_MEMBER_PROPERTY("InAbsPos", m_InAbsoluteSamplePosition)->AddAttributes(new plHiddenAttribute()),

      PL_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new plHiddenAttribute()),
    }
    PL_END_PROPERTIES;
    PL_BEGIN_ATTRIBUTES
    {
      new plCategoryAttribute("Pose Generation"),
      new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Blue)),
      new plTitleAttribute("Sample Frame: '{Clip}'"),
    }
    PL_END_ATTRIBUTES;
  }
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plSampleFrameAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sClip;
  stream << m_fNormalizedSamplePosition;

  PL_SUCCEED_OR_RETURN(m_InNormalizedSamplePosition.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_InAbsoluteSamplePosition.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));

  return PL_SUCCESS;
}

plResult plSampleFrameAnimNode::DeserializeNode(plStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sClip;
  stream >> m_fNormalizedSamplePosition;

  PL_SUCCEED_OR_RETURN(m_InNormalizedSamplePosition.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_InAbsoluteSamplePosition.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return PL_SUCCESS;
}

void plSampleFrameAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected())
    return;

  const auto& clip = ref_controller.GetAnimationClipInfo(m_sClip);

  if (clip.m_hClip.IsValid())
  {
    plResourceLock<plAnimationClipResource> pAnimClip(clip.m_hClip, plResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pAnimClip.GetAcquireResult() != plResourceAcquireResult::Final)
      return;

    float fNormPos = fNormPos = m_InNormalizedSamplePosition.GetNumber(ref_graph, m_fNormalizedSamplePosition);

    if (m_InAbsoluteSamplePosition.IsConnected())
    {
      const plTime tDuration = pAnimClip->GetDescriptor().GetDuration();
      const float fInvDuration = 1.0f / tDuration.AsFloatInSeconds();
      fNormPos = m_InAbsoluteSamplePosition.GetNumber(ref_graph) * fInvDuration;
    }

    fNormPos = plMath::Clamp(fNormPos, 0.0f, 1.0f);

    const void* pThis = this;
    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(plHashingUtils::xxHash32(&pThis, sizeof(pThis)));

    cmd.m_hAnimationClip = clip.m_hClip;
    cmd.m_fPreviousNormalizedSamplePos = fNormPos;
    cmd.m_fNormalizedSamplePos = fNormPos;
    cmd.m_EventSampling = plAnimPoseEventTrackSampleMode::None;

    {
      plAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      pLocalTransforms->m_pWeights = nullptr;
      pLocalTransforms->m_bUseRootMotion = false;
      pLocalTransforms->m_fOverallWeight = 1.0f;
      pLocalTransforms->m_CommandID = cmd.GetCommandID();

      m_OutPose.SetPose(ref_graph, pLocalTransforms);
    }
  }
  else
  {
    const void* pThis = this;
    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandRestPose();

    {
      plAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      pLocalTransforms->m_pWeights = nullptr;
      pLocalTransforms->m_bUseRootMotion = false;
      pLocalTransforms->m_fOverallWeight = 1.0f;
      pLocalTransforms->m_CommandID = cmd.GetCommandID();

      m_OutPose.SetPose(ref_graph, pLocalTransforms);
    }
  }
}

void plSampleFrameAnimNode::SetClip(const char* szClip)
{
  m_sClip.Assign(szClip);
}

const char* plSampleFrameAnimNode::GetClip() const
{
  return m_sClip.GetData();
}


PL_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes2_SampleFrameAnimNode);


#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/SimpleAnimationComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>

using namespace ozz;
using namespace ozz::animation;
using namespace ozz::math;

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plSimpleAnimationComponent, 2, plComponentMode::Static);
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("AnimationClip", GetAnimationClipFile, SetAnimationClipFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    PLASMA_ENUM_MEMBER_PROPERTY("AnimationMode", plPropertyAnimMode, m_AnimationMode),
    PLASMA_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_ENUM_MEMBER_PROPERTY("RootMotionMode", plRootMotionMode, m_RootMotionMode),
    PLASMA_ENUM_MEMBER_PROPERTY("InvisibleUpdateRate", plAnimationInvisibleUpdateRate, m_InvisibleUpdateRate),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation"),
    new plColorAttribute(plColorScheme::Animation),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plSimpleAnimationComponent::plSimpleAnimationComponent() = default;
plSimpleAnimationComponent::~plSimpleAnimationComponent() = default;

void plSimpleAnimationComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_AnimationMode;
  s << m_fSpeed;
  s << m_hAnimationClip;
  s << m_RootMotionMode;
  s << m_InvisibleUpdateRate;
}

void plSimpleAnimationComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_AnimationMode;
  s >> m_fSpeed;
  s >> m_hAnimationClip;
  s >> m_RootMotionMode;

  if (uiVersion >= 2)
  {
    s >> m_InvisibleUpdateRate;
  }
}

void plSimpleAnimationComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  plMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  m_hSkeleton = msg.m_hSkeleton;
}

void plSimpleAnimationComponent::SetAnimationClip(const plAnimationClipResourceHandle& hResource)
{
  m_hAnimationClip = hResource;
}

const plAnimationClipResourceHandle& plSimpleAnimationComponent::GetAnimationClip() const
{
  return m_hAnimationClip;
}

void plSimpleAnimationComponent::SetAnimationClipFile(const char* szFile)
{
  plAnimationClipResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plAnimationClipResource>(szFile);
  }

  SetAnimationClip(hResource);
}

const char* plSimpleAnimationComponent::GetAnimationClipFile() const
{
  if (!m_hAnimationClip.IsValid())
    return "";

  return m_hAnimationClip.GetResourceID();
}

void plSimpleAnimationComponent::SetNormalizedPlaybackPosition(float fPosition)
{
  m_fNormalizedPlaybackPosition = fPosition;

  // force update next time
  SetUserFlag(1, true);
}

void plSimpleAnimationComponent::Update()
{
  if (!m_hSkeleton.IsValid() || !m_hAnimationClip.IsValid())
    return;

  if (m_fSpeed == 0.0f && !GetUserFlag(1))
    return;

  plTime tMinStep = plTime::Seconds(0);
  plVisibilityState visType = GetOwner()->GetVisibilityState();

  if (visType != plVisibilityState::Direct)
  {
    if (m_InvisibleUpdateRate == plAnimationInvisibleUpdateRate::Pause && visType == plVisibilityState::Invisible)
      return;

    tMinStep = plAnimationInvisibleUpdateRate::GetTimeStep(m_InvisibleUpdateRate);
  }

  m_ElapsedTimeSinceUpdate += GetWorld()->GetClock().GetTimeDiff();

  if (m_ElapsedTimeSinceUpdate < tMinStep)
    return;

  const bool bVisible = visType != plVisibilityState::Invisible;

  plResourceLock<plAnimationClipResource> pAnimation(m_hAnimationClip, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimation.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  const plTime tDiff = m_ElapsedTimeSinceUpdate;
  m_ElapsedTimeSinceUpdate.SetZero();

  const plAnimationClipResourceDescriptor& animDesc = pAnimation->GetDescriptor();

  m_Duration = animDesc.GetDuration();

  const float fPrevPlaybackPos = m_fNormalizedPlaybackPosition;

  plAnimPoseEventTrackSampleMode mode = plAnimPoseEventTrackSampleMode::None;

  if (!UpdatePlaybackTime(tDiff, animDesc.m_EventTrack, mode))
    return;

  if (animDesc.m_EventTrack.IsEmpty())
  {
    mode = plAnimPoseEventTrackSampleMode::None;
  }

  // no need to do anything, if we can't get events and are currently invisible
  if (!bVisible && mode == plAnimPoseEventTrackSampleMode::None && m_RootMotionMode == plRootMotionMode::Ignore)
    return;

  plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  plAnimPoseGenerator poseGen;
  poseGen.Reset(pSkeleton.GetPointer());

  auto& cmdSample = poseGen.AllocCommandSampleTrack(0);
  cmdSample.m_hAnimationClip = m_hAnimationClip;
  cmdSample.m_fNormalizedSamplePos = m_fNormalizedPlaybackPosition;
  cmdSample.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
  cmdSample.m_EventSampling = mode;

  if (bVisible)
  {
    auto& cmdL2M = poseGen.AllocCommandLocalToModelPose();
    cmdL2M.m_pSendLocalPoseMsgTo = GetOwner();

    if (animDesc.m_bAdditive)
    {
      auto& cmdComb = poseGen.AllocCommandCombinePoses();
      cmdComb.m_Inputs.PushBack(cmdSample.GetCommandID());
      cmdComb.m_InputWeights.PushBack(1.0f);

      cmdL2M.m_Inputs.PushBack(cmdComb.GetCommandID());
    }
    else
    {
      cmdL2M.m_Inputs.PushBack(cmdSample.GetCommandID());
    }

    auto& cmdOut = poseGen.AllocCommandModelPoseToOutput();
    cmdOut.m_Inputs.PushBack(cmdL2M.GetCommandID());
  }

  auto pose = poseGen.GeneratePose(GetOwner());

  if (m_RootMotionMode != plRootMotionMode::Ignore)
  {
    plVec3 vRootMotion = tDiff.AsFloatInSeconds() * m_fSpeed * animDesc.m_vConstantRootMotion;

    const bool bReverse = GetUserFlag(0);
    if (bReverse)
    {
      vRootMotion = -vRootMotion;
    }

    // only applies positional root motion
    plRootMotionMode::Apply(m_RootMotionMode, GetOwner(), vRootMotion, plAngle(), plAngle(), plAngle());
  }

  if (pose.IsEmpty())
    return;

  // inform child nodes/components that a new pose is available
  {
    plMsgAnimationPoseProposal msg1;
    msg1.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
    msg1.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
    msg1.m_ModelTransforms = pose;

    GetOwner()->SendMessage(msg1);

    if (msg1.m_bContinueAnimating)
    {
      plMsgAnimationPoseUpdated msg2;
      msg2.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
      msg2.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
      msg2.m_ModelTransforms = pose;

      // recursive, so that objects below the mesh can also listen in on these changes
      // for example bone attachments
      GetOwner()->SendMessageRecursive(msg2);

      if (msg2.m_bContinueAnimating == false)
      {
        SetActiveFlag(false);
      }
    }
  }
}

bool plSimpleAnimationComponent::UpdatePlaybackTime(plTime tDiff, const plEventTrack& eventTrack, plAnimPoseEventTrackSampleMode& out_trackSampling)
{
  if (tDiff.IsZero() || m_fSpeed == 0.0f)
  {
    if (GetUserFlag(1))
    {
      SetUserFlag(1, false);
      return true;
    }

    return false;
  }

  out_trackSampling = plAnimPoseEventTrackSampleMode::OnlyBetween;

  const float tDiffNorm = static_cast<float>(tDiff.GetSeconds() / m_Duration.GetSeconds());
  const float tPrefNorm = m_fNormalizedPlaybackPosition;

  switch (m_AnimationMode)
  {
    case plPropertyAnimMode::Once:
    {
      m_fNormalizedPlaybackPosition += tDiffNorm * m_fSpeed;
      m_fNormalizedPlaybackPosition = plMath::Clamp(m_fNormalizedPlaybackPosition, 0.0f, 1.0f);
      break;
    }

    case plPropertyAnimMode::Loop:
    {
      m_fNormalizedPlaybackPosition += tDiffNorm * m_fSpeed;

      if (m_fNormalizedPlaybackPosition < 0.0f)
      {
        m_fNormalizedPlaybackPosition += 1.0f;

        out_trackSampling = plAnimPoseEventTrackSampleMode::LoopAtStart;
      }
      else if (m_fNormalizedPlaybackPosition > 1.0f)
      {
        m_fNormalizedPlaybackPosition -= 1.0f;

        out_trackSampling = plAnimPoseEventTrackSampleMode::LoopAtEnd;
      }

      break;
    }

    case plPropertyAnimMode::BackAndForth:
    {
      const bool bReverse = GetUserFlag(0);

      if (bReverse)
        m_fNormalizedPlaybackPosition -= tDiffNorm * m_fSpeed;
      else
        m_fNormalizedPlaybackPosition += tDiffNorm * m_fSpeed;

      if (m_fNormalizedPlaybackPosition > 1.0f)
      {
        SetUserFlag(0, !bReverse);

        m_fNormalizedPlaybackPosition = 2.0f - m_fNormalizedPlaybackPosition;

        out_trackSampling = plAnimPoseEventTrackSampleMode::BounceAtEnd;
      }
      else if (m_fNormalizedPlaybackPosition < 0.0f)
      {
        SetUserFlag(0, !bReverse);

        m_fNormalizedPlaybackPosition = -m_fNormalizedPlaybackPosition;

        out_trackSampling = plAnimPoseEventTrackSampleMode::BounceAtStart;
      }

      break;
    }
  }

  return tPrefNorm != m_fNormalizedPlaybackPosition;
}


PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_SimpleAnimationComponent);

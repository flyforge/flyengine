#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Shader/Types.h>

// clang-format off
PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgAnimationPosePreparing);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAnimationPosePreparing, 1, plRTTIDefaultAllocator<plMsgAnimationPosePreparing>)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgAnimationPoseUpdated);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAnimationPoseUpdated, 1, plRTTIDefaultAllocator<plMsgAnimationPoseUpdated>)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgAnimationPoseProposal);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAnimationPoseProposal, 1, plRTTIDefaultAllocator<plMsgAnimationPoseProposal>)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgRopePoseUpdated);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgRopePoseUpdated, 1, plRTTIDefaultAllocator<plMsgRopePoseUpdated>)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgQueryAnimationSkeleton);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgQueryAnimationSkeleton, 1, plRTTIDefaultAllocator<plMsgQueryAnimationSkeleton>)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgApplyRootMotion);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgApplyRootMotion, 1, plRTTIDefaultAllocator<plMsgApplyRootMotion>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Translation", m_vTranslation),
    PLASMA_MEMBER_PROPERTY("RotationX", m_RotationX),
    PLASMA_MEMBER_PROPERTY("RotationY", m_RotationY),
    PLASMA_MEMBER_PROPERTY("RotationZ", m_RotationZ),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgRetrieveBoneState);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgRetrieveBoneState, 1, plRTTIDefaultAllocator<plMsgRetrieveBoneState>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plAnimationInvisibleUpdateRate, 1)
  PLASMA_ENUM_CONSTANT(plAnimationInvisibleUpdateRate::FullUpdate),
  PLASMA_ENUM_CONSTANT(plAnimationInvisibleUpdateRate::Max60FPS),
  PLASMA_ENUM_CONSTANT(plAnimationInvisibleUpdateRate::Max30FPS),
  PLASMA_ENUM_CONSTANT(plAnimationInvisibleUpdateRate::Max15FPS),
  PLASMA_ENUM_CONSTANT(plAnimationInvisibleUpdateRate::Max10FPS),
  PLASMA_ENUM_CONSTANT(plAnimationInvisibleUpdateRate::Max5FPS),
  PLASMA_ENUM_CONSTANT(plAnimationInvisibleUpdateRate::Pause),
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

plTime plAnimationInvisibleUpdateRate::GetTimeStep(plAnimationInvisibleUpdateRate::Enum value)
{
  switch (value)
  {
    case plAnimationInvisibleUpdateRate::FullUpdate:
      return plTime::Zero();
    case plAnimationInvisibleUpdateRate::Max60FPS:
      return plTime::Seconds(1.0 / 60.0);
    case plAnimationInvisibleUpdateRate::Max30FPS:
      return plTime::Seconds(1.0 / 30.0);
    case plAnimationInvisibleUpdateRate::Max15FPS:
      return plTime::Seconds(1.0 / 15.0);
    case plAnimationInvisibleUpdateRate::Max10FPS:
      return plTime::Seconds(1.0 / 10.0);

    case plAnimationInvisibleUpdateRate::Max5FPS:
    case plAnimationInvisibleUpdateRate::Pause: // full pausing should be handled separately, and if something isn't fully paused, it should behave like a very low update rate
      return plTime::Seconds(1.0 / 5.0);

      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return plTime::Zero();
}

void plMsgAnimationPoseUpdated::ComputeFullBoneTransform(plUInt32 uiJointIndex, plMat4& ref_mFullTransform) const
{
  ref_mFullTransform = m_pRootTransform->GetAsMat4() * m_ModelTransforms[uiJointIndex];
}

void plMsgAnimationPoseUpdated::ComputeFullBoneTransform(const plMat4& mRootTransform, const plMat4& mModelTransform, plMat4& ref_mFullTransform, plQuat& ref_qRotationOnly)
{
  ref_mFullTransform = mRootTransform * mModelTransform;

  // the bone might contain (non-uniform) scaling and mirroring, which the quaternion can't represent
  // so reconstruct a representable rotation matrix
  ref_qRotationOnly.ReconstructFromMat4(ref_mFullTransform);
}

void plMsgAnimationPoseUpdated::ComputeFullBoneTransform(plUInt32 uiJointIndex, plMat4& ref_mFullTransform, plQuat& ref_qRotationOnly) const
{
  ComputeFullBoneTransform(m_pRootTransform->GetAsMat4(), m_ModelTransforms[uiJointIndex], ref_mFullTransform, ref_qRotationOnly);
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_AnimationPose);

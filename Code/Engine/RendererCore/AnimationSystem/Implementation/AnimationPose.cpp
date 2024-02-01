#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererFoundation/Shader/Types.h>

// clang-format off
PL_IMPLEMENT_MESSAGE_TYPE(plMsgAnimationPosePreparing);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAnimationPosePreparing, 1, plRTTIDefaultAllocator<plMsgAnimationPosePreparing>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgAnimationPoseUpdated);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAnimationPoseUpdated, 1, plRTTIDefaultAllocator<plMsgAnimationPoseUpdated>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgRopePoseUpdated);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgRopePoseUpdated, 1, plRTTIDefaultAllocator<plMsgRopePoseUpdated>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgQueryAnimationSkeleton);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgQueryAnimationSkeleton, 1, plRTTIDefaultAllocator<plMsgQueryAnimationSkeleton>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgApplyRootMotion);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgApplyRootMotion, 1, plRTTIDefaultAllocator<plMsgApplyRootMotion>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Translation", m_vTranslation),
    PL_MEMBER_PROPERTY("RotationX", m_RotationX),
    PL_MEMBER_PROPERTY("RotationY", m_RotationY),
    PL_MEMBER_PROPERTY("RotationZ", m_RotationZ),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgRetrieveBoneState);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgRetrieveBoneState, 1, plRTTIDefaultAllocator<plMsgRetrieveBoneState>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_ENUM(plAnimationInvisibleUpdateRate, 1)
  PL_ENUM_CONSTANT(plAnimationInvisibleUpdateRate::FullUpdate),
  PL_ENUM_CONSTANT(plAnimationInvisibleUpdateRate::Max60FPS),
  PL_ENUM_CONSTANT(plAnimationInvisibleUpdateRate::Max30FPS),
  PL_ENUM_CONSTANT(plAnimationInvisibleUpdateRate::Max15FPS),
  PL_ENUM_CONSTANT(plAnimationInvisibleUpdateRate::Max10FPS),
  PL_ENUM_CONSTANT(plAnimationInvisibleUpdateRate::Max5FPS),
  PL_ENUM_CONSTANT(plAnimationInvisibleUpdateRate::Pause),
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

plTime plAnimationInvisibleUpdateRate::GetTimeStep(plAnimationInvisibleUpdateRate::Enum value)
{
  switch (value)
  {
    case plAnimationInvisibleUpdateRate::FullUpdate:
      return plTime::MakeZero();
    case plAnimationInvisibleUpdateRate::Max60FPS:
      return plTime::MakeFromSeconds(1.0 / 60.0);
    case plAnimationInvisibleUpdateRate::Max30FPS:
      return plTime::MakeFromSeconds(1.0 / 30.0);
    case plAnimationInvisibleUpdateRate::Max15FPS:
      return plTime::MakeFromSeconds(1.0 / 15.0);
    case plAnimationInvisibleUpdateRate::Max10FPS:
      return plTime::MakeFromSeconds(1.0 / 10.0);

    case plAnimationInvisibleUpdateRate::Max5FPS:
    case plAnimationInvisibleUpdateRate::Pause: // full pausing should be handled separately, and if something isn't fully paused, it should behave like a very low update rate
      return plTime::MakeFromSeconds(1.0 / 5.0);

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return plTime::MakeZero();
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

PL_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_AnimationPose);

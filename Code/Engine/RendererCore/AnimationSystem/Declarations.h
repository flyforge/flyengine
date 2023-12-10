#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Communication/Message.h>
#include <RendererCore/RendererCoreDLL.h>
#include <ozz/base/maths/soa_transform.h>

class plSkeleton;
class plAnimationPose;
struct plSkeletonResourceDescriptor;
class plEditableSkeletonJoint;
struct plAnimationClipResourceDescriptor;

using plSkeletonResourceHandle = plTypedResourceHandle<class plSkeletonResource>;

#define plInvalidJointIndex static_cast<plUInt16>(0xFFFFu)

namespace ozz::animation
{
  class Skeleton;
}

struct plSkeletonJointGeometryType
{
  using StorageType = plUInt8;

  enum Enum
  {
    None = 0,
    Capsule,
    Sphere,
    Box,
    ConvexMesh,

    Default = None
  };
};

/// \brief Used by components that skin a mesh to inform children whenever a new pose is being prepared.
///
/// The pose matrices are still in local space and in the ozz internal structure-of-arrays format.
/// At this point individual bones can still be modified, to propagate the effect to the child bones.
struct PLASMA_RENDERERCORE_DLL plMsgAnimationPosePreparing : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgAnimationPosePreparing, plMessage);

  const plSkeleton* m_pSkeleton = nullptr;
  plArrayPtr<ozz::math::SoaTransform> m_LocalTransforms;
};

/// \brief Used by components that skin a mesh to inform children whenever a new pose has been computed.
///
/// This can be used by child nodes/components to synchronize their state to the new animation pose.
/// The message is sent while the pose is in object space.
/// Both skeleton and pose pointer are always valid.
struct PLASMA_RENDERERCORE_DLL plMsgAnimationPoseUpdated : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgAnimationPoseUpdated, plMessage);

  static void ComputeFullBoneTransform(const plMat4& mRootTransform, const plMat4& mModelTransform, plMat4& ref_mFullTransform, plQuat& ref_qRotationOnly);
  void ComputeFullBoneTransform(plUInt32 uiJointIndex, plMat4& ref_mFullTransform) const;
  void ComputeFullBoneTransform(plUInt32 uiJointIndex, plMat4& ref_mFullTransform, plQuat& ref_qRotationOnly) const;

  const plTransform* m_pRootTransform = nullptr;
  const plSkeleton* m_pSkeleton = nullptr;
  plArrayPtr<const plMat4> m_ModelTransforms;
  bool m_bContinueAnimating = true;
};

struct PLASMA_RENDERERCORE_DLL plMsgAnimationPoseProposal : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgAnimationPoseProposal, plMessage);

  const plTransform* m_pRootTransform = nullptr;
  const plSkeleton* m_pSkeleton = nullptr;
  plArrayPtr<const plMat4> m_ModelTransforms;
  bool m_bContinueAnimating = true;
};

/// \brief Used by components that do rope simulation and rendering.
///
/// The rope simulation component sends this message to components attached to the same game object,
/// every time there is a new rope pose. There is no skeleton information, since all joints/bones are
/// connected as one long string.
///
/// For a rope with N segments, N+1 poses are sent. The last pose may use the same rotation as the one before.
struct PLASMA_RENDERERCORE_DLL plMsgRopePoseUpdated : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgRopePoseUpdated, plMessage);

  plArrayPtr<const plTransform> m_LinkTransforms;
};

/// \brief The animated mesh component listens to this message and 'answers' by filling out the skeleton resource handle.
///
/// This can be used by components that require a skeleton, to ask the nearby components to provide it to them.
struct PLASMA_RENDERERCORE_DLL plMsgQueryAnimationSkeleton : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgQueryAnimationSkeleton, plMessage);

  plSkeletonResourceHandle m_hSkeleton;
};

/// \brief This message is sent when animation root motion data is available.
///
/// Listening components can use this to move a character.
struct PLASMA_RENDERERCORE_DLL plMsgApplyRootMotion : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgApplyRootMotion, plMessage);

  plVec3 m_vTranslation;
  plAngle m_RotationX;
  plAngle m_RotationY;
  plAngle m_RotationZ;
};

/// \brief Queries the local transforms of each bone in an object with a skeleton
///
/// Used to retrieve the pose of a ragdoll after simulation.
struct PLASMA_RENDERERCORE_DLL plMsgRetrieveBoneState : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgRetrieveBoneState, plMessage);

  // maps from bone name to its local transform
  plMap<plString, plTransform> m_BoneTransforms;
};

struct plSkeletonJointType
{
  using StorageType = plUInt8;

  enum Enum
  {
    None,
    Fixed,
    //  Hinge,
    SwingTwist,

    Default = None,
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plSkeletonJointType);

//////////////////////////////////////////////////////////////////////////

/// \brief What to do when an animated object is not visible.
struct PLASMA_RENDERERCORE_DLL plAnimationInvisibleUpdateRate
{
  using StorageType = plUInt8;

  enum Enum
  {
    FullUpdate,
    Max60FPS,
    Max30FPS,
    Max15FPS,
    Max10FPS,
    Max5FPS,
    Pause,

    Default = Max5FPS
  };

  static plTime GetTimeStep(plAnimationInvisibleUpdateRate::Enum value);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plAnimationInvisibleUpdateRate);

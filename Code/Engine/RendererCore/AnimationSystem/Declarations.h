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

/// \brief What shape is used to approximate a bone's geometry
struct plSkeletonJointGeometryType
{
  using StorageType = plUInt8;

  enum Enum
  {
    None = 0,
    Capsule,
    Sphere,
    Box,
    ConvexMesh, ///< A convex mesh is extracted from the mesh file.

    Default = None
  };
};

/// \brief Used by components that skin a mesh to inform children whenever a new pose is being prepared.
///
/// The pose matrices are still in local space and in the ozz internal structure-of-arrays format.
/// At this point individual bones can still be modified, to propagate the effect to the child bones.
struct PL_RENDERERCORE_DLL plMsgAnimationPosePreparing : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgAnimationPosePreparing, plMessage);

  const plSkeleton* m_pSkeleton = nullptr;
  plArrayPtr<ozz::math::SoaTransform> m_LocalTransforms;
};

/// \brief Used by components that skin a mesh to inform children whenever a new pose has been computed.
///
/// This can be used by child nodes/components to synchronize their state to the new animation pose.
/// The message is sent while the pose is in object space.
/// Both skeleton and pose pointer are always valid.
struct PL_RENDERERCORE_DLL plMsgAnimationPoseUpdated : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgAnimationPoseUpdated, plMessage);

  static void ComputeFullBoneTransform(const plMat4& mRootTransform, const plMat4& mModelTransform, plMat4& ref_mFullTransform, plQuat& ref_qRotationOnly);
  void ComputeFullBoneTransform(plUInt32 uiJointIndex, plMat4& ref_mFullTransform) const;
  void ComputeFullBoneTransform(plUInt32 uiJointIndex, plMat4& ref_mFullTransform, plQuat& ref_qRotationOnly) const;

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
struct PL_RENDERERCORE_DLL plMsgRopePoseUpdated : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgRopePoseUpdated, plMessage);

  plArrayPtr<const plTransform> m_LinkTransforms;
};

/// \brief The animated mesh component listens to this message and 'answers' by filling out the skeleton resource handle.
///
/// This can be used by components that require a skeleton, to ask the nearby components to provide it to them.
struct PL_RENDERERCORE_DLL plMsgQueryAnimationSkeleton : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgQueryAnimationSkeleton, plMessage);

  plSkeletonResourceHandle m_hSkeleton;
};

/// \brief This message is sent when animation root motion data is available.
///
/// Listening components can use this to move a character.
struct PL_RENDERERCORE_DLL plMsgApplyRootMotion : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgApplyRootMotion, plMessage);

  plVec3 m_vTranslation;
  plAngle m_RotationX;
  plAngle m_RotationY;
  plAngle m_RotationZ;
};

/// \brief Queries the local transforms of each bone in an object with a skeleton
///
/// Used to retrieve the pose of a ragdoll after simulation.
struct PL_RENDERERCORE_DLL plMsgRetrieveBoneState : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgRetrieveBoneState, plMessage);

  // maps from bone name to its local transform
  plMap<plString, plTransform> m_BoneTransforms;
};

/// \brief What type of physics constraint to use for a bone.
struct plSkeletonJointType
{
  using StorageType = plUInt8;

  enum Enum
  {
    None,  ///< The bone is not constrained, at all. It will not be connected to another bone and fall down separately.
    Fixed, ///< The bone is joined to the parent bone by a fixed joint type and can't move, at all.
    //  Hinge,
    SwingTwist, ///< The bone is joined to the parent bone and can swing and twist relative to it in limited fashion.

    Default = None,
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plSkeletonJointType);

//////////////////////////////////////////////////////////////////////////

/// \brief What to do when an animated object is not visible.
///
/// It is often important to still update animated meshes, so that animation events get handled.
/// Also even though a mesh may be invisible itself, its shadow or reflection may still be visible.
struct PL_RENDERERCORE_DLL plAnimationInvisibleUpdateRate
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

PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plAnimationInvisibleUpdateRate);

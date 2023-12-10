#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Math/Mat3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/Declarations.h>

class plStreamWriter;
class plStreamReader;
class plSkeletonBuilder;
class plSkeleton;

using plSurfaceResourceHandle = plTypedResourceHandle<class plSurfaceResource>;

namespace ozz::animation
{
  class Skeleton;
}

/// \brief Describes a single joint.
/// The transforms of the joints are in their local space and thus need to be correctly multiplied with their parent transforms to get the
/// final transform.
class PLASMA_RENDERERCORE_DLL plSkeletonJoint
{
public:
  const plTransform& GetRestPoseLocalTransform() const { return m_RestPoseLocal; }

  /// \brief Returns plInvalidJointIndex if no parent
  plUInt16 GetParentIndex() const { return m_uiParentIndex; }

  bool IsRootJoint() const { return m_uiParentIndex == plInvalidJointIndex; }
  const plHashedString& GetName() const { return m_sName; }

  plAngle GetHalfSwingLimitY() const { return m_HalfSwingLimitY; }
  plAngle GetHalfSwingLimitZ() const { return m_HalfSwingLimitZ; }
  plAngle GetTwistLimitHalfAngle() const { return m_TwistLimitHalfAngle; }
  plAngle GetTwistLimitCenterAngle() const { return m_TwistLimitCenterAngle; }
  plAngle GetTwistLimitLow() const;
  plAngle GetTwistLimitHigh() const;
  plEnum<plSkeletonJointType> GetJointType() const { return m_JointType; }

  plQuat GetLocalOrientation() const { return m_qLocalJointOrientation; }

  plSurfaceResourceHandle GetSurface() const { return m_hSurface; }
  plUInt8 GetCollisionLayer() const { return m_uiCollisionLayer; }

  float GetStiffness() const { return m_fStiffness; }
  void SetStiffness(float fValue) { m_fStiffness = fValue; }

private:
  friend plSkeleton;
  friend plSkeletonBuilder;

  plTransform m_RestPoseLocal;
  plUInt16 m_uiParentIndex = plInvalidJointIndex;
  plHashedString m_sName;

  plSurfaceResourceHandle m_hSurface;
  plUInt8 m_uiCollisionLayer = 0;

  plEnum<plSkeletonJointType> m_JointType;
  plQuat m_qLocalJointOrientation = plQuat::MakeIdentity();
  plAngle m_HalfSwingLimitY;
  plAngle m_HalfSwingLimitZ;
  plAngle m_TwistLimitHalfAngle;
  plAngle m_TwistLimitCenterAngle;
  float m_fStiffness = 0.0f;
};

/// \brief The skeleton class encapsulates the information about the joint structure for a model.
class PLASMA_RENDERERCORE_DLL plSkeleton
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plSkeleton);

public:
  plSkeleton();
  plSkeleton(plSkeleton&& rhs);
  ~plSkeleton();

  void operator=(plSkeleton&& rhs);

  /// \brief Returns the number of joints in the skeleton.
  plUInt16 GetJointCount() const { return static_cast<plUInt16>(m_Joints.GetCount()); }

  /// \brief Returns the nth joint.
  const plSkeletonJoint& GetJointByIndex(plUInt16 uiIndex) const { return m_Joints[uiIndex]; }

  /// \brief Allows to find a specific joint in the skeleton by name. Returns plInvalidJointIndex if not found
  plUInt16 FindJointByName(const plTempHashedString& sName) const;

  /// \brief Checks if two skeletons are compatible (same joint count and hierarchy)
  // bool IsCompatibleWith(const plSkeleton& other) const;

  /// \brief Saves the skeleton in a given stream.
  void Save(plStreamWriter& inout_stream) const;

  /// \brief Loads the skeleton from the given stream.
  void Load(plStreamReader& inout_stream);

  bool IsJointDescendantOf(plUInt16 uiJoint, plUInt16 uiExpectedParent) const;

  const ozz::animation::Skeleton& GetOzzSkeleton() const;

  plUInt64 GetHeapMemoryUsage() const;

  /// \brief The direction in which the bones shall point for visualization
  plEnum<plBasisAxis> m_BoneDirection;

protected:
  friend plSkeletonBuilder;

  plDynamicArray<plSkeletonJoint> m_Joints;
  mutable plUniquePtr<ozz::animation::Skeleton> m_pOzzSkeleton;
};

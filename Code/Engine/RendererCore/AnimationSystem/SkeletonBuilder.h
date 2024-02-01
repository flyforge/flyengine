#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/RendererCoreDLL.h>

/// \brief The skeleton builder class provides the means to build skeleton instances from scratch.
/// This class is not necessary to use skeletons, usually they should be deserialized from data created by the tools.
class PL_RENDERERCORE_DLL plSkeletonBuilder
{

public:
  plSkeletonBuilder();
  ~plSkeletonBuilder();

  /// \brief Adds a joint to the skeleton
  /// Since the only way to add a joint with a parent is through this method the order of joints in the array is guaranteed
  /// so that child joints always come after their parent joints
  plUInt16 AddJoint(plStringView sName, const plTransform& localRestPose, plUInt16 uiParentIndex = plInvalidJointIndex);

  void SetJointLimit(plUInt16 uiJointIndex, const plQuat& qLocalOrientation, plSkeletonJointType::Enum jointType, plAngle halfSwingLimitY, plAngle halfSwingLimitZ, plAngle twistLimitHalfAngle, plAngle twistLimitCenterAngle, float fStiffness);

  void SetJointSurface(plUInt16 uiJointIndex, plStringView sSurface);
  void SetJointCollisionLayer(plUInt16 uiJointIndex, plUInt8 uiCollsionLayer);

  /// \brief Creates a skeleton from the accumulated data.
  void BuildSkeleton(plSkeleton& ref_skeleton) const;

  /// \brief Returns true if there any joints have been added to the skeleton builder
  bool HasJoints() const;

protected:
  struct BuilderJoint
  {
    plTransform m_RestPoseLocal;
    plTransform m_RestPoseGlobal; // this one is temporary and not stored in the final plSkeleton
    plTransform m_InverseRestPoseGlobal;
    plUInt16 m_uiParentIndex = plInvalidJointIndex;
    plHashedString m_sName;
    plEnum<plSkeletonJointType> m_JointType;
    plQuat m_qLocalJointOrientation = plQuat::MakeIdentity();
    plAngle m_HalfSwingLimitZ;
    plAngle m_HalfSwingLimitY;
    plAngle m_TwistLimitHalfAngle;
    plAngle m_TwistLimitCenterAngle;
    float m_fStiffness = 0.0f;

    plString m_sSurface;
    plUInt8 m_uiCollisionLayer = 0;
  };

  plDeque<BuilderJoint> m_Joints;
};

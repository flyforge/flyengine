#include <RendererCore/RendererCorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/AnimationSystem/SkeletonBuilder.h>

plSkeletonBuilder::plSkeletonBuilder() = default;
plSkeletonBuilder::~plSkeletonBuilder() = default;

plUInt16 plSkeletonBuilder::AddJoint(plStringView sName, const plTransform& localRestPose, plUInt16 uiParentIndex /*= plInvalidJointIndex*/)
{
  PL_ASSERT_DEV(uiParentIndex == plInvalidJointIndex || uiParentIndex < m_Joints.GetCount(), "Invalid parent index for joint");

  auto& joint = m_Joints.ExpandAndGetRef();

  joint.m_RestPoseLocal = localRestPose;
  joint.m_RestPoseGlobal = localRestPose;
  joint.m_sName.Assign(sName);
  joint.m_uiParentIndex = uiParentIndex;

  if (uiParentIndex != plInvalidJointIndex)
  {
    joint.m_RestPoseGlobal = m_Joints[joint.m_uiParentIndex].m_RestPoseGlobal * joint.m_RestPoseLocal;
  }

  joint.m_InverseRestPoseGlobal = joint.m_RestPoseGlobal.GetInverse();

  return static_cast<plUInt16>(m_Joints.GetCount() - 1);
}

void plSkeletonBuilder::SetJointLimit(plUInt16 uiJointIndex, const plQuat& qLocalOrientation, plSkeletonJointType::Enum jointType, plAngle halfSwingLimitY, plAngle halfSwingLimitZ, plAngle twistLimitHalfAngle, plAngle twistLimitCenterAngle, float fStiffness)
{
  auto& j = m_Joints[uiJointIndex];
  j.m_qLocalJointOrientation = qLocalOrientation;
  j.m_JointType = jointType;
  j.m_HalfSwingLimitY = halfSwingLimitY;
  j.m_HalfSwingLimitZ = halfSwingLimitZ;
  j.m_TwistLimitHalfAngle = twistLimitHalfAngle;
  j.m_TwistLimitCenterAngle = twistLimitCenterAngle;
  j.m_fStiffness = fStiffness;
}


void plSkeletonBuilder::SetJointSurface(plUInt16 uiJointIndex, plStringView sSurface)
{
  auto& j = m_Joints[uiJointIndex];
  j.m_sSurface = sSurface;
}

void plSkeletonBuilder::SetJointCollisionLayer(plUInt16 uiJointIndex, plUInt8 uiCollsionLayer)
{
  auto& j = m_Joints[uiJointIndex];
  j.m_uiCollisionLayer = uiCollsionLayer;
}

void plSkeletonBuilder::BuildSkeleton(plSkeleton& ref_skeleton) const
{
  // PL_ASSERT_DEV(HasJoints(), "Can't build a skeleton with no joints!");

  const plUInt32 numJoints = m_Joints.GetCount();

  // Copy joints to skeleton
  ref_skeleton.m_Joints.SetCount(numJoints);

  for (plUInt32 i = 0; i < numJoints; ++i)
  {
    ref_skeleton.m_Joints[i].m_sName = m_Joints[i].m_sName;
    ref_skeleton.m_Joints[i].m_uiParentIndex = m_Joints[i].m_uiParentIndex;
    ref_skeleton.m_Joints[i].m_RestPoseLocal = m_Joints[i].m_RestPoseLocal;

    ref_skeleton.m_Joints[i].m_JointType = m_Joints[i].m_JointType;
    ref_skeleton.m_Joints[i].m_qLocalJointOrientation = m_Joints[i].m_qLocalJointOrientation;
    ref_skeleton.m_Joints[i].m_HalfSwingLimitY = m_Joints[i].m_HalfSwingLimitY;
    ref_skeleton.m_Joints[i].m_HalfSwingLimitZ = m_Joints[i].m_HalfSwingLimitZ;
    ref_skeleton.m_Joints[i].m_TwistLimitHalfAngle = m_Joints[i].m_TwistLimitHalfAngle;
    ref_skeleton.m_Joints[i].m_TwistLimitCenterAngle = m_Joints[i].m_TwistLimitCenterAngle;

    ref_skeleton.m_Joints[i].m_uiCollisionLayer = m_Joints[i].m_uiCollisionLayer;
    ref_skeleton.m_Joints[i].m_hSurface = plResourceManager::LoadResource<plSurfaceResource>(m_Joints[i].m_sSurface);
    ref_skeleton.m_Joints[i].m_fStiffness = m_Joints[i].m_fStiffness;
  }
}

bool plSkeletonBuilder::HasJoints() const
{
  return !m_Joints.IsEmpty();
}



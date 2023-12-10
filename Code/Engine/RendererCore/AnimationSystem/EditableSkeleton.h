#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <RendererCore/AnimationSystem/Declarations.h>

class plSkeletonBuilder;
class plSkeleton;

namespace ozz::animation
{
  class Skeleton;

  namespace offline
  {
    struct RawSkeleton;
  }
} // namespace ozz::animation

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plSkeletonJointGeometryType);

struct PLASMA_RENDERERCORE_DLL plEditableSkeletonBoneShape : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEditableSkeletonBoneShape, plReflectedClass);

  plEnum<plSkeletonJointGeometryType> m_Geometry;

  plVec3 m_vOffset = plVec3::MakeZero();
  plQuat m_qRotation = plQuat::MakeIdentity();

  float m_fLength = 0;    // Box, Capsule; 0 means parent joint to this joint (auto mode)
  float m_fWidth = 0;     // Box
  float m_fThickness = 0; // Sphere radius, Capsule radius
};

struct PLASMA_RENDERERCORE_DLL plEditableSkeletonBoneCollider : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEditableSkeletonBoneCollider, plReflectedClass);

  plString m_sIdentifier;
  plDynamicArray<plVec3> m_VertexPositions;
  plDynamicArray<plUInt8> m_TriangleIndices;
};

class PLASMA_RENDERERCORE_DLL plEditableSkeletonJoint : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEditableSkeletonJoint, plReflectedClass);

public:
  plEditableSkeletonJoint();
  ~plEditableSkeletonJoint();

  const char* GetName() const;
  void SetName(const char* szSz);

  void ClearJoints();

  // copies the properties for geometry etc. from another joint
  // does NOT copy the name, the transform or the children
  void CopyPropertiesFrom(const plEditableSkeletonJoint* pJoint);

  plHashedString m_sName;
  plTransform m_LocalTransform = plTransform::MakeIdentity();

  plEnum<plSkeletonJointType> m_JointType;

  float m_fStiffness = 0.0f;

  plAngle m_TwistLimitHalfAngle;
  plAngle m_TwistLimitCenterAngle;
  plAngle m_SwingLimitY;
  plAngle m_SwingLimitZ;

  plVec3 m_vGizmoOffsetPositionRO = plVec3::MakeZero();
  plQuat m_qGizmoOffsetRotationRO = plQuat::MakeIdentity();

  plQuat m_qLocalJointRotation = plQuat::MakeIdentity();

  plHybridArray<plEditableSkeletonJoint*, 4> m_Children;
  plHybridArray<plEditableSkeletonBoneShape, 1> m_BoneShapes;
  plDynamicArray<plEditableSkeletonBoneCollider> m_BoneColliders;

  bool m_bOverrideSurface = false;
  bool m_bOverrideCollisionLayer = false;
  plString m_sSurfaceOverride;
  plUInt8 m_uiCollisionLayerOverride;
};

class PLASMA_RENDERERCORE_DLL plEditableSkeleton : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEditableSkeleton, plReflectedClass);

public:
  plEditableSkeleton();
  ~plEditableSkeleton();

  void ClearJoints();
  void FillResourceDescriptor(plSkeletonResourceDescriptor& ref_desc) const;
  void GenerateRawOzzSkeleton(ozz::animation::offline::RawSkeleton& out_skeleton) const;
  void GenerateOzzSkeleton(ozz::animation::Skeleton& out_skeleton) const;
  void CreateJointsRecursive(plSkeletonBuilder& ref_sb, plSkeletonResourceDescriptor& ref_desc, const plEditableSkeletonJoint* pParentJoint, const plEditableSkeletonJoint* pThisJoint, plUInt16 uiThisJointIdx, const plQuat& qParentAccuRot, const plMat4& mRootTransform) const;

  plString m_sSourceFile;
  plString m_sPreviewMesh;

  plString m_sSurfaceFile;
  plUInt8 m_uiCollisionLayer = 0;

  float m_fUniformScaling = 1.0f;
  float m_fMaxImpulse = 100.0f;

  plEnum<plBasisAxis> m_RightDir;
  plEnum<plBasisAxis> m_UpDir;
  bool m_bFlipForwardDir = false;
  plEnum<plBasisAxis> m_BoneDirection;

  plHybridArray<plEditableSkeletonJoint*, 4> m_Children;
};

struct PLASMA_RENDERERCORE_DLL plExposedBone
{
  plString m_sName;
  plString m_sParent;
  plTransform m_Transform;
  // when adding new values, the hash function below has to be adjusted
};

PLASMA_DECLARE_CUSTOM_VARIANT_TYPE(plExposedBone);

PLASMA_RENDERERCORE_DLL void operator<<(plStreamWriter& inout_stream, const plExposedBone& bone);
PLASMA_RENDERERCORE_DLL void operator>>(plStreamReader& inout_stream, plExposedBone& ref_bone);
PLASMA_RENDERERCORE_DLL bool operator==(const plExposedBone& lhs, const plExposedBone& rhs);

template <>
struct plHashHelper<plExposedBone>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(const plExposedBone& value)
  {
    return plHashingUtils::xxHash32String(value.m_sName) + plHashingUtils::xxHash32String(value.m_sParent) + plHashingUtils::xxHash32(&value, sizeof(plTransform));
  }

  PLASMA_ALWAYS_INLINE static bool Equal(const plExposedBone& a, const plExposedBone& b) { return a == b; }
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plExposedBone);

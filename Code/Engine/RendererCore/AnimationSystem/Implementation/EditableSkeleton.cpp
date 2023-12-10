#include <RendererCore/RendererCorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <RendererCore/AnimationSystem/SkeletonBuilder.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plSkeletonJointGeometryType, 1)
PLASMA_ENUM_CONSTANTS(plSkeletonJointGeometryType::None, plSkeletonJointGeometryType::Capsule, plSkeletonJointGeometryType::Sphere, plSkeletonJointGeometryType::Box)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditableSkeletonBoneShape, 1, plRTTIDefaultAllocator<plEditableSkeletonBoneShape>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("Geometry", plSkeletonJointGeometryType, m_Geometry),
    PLASMA_MEMBER_PROPERTY("Offset", m_vOffset),
    PLASMA_MEMBER_PROPERTY("Rotation", m_qRotation),
    PLASMA_MEMBER_PROPERTY("Length", m_fLength)->AddAttributes(new plDefaultValueAttribute(0.1f), new plClampValueAttribute(0.01f, 10.0f)),
    PLASMA_MEMBER_PROPERTY("Width", m_fWidth)->AddAttributes(new plDefaultValueAttribute(0.05f), new plClampValueAttribute(0.01f, 10.0f)),
    PLASMA_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new plDefaultValueAttribute(0.05f), new plClampValueAttribute(0.01f, 10.0f)),

  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditableSkeletonBoneCollider, 1, plRTTIDefaultAllocator<plEditableSkeletonBoneCollider>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Identifier", m_sIdentifier)->AddAttributes(new plHiddenAttribute()),
    PLASMA_ARRAY_MEMBER_PROPERTY("VertexPositions", m_VertexPositions)->AddAttributes(new plHiddenAttribute()),
    PLASMA_ARRAY_MEMBER_PROPERTY("TriangleIndices", m_TriangleIndices)->AddAttributes(new plHiddenAttribute()),

  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditableSkeletonJoint, 2, plRTTIDefaultAllocator<plEditableSkeletonJoint>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Name", GetName, SetName)->AddAttributes(new plReadOnlyAttribute()),
    PLASMA_MEMBER_PROPERTY("Transform", m_LocalTransform)->AddFlags(plPropertyFlags::Hidden)->AddAttributes(new plDefaultValueAttribute(plTransform::MakeIdentity())),
    PLASMA_MEMBER_PROPERTY_READ_ONLY("GizmoOffsetTranslationRO", m_vGizmoOffsetPositionRO)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY_READ_ONLY("GizmoOffsetRotationRO", m_qGizmoOffsetRotationRO)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("LocalRotation", m_qLocalJointRotation),
    PLASMA_ENUM_MEMBER_PROPERTY("JointType", plSkeletonJointType, m_JointType),
    PLASMA_MEMBER_PROPERTY("Stiffness", m_fStiffness)->AddAttributes(new plDefaultValueAttribute(10.0f)),
    PLASMA_MEMBER_PROPERTY("SwingLimitY", m_SwingLimitY)->AddAttributes(new plClampValueAttribute(plAngle(), plAngle::MakeFromDegree(170)), new plDefaultValueAttribute(plAngle::MakeFromDegree(30))),
    PLASMA_MEMBER_PROPERTY("SwingLimitZ", m_SwingLimitZ)->AddAttributes(new plClampValueAttribute(plAngle(), plAngle::MakeFromDegree(170)), new plDefaultValueAttribute(plAngle::MakeFromDegree(30))),
    PLASMA_MEMBER_PROPERTY("TwistLimitHalfAngle", m_TwistLimitHalfAngle)->AddAttributes(new plClampValueAttribute(plAngle::MakeFromDegree(10), plAngle::MakeFromDegree(170)), new plDefaultValueAttribute(plAngle::MakeFromDegree(30))),
    PLASMA_MEMBER_PROPERTY("TwistLimitCenterAngle", m_TwistLimitCenterAngle)->AddAttributes(new plClampValueAttribute(-plAngle::MakeFromDegree(170), plAngle::MakeFromDegree(170))),

    PLASMA_MEMBER_PROPERTY("OverrideSurface", m_bOverrideSurface),
    PLASMA_MEMBER_PROPERTY("Surface", m_sSurfaceOverride)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface", plDependencyFlags::Package)),
    PLASMA_MEMBER_PROPERTY("OverrideCollisionLayer", m_bOverrideCollisionLayer),
    PLASMA_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayerOverride)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),

    PLASMA_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddFlags(plPropertyFlags::PointerOwner | plPropertyFlags::Hidden),
    PLASMA_ARRAY_MEMBER_PROPERTY("BoneShapes", m_BoneShapes),
    PLASMA_ARRAY_MEMBER_PROPERTY("Colliders", m_BoneColliders)->AddAttributes(new plContainerAttribute(false, false, false)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTransformManipulatorAttribute(nullptr, "LocalRotation", nullptr, "GizmoOffsetTranslationRO", "GizmoOffsetRotationRO"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditableSkeleton, 1, plRTTIDefaultAllocator<plEditableSkeleton>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("File", m_sSourceFile)->AddAttributes(new plFileBrowserAttribute("Select Mesh", plFileBrowserAttribute::MeshesWithAnimations)),
    PLASMA_ENUM_MEMBER_PROPERTY("RightDir", plBasisAxis, m_RightDir)->AddAttributes(new plDefaultValueAttribute((int)plBasisAxis::PositiveX)),
    PLASMA_ENUM_MEMBER_PROPERTY("UpDir", plBasisAxis, m_UpDir)->AddAttributes(new plDefaultValueAttribute((int)plBasisAxis::PositiveY)),
    PLASMA_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    PLASMA_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0001f, 10000.0f)),
    PLASMA_ENUM_MEMBER_PROPERTY("BoneDirection", plBasisAxis, m_BoneDirection)->AddAttributes(new plDefaultValueAttribute((int)plBasisAxis::PositiveY)),
    PLASMA_MEMBER_PROPERTY("PreviewMesh", m_sPreviewMesh)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned", plDependencyFlags::None)),
    PLASMA_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PLASMA_MEMBER_PROPERTY("Surface", m_sSurfaceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface", plDependencyFlags::Package)),
    PLASMA_MEMBER_PROPERTY("MaxImpulse", m_fMaxImpulse)->AddAttributes(new plDefaultValueAttribute(100.f)),

    PLASMA_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddFlags(plPropertyFlags::PointerOwner | plPropertyFlags::Hidden),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plExposedBone, plNoBase, 1, plRTTIDefaultAllocator<plExposedBone>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Name", m_sName),
    PLASMA_MEMBER_PROPERTY("Parent", m_sParent),
    PLASMA_MEMBER_PROPERTY("Transform", m_Transform),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plExposedBone);
// clang-format on


void operator<<(plStreamWriter& inout_stream, const plExposedBone& bone)
{
  inout_stream << bone.m_sName;
  inout_stream << bone.m_sParent;
  inout_stream << bone.m_Transform;
}

void operator>>(plStreamReader& inout_stream, plExposedBone& ref_bone)
{
  inout_stream >> ref_bone.m_sName;
  inout_stream >> ref_bone.m_sParent;
  inout_stream >> ref_bone.m_Transform;
}

bool operator==(const plExposedBone& lhs, const plExposedBone& rhs)
{
  if (lhs.m_sName != rhs.m_sName)
    return false;
  if (lhs.m_sParent != rhs.m_sParent)
    return false;
  if (lhs.m_Transform != rhs.m_Transform)
    return false;
  return true;
}

plEditableSkeleton::plEditableSkeleton() = default;
plEditableSkeleton::~plEditableSkeleton()
{
  ClearJoints();
}

void plEditableSkeleton::ClearJoints()
{
  for (plEditableSkeletonJoint* pChild : m_Children)
  {
    PLASMA_DEFAULT_DELETE(pChild);
  }

  m_Children.Clear();
}

void plEditableSkeleton::CreateJointsRecursive(plSkeletonBuilder& ref_sb, plSkeletonResourceDescriptor& ref_desc, const plEditableSkeletonJoint* pParentJoint, const plEditableSkeletonJoint* pThisJoint, plUInt16 uiThisJointIdx, const plQuat& qParentAccuRot, const plMat4& mRootTransform) const
{
  for (auto& shape : pThisJoint->m_BoneShapes)
  {
    auto& geo = ref_desc.m_Geometry.ExpandAndGetRef();

    geo.m_Type = shape.m_Geometry;
    geo.m_uiAttachedToJoint = static_cast<plUInt16>(uiThisJointIdx);
    geo.m_Transform.SetIdentity();
    geo.m_Transform.m_vScale.Set(shape.m_fLength, shape.m_fWidth, shape.m_fThickness);
    geo.m_Transform.m_vPosition = shape.m_vOffset;
    geo.m_Transform.m_qRotation = shape.m_qRotation;
  }

  for (auto& shape : pThisJoint->m_BoneColliders)
  {
    auto& geo = ref_desc.m_Geometry.ExpandAndGetRef();
    geo.m_Type = plSkeletonJointGeometryType::ConvexMesh;
    geo.m_uiAttachedToJoint = static_cast<plUInt16>(uiThisJointIdx);
    geo.m_Transform.SetIdentity();
    geo.m_VertexPositions = shape.m_VertexPositions;
    geo.m_TriangleIndices = shape.m_TriangleIndices;
  }

  const plVec3 s = pThisJoint->m_LocalTransform.m_vScale;
  if (!s.IsEqual(plVec3(1), 0.1f))
  {
    // plLog::Warning("Mesh bone '{}' has scaling values of {}/{}/{} - this is not supported.", pThisJoint->m_sName, s.x, s.y, s.z);
  }

  const plQuat qThisAccuRot = qParentAccuRot * pThisJoint->m_LocalTransform.m_qRotation;
  plQuat qParentGlobalRot;

  {
    // as always, the root transform is the bane of my existence
    // since it can contain mirroring, the final global rotation of a joint will be incorrect if we don't incorporate the root scale
    // unfortunately this can't be done once for the first node, but has to be done on the result instead

    plMat4 full;
    plMsgAnimationPoseUpdated::ComputeFullBoneTransform(mRootTransform, qParentAccuRot.GetAsMat4(), full, qParentGlobalRot);
  }

  ref_sb.SetJointLimit(uiThisJointIdx, pThisJoint->m_qLocalJointRotation, pThisJoint->m_JointType, pThisJoint->m_SwingLimitY, pThisJoint->m_SwingLimitZ, pThisJoint->m_TwistLimitHalfAngle, pThisJoint->m_TwistLimitCenterAngle, pThisJoint->m_fStiffness);

  ref_sb.SetJointCollisionLayer(uiThisJointIdx, pThisJoint->m_bOverrideCollisionLayer ? pThisJoint->m_uiCollisionLayerOverride : m_uiCollisionLayer);
  ref_sb.SetJointSurface(uiThisJointIdx, pThisJoint->m_bOverrideSurface ? pThisJoint->m_sSurfaceOverride : m_sSurfaceFile);

  for (const auto* pChildJoint : pThisJoint->m_Children)
  {
    const plUInt16 uiChildJointIdx = ref_sb.AddJoint(pChildJoint->GetName(), pChildJoint->m_LocalTransform, uiThisJointIdx);

    CreateJointsRecursive(ref_sb, ref_desc, pThisJoint, pChildJoint, uiChildJointIdx, qThisAccuRot, mRootTransform);
  }
}

void plEditableSkeleton::FillResourceDescriptor(plSkeletonResourceDescriptor& ref_desc) const
{
  ref_desc.m_fMaxImpulse = m_fMaxImpulse;
  ref_desc.m_Geometry.Clear();

  plSkeletonBuilder sb;
  for (const auto* pJoint : m_Children)
  {
    const plUInt16 idx = sb.AddJoint(pJoint->GetName(), pJoint->m_LocalTransform);

    CreateJointsRecursive(sb, ref_desc, nullptr, pJoint, idx, plQuat::MakeIdentity(), ref_desc.m_RootTransform.GetAsMat4());
  }

  sb.BuildSkeleton(ref_desc.m_Skeleton);
  ref_desc.m_Skeleton.m_BoneDirection = m_BoneDirection;
}

static void BuildOzzRawSkeleton(const plEditableSkeletonJoint& srcJoint, ozz::animation::offline::RawSkeleton::Joint& ref_dstJoint)
{
  ref_dstJoint.name = srcJoint.m_sName.GetString();
  ref_dstJoint.transform.translation.x = srcJoint.m_LocalTransform.m_vPosition.x;
  ref_dstJoint.transform.translation.y = srcJoint.m_LocalTransform.m_vPosition.y;
  ref_dstJoint.transform.translation.z = srcJoint.m_LocalTransform.m_vPosition.z;
  ref_dstJoint.transform.rotation.x = srcJoint.m_LocalTransform.m_qRotation.x;
  ref_dstJoint.transform.rotation.y = srcJoint.m_LocalTransform.m_qRotation.y;
  ref_dstJoint.transform.rotation.z = srcJoint.m_LocalTransform.m_qRotation.z;
  ref_dstJoint.transform.rotation.w = srcJoint.m_LocalTransform.m_qRotation.w;
  ref_dstJoint.transform.scale.x = srcJoint.m_LocalTransform.m_vScale.x;
  ref_dstJoint.transform.scale.y = srcJoint.m_LocalTransform.m_vScale.y;
  ref_dstJoint.transform.scale.z = srcJoint.m_LocalTransform.m_vScale.z;

  ref_dstJoint.children.resize((size_t)srcJoint.m_Children.GetCount());

  for (plUInt32 b = 0; b < srcJoint.m_Children.GetCount(); ++b)
  {
    BuildOzzRawSkeleton(*srcJoint.m_Children[b], ref_dstJoint.children[b]);
  }
}

void plEditableSkeleton::GenerateRawOzzSkeleton(ozz::animation::offline::RawSkeleton& out_skeleton) const
{
  out_skeleton.roots.resize((size_t)m_Children.GetCount());

  for (plUInt32 b = 0; b < m_Children.GetCount(); ++b)
  {
    BuildOzzRawSkeleton(*m_Children[b], out_skeleton.roots[b]);
  }
}

void plEditableSkeleton::GenerateOzzSkeleton(ozz::animation::Skeleton& out_skeleton) const
{
  ozz::animation::offline::RawSkeleton rawSkeleton;
  GenerateRawOzzSkeleton(rawSkeleton);

  ozz::animation::offline::SkeletonBuilder skeletonBuilder;
  auto pNewOzzSkeleton = skeletonBuilder(rawSkeleton);

  plOzzUtils::CopySkeleton(&out_skeleton, pNewOzzSkeleton.get());
}

plEditableSkeletonJoint::plEditableSkeletonJoint() = default;

plEditableSkeletonJoint::~plEditableSkeletonJoint()
{
  ClearJoints();
}

const char* plEditableSkeletonJoint::GetName() const
{
  return m_sName.GetData();
}

void plEditableSkeletonJoint::SetName(const char* szSz)
{
  m_sName.Assign(szSz);
}

void plEditableSkeletonJoint::ClearJoints()
{
  for (plEditableSkeletonJoint* pChild : m_Children)
  {
    PLASMA_DEFAULT_DELETE(pChild);
  }
  m_Children.Clear();
}

void plEditableSkeletonJoint::CopyPropertiesFrom(const plEditableSkeletonJoint* pJoint)
{
  // copy existing (user edited) properties from pJoint into this joint
  // which has just been imported from file

  // do not copy:
  //  name
  //  transform
  //  children
  //  bone collider geometry (vertices, indices)

  // synchronize user config of bone colliders
  for (plUInt32 i = 0; i < m_BoneColliders.GetCount(); ++i)
  {
    auto& dst = m_BoneColliders[i];

    for (plUInt32 j = 0; j < pJoint->m_BoneColliders.GetCount(); ++j)
    {
      const auto& src = pJoint->m_BoneColliders[j];

      if (dst.m_sIdentifier == src.m_sIdentifier)
      {
        // dst.m_bOverrideSurface = src.m_bOverrideSurface;
        // dst.m_bOverrideCollisionLayer = src.m_bOverrideCollisionLayer;
        // dst.m_sSurfaceOverride = src.m_sSurfaceOverride;
        // dst.m_uiCollisionLayerOverride = src.m_uiCollisionLayerOverride;
        break;
      }
    }
  }

  m_BoneShapes = pJoint->m_BoneShapes;
  m_qLocalJointRotation = pJoint->m_qLocalJointRotation;
  m_JointType = pJoint->m_JointType;
  m_SwingLimitY = pJoint->m_SwingLimitY;
  m_SwingLimitZ = pJoint->m_SwingLimitZ;
  m_TwistLimitHalfAngle = pJoint->m_TwistLimitHalfAngle;
  m_TwistLimitCenterAngle = pJoint->m_TwistLimitCenterAngle;
  m_fStiffness = pJoint->m_fStiffness;

  m_bOverrideSurface = pJoint->m_bOverrideSurface;
  m_bOverrideCollisionLayer = pJoint->m_bOverrideCollisionLayer;
  m_sSurfaceOverride = pJoint->m_sSurfaceOverride;
  m_uiCollisionLayerOverride = pJoint->m_uiCollisionLayerOverride;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_EditableSkeleton);

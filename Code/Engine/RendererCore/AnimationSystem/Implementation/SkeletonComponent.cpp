#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/SkeletonComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plSkeletonComponent, 5, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    PLASMA_MEMBER_PROPERTY("VisualizeSkeleton", m_bVisualizeBones)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("VisualizeColliders", m_bVisualizeColliders),
    PLASMA_MEMBER_PROPERTY("VisualizeJoints", m_bVisualizeJoints),
    PLASMA_MEMBER_PROPERTY("VisualizeSwingLimits", m_bVisualizeSwingLimits),
    PLASMA_MEMBER_PROPERTY("VisualizeTwistLimits", m_bVisualizeTwistLimits),
    PLASMA_ACCESSOR_PROPERTY("BonesToHighlight", GetBonesToHighlight, SetBonesToHighlight),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    PLASMA_MESSAGE_HANDLER(plMsgQueryAnimationSkeleton, OnQueryAnimationSkeleton)
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSkeletonComponent::plSkeletonComponent() = default;
plSkeletonComponent::~plSkeletonComponent() = default;

plResult plSkeletonComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  if (m_MaxBounds.IsValid())
  {
    plBoundingBox bbox = m_MaxBounds;
    ref_bounds = plBoundingBoxSphere::MakeFromBox(bbox);
    ref_bounds.Transform(m_RootTransform.GetAsMat4());
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

void plSkeletonComponent::Update()
{
  if (m_hSkeleton.IsValid() && (m_bVisualizeBones || m_bVisualizeColliders || m_bVisualizeJoints || m_bVisualizeSwingLimits || m_bVisualizeTwistLimits))
  {
    plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::AllowLoadingFallback_NeverFail);

    if (pSkeleton.GetAcquireResult() != plResourceAcquireResult::Final)
      return;

    if (m_uiSkeletonChangeCounter != pSkeleton->GetCurrentResourceChangeCounter())
    {
      VisualizeSkeletonDefaultState();
    }

    const plQuat qBoneDir = plBasisAxis::GetBasisRotation_PosX(pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
    const plVec3 vBoneDir = qBoneDir * plVec3(1, 0, 0);
    const plVec3 vBoneTangent = qBoneDir * plVec3(0, 1, 0);

    plDebugRenderer::DrawLines(GetWorld(), m_LinesSkeleton, plColor::White, GetOwner()->GetGlobalTransform());

    for (const auto& shape : m_SpheresShapes)
    {
      plDebugRenderer::DrawLineSphere(GetWorld(), shape.m_Shape, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_BoxShapes)
    {
      plDebugRenderer::DrawLineBox(GetWorld(), shape.m_Shape, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_CapsuleShapes)
    {
      plDebugRenderer::DrawLineCapsuleZ(GetWorld(), shape.m_fLength, shape.m_fRadius, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_AngleShapes)
    {
      plDebugRenderer::DrawAngle(GetWorld(), shape.m_StartAngle, shape.m_EndAngle, plColor::MakeZero(), shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform, vBoneTangent, vBoneDir);
    }

    for (const auto& shape : m_ConeLimitShapes)
    {
      plDebugRenderer::DrawLimitCone(GetWorld(), shape.m_Angle1, shape.m_Angle2, plColor::MakeZero(), shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_CylinderShapes)
    {
      plDebugRenderer::DrawCylinder(GetWorld(), shape.m_fRadius1, shape.m_fRadius2, shape.m_fLength, shape.m_Color, plColor::MakeZero(), GetOwner()->GetGlobalTransform() * shape.m_Transform, false, false);
    }
  }
}

void plSkeletonComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hSkeleton;
  s << m_bVisualizeBones;
  s << m_sBonesToHighlight;
  s << m_bVisualizeColliders;
  s << m_bVisualizeJoints;
  s << m_bVisualizeSwingLimits;
  s << m_bVisualizeTwistLimits;
}

void plSkeletonComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  if (uiVersion <= 4)
    return;

  auto& s = inout_stream.GetStream();

  s >> m_hSkeleton;
  s >> m_bVisualizeBones;
  s >> m_sBonesToHighlight;
  s >> m_bVisualizeColliders;
  s >> m_bVisualizeJoints;
  s >> m_bVisualizeSwingLimits;
  s >> m_bVisualizeTwistLimits;
}

void plSkeletonComponent::OnActivated()
{
  SUPER::OnActivated();

  m_MaxBounds = plBoundingBox::MakeInvalid();
  VisualizeSkeletonDefaultState();
}

void plSkeletonComponent::SetSkeletonFile(const char* szFile)
{
  plSkeletonResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plSkeletonResource>(szFile);
  }

  SetSkeleton(hResource);
}

const char* plSkeletonComponent::GetSkeletonFile() const
{
  if (!m_hSkeleton.IsValid())
    return "";

  return m_hSkeleton.GetResourceID();
}


void plSkeletonComponent::SetSkeleton(const plSkeletonResourceHandle& hResource)
{
  if (m_hSkeleton != hResource)
  {
    m_hSkeleton = hResource;

    m_MaxBounds = plBoundingBox::MakeInvalid();
    VisualizeSkeletonDefaultState();
  }
}

void plSkeletonComponent::SetBonesToHighlight(const char* szFilter)
{
  if (m_sBonesToHighlight != szFilter)
  {
    m_sBonesToHighlight = szFilter;

    m_uiSkeletonChangeCounter = 0xFFFFFFFF;

    VisualizeSkeletonDefaultState();
  }
}

const char* plSkeletonComponent::GetBonesToHighlight() const
{
  return m_sBonesToHighlight;
}

void plSkeletonComponent::OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& msg)
{
  m_LinesSkeleton.Clear();
  m_SpheresShapes.Clear();
  m_BoxShapes.Clear();
  m_CapsuleShapes.Clear();
  m_AngleShapes.Clear();
  m_ConeLimitShapes.Clear();
  m_CylinderShapes.Clear();

  m_RootTransform = *msg.m_pRootTransform;

  BuildSkeletonVisualization(msg);
  BuildColliderVisualization(msg);
  BuildJointVisualization(msg);

  plBoundingBox poseBounds;
  poseBounds = plBoundingBox::MakeInvalid();

  for (const auto& bone : msg.m_ModelTransforms)
  {
    poseBounds.ExpandToInclude(bone.GetTranslationVector());
  }

  if (poseBounds.IsValid() && (!m_MaxBounds.IsValid() || !m_MaxBounds.Contains(poseBounds)))
  {
    m_MaxBounds.ExpandToInclude(poseBounds);
    TriggerLocalBoundsUpdate();
  }
  else if (((plRenderWorld::GetFrameCounter() + GetUniqueIdForRendering()) & (PLASMA_BIT(10) - 1)) == 0) // reset the bbox every once in a while
  {
    m_MaxBounds = poseBounds;
    TriggerLocalBoundsUpdate();
  }
}

void plSkeletonComponent::BuildSkeletonVisualization(plMsgAnimationPoseUpdated& msg)
{
  if (!m_bVisualizeBones || !msg.m_pSkeleton)
    return;

  plStringBuilder tmp;

  struct Bone
  {
    plVec3 pos = plVec3::MakeZero();
    plVec3 dir = plVec3::MakeZero();
    float distToParent = 0.0f;
    float minDistToChild = 10.0f;
    bool highlight = false;
  };

  plHybridArray<Bone, 128> bones;

  bones.SetCount(msg.m_pSkeleton->GetJointCount());
  m_LinesSkeleton.Reserve(m_LinesSkeleton.GetCount() + msg.m_pSkeleton->GetJointCount());

  const plVec3 vBoneDir = plBasisAxis::GetBasisVector(msg.m_pSkeleton->m_BoneDirection);

  auto renderBone = [&](int iCurrentBone, int iParentBone) {
    if (iParentBone == ozz::animation::Skeleton::kNoParent)
      return;

    const plVec3 v0 = *msg.m_pRootTransform * msg.m_ModelTransforms[iParentBone].GetTranslationVector();
    const plVec3 v1 = *msg.m_pRootTransform * msg.m_ModelTransforms[iCurrentBone].GetTranslationVector();

    plVec3 dirToBone = (v1 - v0);

    auto& bone = bones[iCurrentBone];
    bone.pos = v1;
    bone.distToParent = dirToBone.GetLength();
    bone.dir = *msg.m_pRootTransform * msg.m_ModelTransforms[iCurrentBone].TransformDirection(vBoneDir);
    bone.dir.NormalizeIfNotZero(plVec3::MakeZero()).IgnoreResult();

    auto& pb = bones[iParentBone];

    if (!pb.dir.IsZero() && dirToBone.NormalizeIfNotZero(plVec3::MakeZero()).Succeeded())
    {
      if (pb.dir.GetAngleBetween(dirToBone) < plAngle::MakeFromDegree(45))
      {
        plPlane plane;
        plane = plPlane::MakeFromNormalAndPoint(pb.dir, pb.pos);
        pb.minDistToChild = plMath::Min(pb.minDistToChild, plane.GetDistanceTo(v1));
      }
    }
  };

  ozz::animation::IterateJointsDF(msg.m_pSkeleton->GetOzzSkeleton(), renderBone);

  if (m_sBonesToHighlight == "*")
  {
    for (plUInt32 b = 0; b < bones.GetCount(); ++b)
    {
      bones[b].highlight = true;
    }
  }
  else if (!m_sBonesToHighlight.IsEmpty())
  {
    const plStringBuilder mask(";", m_sBonesToHighlight, ";");

    for (plUInt16 b = 0; b < static_cast<plUInt16>(bones.GetCount()); ++b)
    {
      const plString currentName = msg.m_pSkeleton->GetJointByIndex(b).GetName().GetString();

      tmp.Set(";", currentName, ";");

      if (mask.FindSubString(tmp))
      {
        bones[b].highlight = true;
      }
    }
  }

  for (plUInt32 b = 0; b < bones.GetCount(); ++b)
  {
    const auto& bone = bones[b];

    if (!bone.highlight)
    {
      float len = 0.3f;

      if (bone.minDistToChild < 10.0f)
      {
        len = bone.minDistToChild;
      }
      else if (bone.distToParent > 0)
      {
        len = plMath::Max(bone.distToParent * 0.5f, 0.1f);
      }
      else
      {
        len = 0.1f;
      }

      plVec3 v0 = bone.pos;
      plVec3 v1 = bone.pos + bone.dir * len;

      m_LinesSkeleton.PushBack(plDebugRenderer::Line(v0, v1));
      m_LinesSkeleton.PeekBack().m_startColor = plColor::DarkCyan;
      m_LinesSkeleton.PeekBack().m_endColor = plColor::DarkCyan;
    }
  }

  for (plUInt32 b = 0; b < bones.GetCount(); ++b)
  {
    const auto& bone = bones[b];

    if (bone.highlight && !bone.dir.IsZero(0.0001f))
    {
      float len = 0.3f;

      if (bone.minDistToChild < 10.0f)
      {
        len = bone.minDistToChild;
      }
      else if (bone.distToParent > 0)
      {
        len = plMath::Max(bone.distToParent * 0.5f, 0.1f);
      }
      else
      {
        len = 0.1f;
      }

      plVec3 v0 = bone.pos;
      plVec3 v1 = bone.pos + bone.dir * len;

      const plVec3 vO1 = bone.dir.GetOrthogonalVector().GetNormalized();
      const plVec3 vO2 = bone.dir.CrossRH(vO1).GetNormalized();

      plVec3 s[4];
      s[0] = v0 + vO1 * len * 0.1f + bone.dir * len * 0.1f;
      s[1] = v0 + vO2 * len * 0.1f + bone.dir * len * 0.1f;
      s[2] = v0 - vO1 * len * 0.1f + bone.dir * len * 0.1f;
      s[3] = v0 - vO2 * len * 0.1f + bone.dir * len * 0.1f;

      m_LinesSkeleton.PushBack(plDebugRenderer::Line(v0, v1));
      m_LinesSkeleton.PeekBack().m_startColor = plColor::DarkCyan;
      m_LinesSkeleton.PeekBack().m_endColor = plColor::DarkCyan;

      for (plUInt32 si = 0; si < 4; ++si)
      {
        m_LinesSkeleton.PushBack(plDebugRenderer::Line(v0, s[si]));
        m_LinesSkeleton.PeekBack().m_startColor = plColor::Chartreuse;
        m_LinesSkeleton.PeekBack().m_endColor = plColor::Chartreuse;

        m_LinesSkeleton.PushBack(plDebugRenderer::Line(s[si], v1));
        m_LinesSkeleton.PeekBack().m_startColor = plColor::Chartreuse;
        m_LinesSkeleton.PeekBack().m_endColor = plColor::Chartreuse;
      }
    }
  }
}

void plSkeletonComponent::BuildColliderVisualization(plMsgAnimationPoseUpdated& msg)
{
  if (!m_bVisualizeColliders || !msg.m_pSkeleton || !m_hSkeleton.IsValid())
    return;

  plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::BlockTillLoaded);

  const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  const plQuat qBoneDirAdjustment = plBasisAxis::GetBasisRotation(plBasisAxis::PositiveX, srcBoneDir);

  plStringBuilder bonesToHighlight(";", m_sBonesToHighlight, ";");
  plStringBuilder boneName;
  if (m_sBonesToHighlight == "*")
    bonesToHighlight.Clear();

  plQuat qRotZtoX; // the capsule should extend along X, but the debug renderer draws them along Z
  qRotZtoX = plQuat::MakeFromAxisAndAngle(plVec3(0, 1, 0), plAngle::MakeFromDegree(-90));

  for (const auto& geo : pSkeleton->GetDescriptor().m_Geometry)
  {
    if (geo.m_Type == plSkeletonJointGeometryType::None)
      continue;

    plMat4 boneTrans;
    plQuat boneRot;
    msg.ComputeFullBoneTransform(geo.m_uiAttachedToJoint, boneTrans, boneRot);

    boneName.Set(";", msg.m_pSkeleton->GetJointByIndex(geo.m_uiAttachedToJoint).GetName().GetString(), ";");
    const bool bHighlight = bonesToHighlight.IsEmpty() || bonesToHighlight.FindLastSubString(boneName) != nullptr;
    const plColor hlS = plMath::Lerp(plColor::DimGrey, plColor::Yellow, bHighlight ? 1.0f : 0.2f);

    const plQuat qFinalBoneRot = boneRot * qBoneDirAdjustment;

    plTransform st;
    st.SetIdentity();
    st.m_vPosition = boneTrans.GetTranslationVector() + qFinalBoneRot * geo.m_Transform.m_vPosition;
    st.m_qRotation = qFinalBoneRot * geo.m_Transform.m_qRotation;

    if (geo.m_Type == plSkeletonJointGeometryType::Sphere)
    {
      auto& shape = m_SpheresShapes.ExpandAndGetRef();
      shape.m_Transform = st;
      shape.m_Shape = plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), geo.m_Transform.m_vScale.z);
      shape.m_Color = hlS;
    }

    if (geo.m_Type == plSkeletonJointGeometryType::Box)
    {
      auto& shape = m_BoxShapes.ExpandAndGetRef();

      plVec3 ext;
      ext.x = geo.m_Transform.m_vScale.x * 0.5f;
      ext.y = geo.m_Transform.m_vScale.y * 0.5f;
      ext.z = geo.m_Transform.m_vScale.z * 0.5f;

      // TODO: if offset desired
      st.m_vPosition += qFinalBoneRot * plVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      shape.m_Transform = st;
      shape.m_Shape = plBoundingBox::MakeFromCenterAndHalfExtents(plVec3::MakeZero(), ext);
      shape.m_Color = hlS;
    }

    if (geo.m_Type == plSkeletonJointGeometryType::Capsule)
    {
      st.m_qRotation = st.m_qRotation * qRotZtoX;

      // TODO: if offset desired
      st.m_vPosition += qFinalBoneRot * plVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      auto& shape = m_CapsuleShapes.ExpandAndGetRef();
      shape.m_Transform = st;
      shape.m_fLength = geo.m_Transform.m_vScale.x;
      shape.m_fRadius = geo.m_Transform.m_vScale.z;
      shape.m_Color = hlS;
    }

    if (geo.m_Type == plSkeletonJointGeometryType::ConvexMesh)
    {
      st.SetIdentity();
      st = *msg.m_pRootTransform;

      for (plUInt32 f = 0; f < geo.m_TriangleIndices.GetCount(); f += 3)
      {
        const plUInt32 i0 = geo.m_TriangleIndices[f + 0];
        const plUInt32 i1 = geo.m_TriangleIndices[f + 1];
        const plUInt32 i2 = geo.m_TriangleIndices[f + 2];

        {
          auto& l = m_LinesSkeleton.ExpandAndGetRef();
          l.m_startColor = l.m_endColor = hlS;
          l.m_start = st * geo.m_VertexPositions[i0];
          l.m_end = st * geo.m_VertexPositions[i1];
        }
        {
          auto& l = m_LinesSkeleton.ExpandAndGetRef();
          l.m_startColor = l.m_endColor = hlS;
          l.m_start = st * geo.m_VertexPositions[i1];
          l.m_end = st * geo.m_VertexPositions[i2];
        }
        {
          auto& l = m_LinesSkeleton.ExpandAndGetRef();
          l.m_startColor = l.m_endColor = hlS;
          l.m_start = st * geo.m_VertexPositions[i2];
          l.m_end = st * geo.m_VertexPositions[i0];
        }
      }
    }
  }
}

void plSkeletonComponent::BuildJointVisualization(plMsgAnimationPoseUpdated& msg)
{
  if (!m_hSkeleton.IsValid() || (!m_bVisualizeJoints && !m_bVisualizeSwingLimits && !m_bVisualizeTwistLimits))
    return;

  plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::BlockTillLoaded);
  const auto& skel = pSkeleton->GetDescriptor().m_Skeleton;

  plStringBuilder bonesToHighlight(";", m_sBonesToHighlight, ";");
  plStringBuilder boneName;
  if (m_sBonesToHighlight == "*")
    bonesToHighlight.Clear();

  const plQuat qBoneDir = plBasisAxis::GetBasisRotation_PosX(pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const plQuat qBoneDirT = plBasisAxis::GetBasisRotation(plBasisAxis::PositiveY, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const plQuat qBoneDirBT = plBasisAxis::GetBasisRotation(plBasisAxis::PositiveZ, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const plQuat qBoneDirT2 = plBasisAxis::GetBasisRotation(plBasisAxis::NegativeY, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);

  for (plUInt16 uiJointIdx = 0; uiJointIdx < skel.GetJointCount(); ++uiJointIdx)
  {
    const auto& thisJoint = skel.GetJointByIndex(uiJointIdx);
    const plUInt16 uiParentIdx = thisJoint.GetParentIndex();

    if (thisJoint.IsRootJoint())
      continue;

    boneName.Set(";", thisJoint.GetName().GetString(), ";");

    const bool bHighlight = bonesToHighlight.IsEmpty() || bonesToHighlight.FindSubString(boneName) != nullptr;

    plMat4 parentTrans;
    plQuat parentRot; // contains root transform
    msg.ComputeFullBoneTransform(uiParentIdx, parentTrans, parentRot);

    plMat4 thisTrans; // contains root transform
    plQuat thisRot;   // contains root transform
    msg.ComputeFullBoneTransform(uiJointIdx, thisTrans, thisRot);

    const plVec3 vJointPos = thisTrans.GetTranslationVector();
    const plQuat qLimitRot = parentRot * thisJoint.GetLocalOrientation();

    // main directions
    if (m_bVisualizeJoints)
    {
      const plColor hlM = plMath::Lerp(plColor::OrangeRed, plColor::DimGrey, bHighlight ? 0 : 0.8f);
      const plColor hlT = plMath::Lerp(plColor::LawnGreen, plColor::DimGrey, bHighlight ? 0 : 0.8f);
      const plColor hlBT = plMath::Lerp(plColor::BlueViolet, plColor::DimGrey, bHighlight ? 0 : 0.8f);

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlM;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDir;
        cyl.m_Transform.m_vScale.Set(1);
      }

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlT;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirT;
        cyl.m_Transform.m_vScale.Set(1);
      }

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlBT;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirBT;
        cyl.m_Transform.m_vScale.Set(1);
      }
    }

    // swing limit
    if (m_bVisualizeSwingLimits && (thisJoint.GetHalfSwingLimitY() > plAngle() || thisJoint.GetHalfSwingLimitZ() > plAngle()))
    {
      auto& shape = m_ConeLimitShapes.ExpandAndGetRef();
      shape.m_Angle1 = thisJoint.GetHalfSwingLimitY();
      shape.m_Angle2 = thisJoint.GetHalfSwingLimitZ();
      shape.m_Color = plMath::Lerp(plColor::DimGrey, plColor::DeepPink, bHighlight ? 1.0f : 0.2f);
      shape.m_Transform.m_vScale.Set(0.05f);
      shape.m_Transform.m_vPosition = vJointPos;
      shape.m_Transform.m_qRotation = qLimitRot * qBoneDir;

      const plColor hlM = plMath::Lerp(plColor::OrangeRed, plColor::DimGrey, bHighlight ? 0 : 0.8f);

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlM;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDir;
        cyl.m_Transform.m_vScale.Set(1);
      }
    }

    // twist limit
    if (m_bVisualizeTwistLimits && thisJoint.GetTwistLimitHalfAngle() > plAngle::MakeFromDegree(0))
    {
      auto& shape = m_AngleShapes.ExpandAndGetRef();
      shape.m_StartAngle = thisJoint.GetTwistLimitLow();
      shape.m_EndAngle = thisJoint.GetTwistLimitHigh();
      shape.m_Color = plMath::Lerp(plColor::DimGrey, plColor::LightPink, bHighlight ? 0.8f : 0.2f);
      shape.m_Transform.m_vScale.Set(0.04f);
      shape.m_Transform.m_vPosition = vJointPos;
      shape.m_Transform.m_qRotation = qLimitRot;

      const plColor hlT = plMath::Lerp(plColor::DimGrey, plColor::LightPink, bHighlight ? 1.0f : 0.4f);

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlT;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirT2;
        cyl.m_Transform.m_vScale.Set(1);

        plVec3 vDir = cyl.m_Transform.m_qRotation * plVec3(1, 0, 0);
        vDir.Normalize();

        plVec3 vDirRef = shape.m_Transform.m_qRotation * qBoneDir * plVec3(0, 1, 0);
        vDirRef.Normalize();

        const plVec3 vRotDir = shape.m_Transform.m_qRotation * qBoneDir * plVec3(1, 0, 0);
        plQuat qRotRef = plQuat::MakeFromAxisAndAngle(vRotDir, thisJoint.GetTwistLimitCenterAngle());
        vDirRef = qRotRef * vDirRef;

        // if the current twist is outside the twist limit range, highlight the bone
        if (vDir.GetAngleBetween(vDirRef) > thisJoint.GetTwistLimitHalfAngle())
        {
          cyl.m_Color = plColor::Orange;
        }
      }
    }
  }
}

void plSkeletonComponent::VisualizeSkeletonDefaultState()
{
  if (!IsActiveAndInitialized())
    return;

  m_uiSkeletonChangeCounter = 0;

  if (m_hSkeleton.IsValid())
  {
    plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pSkeleton.GetAcquireResult() == plResourceAcquireResult::Final)
    {
      m_uiSkeletonChangeCounter = pSkeleton->GetCurrentResourceChangeCounter();

      if (pSkeleton->GetDescriptor().m_Skeleton.GetJointCount() > 0)
      {
        ozz::vector<ozz::math::Float4x4> modelTransforms;
        modelTransforms.resize(pSkeleton->GetDescriptor().m_Skeleton.GetJointCount());

        {
          ozz::animation::LocalToModelJob job;
          job.input = pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_rest_poses();
          job.output = make_span(modelTransforms);
          job.skeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
          job.Run();
        }

        plMsgAnimationPoseUpdated msg;
        msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
        msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
        msg.m_ModelTransforms = plArrayPtr<const plMat4>(reinterpret_cast<const plMat4*>(&modelTransforms[0]), (plUInt32)modelTransforms.size());

        OnAnimationPoseUpdated(msg);
      }
    }
  }

  TriggerLocalBoundsUpdate();
}

plDebugRenderer::Line& plSkeletonComponent::AddLine(const plVec3& vStart, const plVec3& vEnd, const plColor& color)
{
  auto& line = m_LinesSkeleton.ExpandAndGetRef();
  line.m_start = vStart;
  line.m_end = vEnd;
  line.m_startColor = color;
  line.m_endColor = color;
  return line;
}

void plSkeletonComponent::OnQueryAnimationSkeleton(plMsgQueryAnimationSkeleton& msg)
{
  // if we have a skeleton, always overwrite it any incoming message with that
  if (m_hSkeleton.IsValid())
  {
    msg.m_hSkeleton = m_hSkeleton;
  }
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonComponent);

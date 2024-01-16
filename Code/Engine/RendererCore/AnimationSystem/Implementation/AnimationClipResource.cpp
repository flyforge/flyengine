#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/animation_optimizer.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimationClipResource, 1, plRTTIDefaultAllocator<plAnimationClipResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plAnimationClipResource);
// clang-format on

plAnimationClipResource::plAnimationClipResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plAnimationClipResource, plAnimationClipResourceDescriptor)
{
  m_pDescriptor = PLASMA_DEFAULT_NEW(plAnimationClipResourceDescriptor);
  *m_pDescriptor = std::move(descriptor);

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}

plResourceLoadDesc plAnimationClipResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

plResourceLoadDesc plAnimationClipResource::UpdateContent(plStreamReader* Stream)
{
  PLASMA_LOG_BLOCK("plAnimationClipResource::UpdateContent", GetResourceIdOrDescription());

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    plStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  plAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_pDescriptor = PLASMA_DEFAULT_NEW(plAnimationClipResourceDescriptor);
  m_pDescriptor->Deserialize(*Stream).IgnoreResult();

  res.m_State = plResourceState::Loaded;
  return res;
}

void plAnimationClipResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plAnimationClipResource);

  if (m_pDescriptor)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += static_cast<plUInt32>(m_pDescriptor->GetHeapMemoryUsage());
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct plAnimationClipResourceDescriptor::OzzImpl
{
  struct CachedAnim
  {
    plUInt32 m_uiResourceChangeCounter = 0;
    ozz::unique_ptr<ozz::animation::Animation> m_pAnim;
  };

  plMap<const plSkeletonResource*, CachedAnim> m_MappedOzzAnimations;
};

plAnimationClipResourceDescriptor::plAnimationClipResourceDescriptor()
{
  m_pOzzImpl = PLASMA_DEFAULT_NEW(OzzImpl);
}

plAnimationClipResourceDescriptor::plAnimationClipResourceDescriptor(plAnimationClipResourceDescriptor&& rhs)
{
  *this = std::move(rhs);
}

plAnimationClipResourceDescriptor::~plAnimationClipResourceDescriptor() = default;

void plAnimationClipResourceDescriptor::operator=(plAnimationClipResourceDescriptor&& rhs) noexcept
{
  m_pOzzImpl = std::move(rhs.m_pOzzImpl);

  m_JointInfos = std::move(rhs.m_JointInfos);
  m_Transforms = std::move(rhs.m_Transforms);
  m_uiNumTotalPositions = rhs.m_uiNumTotalPositions;
  m_uiNumTotalRotations = rhs.m_uiNumTotalRotations;
  m_uiNumTotalScales = rhs.m_uiNumTotalScales;
  m_Duration = rhs.m_Duration;
}

plResult plAnimationClipResourceDescriptor::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(9);

  const plUInt16 uiNumJoints = static_cast<plUInt16>(m_JointInfos.GetCount());
  inout_stream << uiNumJoints;
  for (plUInt32 i = 0; i < m_JointInfos.GetCount(); ++i)
  {
    const auto& val = m_JointInfos.GetValue(i);

    inout_stream << m_JointInfos.GetKey(i);
    inout_stream << val.m_uiPositionIdx;
    inout_stream << val.m_uiPositionCount;
    inout_stream << val.m_uiRotationIdx;
    inout_stream << val.m_uiRotationCount;
    inout_stream << val.m_uiScaleIdx;
    inout_stream << val.m_uiScaleCount;
  }

  inout_stream << m_Duration;
  inout_stream << m_uiNumTotalPositions;
  inout_stream << m_uiNumTotalRotations;
  inout_stream << m_uiNumTotalScales;

  PLASMA_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Transforms));

  inout_stream << m_vConstantRootMotion;

  m_EventTrack.Save(inout_stream);

  inout_stream << m_bAdditive;

  return PLASMA_SUCCESS;
}

plResult plAnimationClipResourceDescriptor::Deserialize(plStreamReader& inout_stream)
{
  const plTypeVersion uiVersion = inout_stream.ReadVersion(9);

  if (uiVersion < 6)
    return PLASMA_FAILURE;

  plUInt16 uiNumJoints = 0;
  inout_stream >> uiNumJoints;

  m_JointInfos.Reserve(uiNumJoints);

  plHashedString hs;

  for (plUInt16 i = 0; i < uiNumJoints; ++i)
  {
    inout_stream >> hs;

    JointInfo ji;
    inout_stream >> ji.m_uiPositionIdx;
    inout_stream >> ji.m_uiPositionCount;
    inout_stream >> ji.m_uiRotationIdx;
    inout_stream >> ji.m_uiRotationCount;
    inout_stream >> ji.m_uiScaleIdx;
    inout_stream >> ji.m_uiScaleCount;

    m_JointInfos.Insert(hs, ji);
  }

  m_JointInfos.Sort();

  inout_stream >> m_Duration;
  inout_stream >> m_uiNumTotalPositions;
  inout_stream >> m_uiNumTotalRotations;
  inout_stream >> m_uiNumTotalScales;

  PLASMA_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Transforms));

  if (uiVersion >= 7)
  {
    inout_stream >> m_vConstantRootMotion;
  }

  if (uiVersion >= 8)
  {
    m_EventTrack.Load(inout_stream);
  }

  if (uiVersion >= 9)
  {
    inout_stream >> m_bAdditive;
  }

  return PLASMA_SUCCESS;
}

plUInt64 plAnimationClipResourceDescriptor::GetHeapMemoryUsage() const
{
  return m_Transforms.GetHeapMemoryUsage() + m_JointInfos.GetHeapMemoryUsage() + m_pOzzImpl->m_MappedOzzAnimations.GetHeapMemoryUsage();
}

plUInt16 plAnimationClipResourceDescriptor::GetNumJoints() const
{
  return static_cast<plUInt16>(m_JointInfos.GetCount());
}

plTime plAnimationClipResourceDescriptor::GetDuration() const
{
  return m_Duration;
}

void plAnimationClipResourceDescriptor::SetDuration(plTime duration)
{
  m_Duration = duration;
}

PLASMA_FORCE_INLINE void pl2ozz(const plVec3& vIn, ozz::math::Float3& ref_out)
{
  ref_out.x = vIn.x;
  ref_out.y = vIn.y;
  ref_out.z = vIn.z;
}

PLASMA_FORCE_INLINE void pl2ozz(const plQuat& qIn, ozz::math::Quaternion& ref_out)
{
  ref_out.x = qIn.v.x;
  ref_out.y = qIn.v.y;
  ref_out.z = qIn.v.z;
  ref_out.w = qIn.w;
}

const ozz::animation::Animation& plAnimationClipResourceDescriptor::GetMappedOzzAnimation(const plSkeletonResource& skeleton) const
{
  auto it = m_pOzzImpl->m_MappedOzzAnimations.Find(&skeleton);
  if (it.IsValid())
  {
    if (it.Value().m_uiResourceChangeCounter == skeleton.GetCurrentResourceChangeCounter())
    {
      return *it.Value().m_pAnim.get();
    }
  }

  auto pOzzSkeleton = &skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton();
  const plUInt32 uiNumJoints = pOzzSkeleton->num_joints();

  ozz::animation::offline::RawAnimation rawAnim;
  rawAnim.duration = plMath::Max(1.0f / 60.0f, m_Duration.AsFloatInSeconds());
  rawAnim.tracks.resize(uiNumJoints);

  for (plUInt32 j = 0; j < uiNumJoints; ++j)
  {
    auto& dstTrack = rawAnim.tracks[j];

    const plTempHashedString sJointName = plTempHashedString(pOzzSkeleton->joint_names()[j]);

    const JointInfo* pJointInfo = GetJointInfo(sJointName);

    if (pJointInfo == nullptr)
    {
      dstTrack.translations.resize(1);
      dstTrack.rotations.resize(1);
      dstTrack.scales.resize(1);

      const plUInt16 uiFallbackIdx = skeleton.GetDescriptor().m_Skeleton.FindJointByName(sJointName);

      PLASMA_ASSERT_DEV(uiFallbackIdx != plInvalidJointIndex, "");

      const auto& fallbackJoint = skeleton.GetDescriptor().m_Skeleton.GetJointByIndex(uiFallbackIdx);

      const plTransform& fallbackTransform = fallbackJoint.GetRestPoseLocalTransform();

      auto& dstT = dstTrack.translations[0];
      auto& dstR = dstTrack.rotations[0];
      auto& dstS = dstTrack.scales[0];

      dstT.time = 0.0f;
      dstR.time = 0.0f;
      dstS.time = 0.0f;

      pl2ozz(fallbackTransform.m_vPosition, dstT.value);
      pl2ozz(fallbackTransform.m_qRotation, dstR.value);
      pl2ozz(fallbackTransform.m_vScale, dstS.value);
    }
    else
    {
      // positions
      {
        dstTrack.translations.resize(pJointInfo->m_uiPositionCount);
        const plArrayPtr<const KeyframeVec3> keyframes = GetPositionKeyframes(*pJointInfo);

        for (plUInt32 i = 0; i < pJointInfo->m_uiPositionCount; ++i)
        {
          auto& dst = dstTrack.translations[i];

          dst.time = keyframes[i].m_fTimeInSec;
          pl2ozz(keyframes[i].m_Value, dst.value);
        }
      }

      // rotations
      {
        dstTrack.rotations.resize(pJointInfo->m_uiRotationCount);
        const plArrayPtr<const KeyframeQuat> keyframes = GetRotationKeyframes(*pJointInfo);

        for (plUInt32 i = 0; i < pJointInfo->m_uiRotationCount; ++i)
        {
          auto& dst = dstTrack.rotations[i];

          dst.time = keyframes[i].m_fTimeInSec;
          pl2ozz(keyframes[i].m_Value, dst.value);
        }
      }

      // scales
      {
        dstTrack.scales.resize(pJointInfo->m_uiScaleCount);
        const plArrayPtr<const KeyframeVec3> keyframes = GetScaleKeyframes(*pJointInfo);

        for (plUInt32 i = 0; i < pJointInfo->m_uiScaleCount; ++i)
        {
          auto& dst = dstTrack.scales[i];

          dst.time = keyframes[i].m_fTimeInSec;
          pl2ozz(keyframes[i].m_Value, dst.value);
        }
      }
    }
  }

  ozz::animation::offline::AnimationBuilder animBuilder;

  PLASMA_ASSERT_DEBUG(rawAnim.Validate(), "Invalid animation data");

  auto& cached = m_pOzzImpl->m_MappedOzzAnimations[&skeleton];
  cached.m_pAnim = std::move(animBuilder(rawAnim));
  cached.m_uiResourceChangeCounter = skeleton.GetCurrentResourceChangeCounter();

  return *cached.m_pAnim.get();
}

plAnimationClipResourceDescriptor::JointInfo plAnimationClipResourceDescriptor::CreateJoint(const plHashedString& sJointName, plUInt16 uiNumPositions, plUInt16 uiNumRotations, plUInt16 uiNumScales)
{
  JointInfo ji;
  ji.m_uiPositionIdx = m_uiNumTotalPositions;
  ji.m_uiRotationIdx = m_uiNumTotalRotations;
  ji.m_uiScaleIdx = m_uiNumTotalScales;

  ji.m_uiPositionCount = uiNumPositions;
  ji.m_uiRotationCount = uiNumRotations;
  ji.m_uiScaleCount = uiNumScales;

  m_uiNumTotalPositions += uiNumPositions;
  m_uiNumTotalRotations += uiNumRotations;
  m_uiNumTotalScales += uiNumScales;

  m_JointInfos.Insert(sJointName, ji);

  return ji;
}

const plAnimationClipResourceDescriptor::JointInfo* plAnimationClipResourceDescriptor::GetJointInfo(const plTempHashedString& sJointName) const
{
  plUInt32 uiIndex = m_JointInfos.Find(sJointName);

  if (uiIndex == plInvalidIndex)
    return nullptr;

  return &m_JointInfos.GetValue(uiIndex);
}

void plAnimationClipResourceDescriptor::AllocateJointTransforms()
{
  const plUInt32 uiNumBytes = m_uiNumTotalPositions * sizeof(KeyframeVec3) + m_uiNumTotalRotations * sizeof(KeyframeQuat) + m_uiNumTotalScales * sizeof(KeyframeVec3);

  m_Transforms.SetCountUninitialized(uiNumBytes);
}

plArrayPtr<plAnimationClipResourceDescriptor::KeyframeVec3> plAnimationClipResourceDescriptor::GetPositionKeyframes(const JointInfo& jointInfo)
{
  PLASMA_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  plUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiPositionIdx;

  return plArrayPtr<KeyframeVec3>(reinterpret_cast<KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiPositionCount);
}

plArrayPtr<plAnimationClipResourceDescriptor::KeyframeQuat> plAnimationClipResourceDescriptor::GetRotationKeyframes(const JointInfo& jointInfo)
{
  PLASMA_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  plUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * jointInfo.m_uiRotationIdx;

  return plArrayPtr<KeyframeQuat>(reinterpret_cast<KeyframeQuat*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiRotationCount);
}

plArrayPtr<plAnimationClipResourceDescriptor::KeyframeVec3> plAnimationClipResourceDescriptor::GetScaleKeyframes(const JointInfo& jointInfo)
{
  PLASMA_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  plUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * m_uiNumTotalRotations;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiScaleIdx;

  return plArrayPtr<KeyframeVec3>(reinterpret_cast<KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiScaleCount);
}

plArrayPtr<const plAnimationClipResourceDescriptor::KeyframeVec3> plAnimationClipResourceDescriptor::GetPositionKeyframes(const JointInfo& jointInfo) const
{
  plUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiPositionIdx;

  return plArrayPtr<const KeyframeVec3>(reinterpret_cast<const KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiPositionCount);
}

plArrayPtr<const plAnimationClipResourceDescriptor::KeyframeQuat> plAnimationClipResourceDescriptor::GetRotationKeyframes(const JointInfo& jointInfo) const
{
  plUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * jointInfo.m_uiRotationIdx;

  return plArrayPtr<const KeyframeQuat>(reinterpret_cast<const KeyframeQuat*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiRotationCount);
}

plArrayPtr<const plAnimationClipResourceDescriptor::KeyframeVec3> plAnimationClipResourceDescriptor::GetScaleKeyframes(const JointInfo& jointInfo) const
{
  plUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * m_uiNumTotalRotations;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiScaleIdx;

  return plArrayPtr<const KeyframeVec3>(reinterpret_cast<const KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiScaleCount);
}

// bool plAnimationClipResourceDescriptor::HasRootMotion() const
//{
//  return m_JointNameToIndex.Contains(plTempHashedString("plRootMotionTransform"));
//}
//
// plUInt16 plAnimationClipResourceDescriptor::GetRootMotionJoint() const
//{
//  plUInt16 jointIdx = 0;
//
//#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
//
//  const plUInt32 idx = m_JointNameToIndex.Find(plTempHashedString("plRootMotionTransform"));
//  PLASMA_ASSERT_DEBUG(idx != plInvalidIndex, "Animation Clip has no root motion transforms");
//
//  jointIdx = m_JointNameToIndex.GetValue(idx);
//  PLASMA_ASSERT_DEBUG(jointIdx == 0, "The root motion joint should always be at index 0");
//#endif
//
//  return jointIdx;
//}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_AnimationClipResource);

#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Tracks/EventTrack.h>

class plSkeletonResource;

namespace ozz::animation
{
  class Animation;
}

struct PLASMA_RENDERERCORE_DLL plAnimationClipResourceDescriptor
{
public:
  plAnimationClipResourceDescriptor();
  plAnimationClipResourceDescriptor(plAnimationClipResourceDescriptor&& rhs);
  ~plAnimationClipResourceDescriptor();

  void operator=(plAnimationClipResourceDescriptor&& rhs) noexcept;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);

  plUInt64 GetHeapMemoryUsage() const;

  plUInt16 GetNumJoints() const;
  plTime GetDuration() const;
  void SetDuration(plTime duration);

  const ozz::animation::Animation& GetMappedOzzAnimation(const plSkeletonResource& skeleton) const;

  struct JointInfo
  {
    plUInt32 m_uiPositionIdx = 0;
    plUInt32 m_uiRotationIdx = 0;
    plUInt32 m_uiScaleIdx = 0;
    plUInt16 m_uiPositionCount = 0;
    plUInt16 m_uiRotationCount = 0;
    plUInt16 m_uiScaleCount = 0;
  };

  struct KeyframeVec3
  {
    float m_fTimeInSec;
    plVec3 m_Value;
  };

  struct KeyframeQuat
  {
    float m_fTimeInSec;
    plQuat m_Value;
  };

  JointInfo CreateJoint(const plHashedString& sJointName, plUInt16 uiNumPositions, plUInt16 uiNumRotations, plUInt16 uiNumScales);
  const JointInfo* GetJointInfo(const plTempHashedString& sJointName) const;
  void AllocateJointTransforms();

  plArrayPtr<KeyframeVec3> GetPositionKeyframes(const JointInfo& jointInfo);
  plArrayPtr<KeyframeQuat> GetRotationKeyframes(const JointInfo& jointInfo);
  plArrayPtr<KeyframeVec3> GetScaleKeyframes(const JointInfo& jointInfo);

  plArrayPtr<const KeyframeVec3> GetPositionKeyframes(const JointInfo& jointInfo) const;
  plArrayPtr<const KeyframeQuat> GetRotationKeyframes(const JointInfo& jointInfo) const;
  plArrayPtr<const KeyframeVec3> GetScaleKeyframes(const JointInfo& jointInfo) const;

  plVec3 m_vConstantRootMotion = plVec3::MakeZero();

  plEventTrack m_EventTrack;

  bool m_bAdditive = false;

private:
  plArrayMap<plHashedString, JointInfo> m_JointInfos;
  plDataBuffer m_Transforms;
  plUInt32 m_uiNumTotalPositions = 0;
  plUInt32 m_uiNumTotalRotations = 0;
  plUInt32 m_uiNumTotalScales = 0;
  plTime m_Duration;

  struct OzzImpl;
  plUniquePtr<OzzImpl> m_pOzzImpl;
};

//////////////////////////////////////////////////////////////////////////

using plAnimationClipResourceHandle = plTypedResourceHandle<class plAnimationClipResource>;

class PLASMA_RENDERERCORE_DLL plAnimationClipResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAnimationClipResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plAnimationClipResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plAnimationClipResource, plAnimationClipResourceDescriptor);

public:
  plAnimationClipResource();

  const plAnimationClipResourceDescriptor& GetDescriptor() const { return *m_pDescriptor; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plUniquePtr<plAnimationClipResourceDescriptor> m_pDescriptor;
};

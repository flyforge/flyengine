#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>

class plEventTrack;
struct plMsgGenericEvent;

using plAnimationClipResourceHandle = plTypedResourceHandle<class plAnimationClipResource>;
using plSkeletonResourceHandle = plTypedResourceHandle<class plSkeletonResource>;

using plSimpleAnimationComponentManager = plComponentManagerSimple<class plSimpleAnimationComponent, plComponentUpdateType::WhenSimulating, plBlockStorageType::FreeList>;

/// \brief Plays a single animation clip on an animated mesh.
///
/// \see plAnimatedMeshComponent
class PL_GAMEENGINE_DLL plSimpleAnimationComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plSimpleAnimationComponent, plComponent, plSimpleAnimationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plJointAttachmentComponent

public:
  plSimpleAnimationComponent();
  ~plSimpleAnimationComponent();

  void SetAnimationClip(const plAnimationClipResourceHandle& hResource);
  const plAnimationClipResourceHandle& GetAnimationClip() const;

  void SetAnimationClipFile(const char* szFile); // [ property ]
  const char* GetAnimationClipFile() const;      // [ property ]

  /// \brief How to play the animation.
  plEnum<plPropertyAnimMode> m_AnimationMode; // [ property ]

  /// \brief How quickly or slowly to play the animation.
  float m_fSpeed = 1.0f; // [ property ]

  /// \brief Sets the current sample position of the animation clip in 0 (start) to 1 (end) range.
  void SetNormalizedPlaybackPosition(float fPosition);

  /// \brief Returns the normalized [0;1] sample position of the animation clip.
  float GetNormalizedPlaybackPosition() const { return m_fNormalizedPlaybackPosition; }

  /// \brief How often to update the animation while the animated mesh is invisible.
  plEnum<plAnimationInvisibleUpdateRate> m_InvisibleUpdateRate; // [ property ]

protected:
  void Update();
  bool UpdatePlaybackTime(plTime tDiff, const plEventTrack& eventTrack, plAnimPoseEventTrackSampleMode& out_trackSampling);

  plEnum<plRootMotionMode> m_RootMotionMode;
  float m_fNormalizedPlaybackPosition = 0.0f;
  plTime m_Duration;
  plAnimationClipResourceHandle m_hAnimationClip;
  plSkeletonResourceHandle m_hSkeleton;
  plTime m_ElapsedTimeSinceUpdate = plTime::MakeZero();

  ozz::vector<ozz::math::SoaTransform> m_OzzLocalTransforms; // TODO: could be frame allocated
};

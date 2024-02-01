#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using plJointAttachmentComponentManager = plComponentManager<class plJointAttachmentComponent, plBlockStorageType::FreeList>;

/// \brief Used to expose an animated mesh's bone as a game object, such that objects can be attached to it to move along.
///
/// The animation system deals with bone animations internally.
/// Sometimes it is desirable to move certain objects along with a bone,
/// for example when a character should hold something in their hand.
///
/// This component references a bone by name, and takes care to position the owner object at the same location as the bone
/// whenever the animation pose changes.
/// Thus it is possible to attach other objects as child objects to this one, so that they move along as well.
class PL_GAMEENGINE_DLL plJointAttachmentComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plJointAttachmentComponent, plComponent, plJointAttachmentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plJointAttachmentComponent

public:
  plJointAttachmentComponent();
  ~plJointAttachmentComponent();

  /// \brief Sets the bone name whose transform should be copied into this game object.
  void SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;      // [ property ]

  /// \brief An additional local offset to be added to the transform.
  plVec3 m_vLocalPositionOffset = plVec3::MakeZero(); // [ property ]

  /// \brief An additional local offset to be added to the transform.
  plQuat m_vLocalRotationOffset = plQuat::MakeIdentity(); // [ property ]

protected:
  void OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& msg); // [ msg handler ]

  plHashedString m_sJointToAttachTo;
  plUInt16 m_uiJointIndex = plInvalidJointIndex;
};

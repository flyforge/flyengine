#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using plJointOverrideComponentManager = plComponentManager<class plJointOverrideComponent, plBlockStorageType::FreeList>;

/// \brief Overrides the local transform of a bone in a skeletal animation.
///
/// Every time a new animation pose is prepared, this component replaces the transform of the chosen bone
/// to be the same as the local transform of the owner game object.
///
/// That allows you to do a simple kind of forward kinematics. For example it can be used to modify a targeting bone,
/// so that an animated object points into the right direction.
///
/// The global transform of the game object is irrelevant, but the local transform is used to copy over.
class PL_GAMEENGINE_DLL plJointOverrideComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plJointOverrideComponent, plComponent, plJointOverrideComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plJointOverrideComponent

public:
  plJointOverrideComponent();
  ~plJointOverrideComponent();

  /// \brief The name of the bone whose transform should be replaced with the transform of this game object.
  void SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;      // [ property ]

  /// \brief If true, the position of the bone will be overridden.
  bool m_bOverridePosition = false; // [ property ]

  /// \brief If true, the rotation of the bone will be overridden.
  bool m_bOverrideRotation = true; // [ property ]

  /// \brief If true, the scale of the bone will be overridden.
  bool m_bOverrideScale = false; // [ property ]

protected:
  void OnAnimationPosePreparing(plMsgAnimationPosePreparing& msg) const; // [ msg handler ]

  plHashedString m_sJointToOverride;
  mutable plUInt16 m_uiJointIndex = plInvalidJointIndex;
};

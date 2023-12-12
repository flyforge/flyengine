#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using plJointAttachmentComponentManager = plComponentManager<class plJointAttachmentComponent, plBlockStorageType::FreeList>;

class PLASMA_GAMEENGINE_DLL plJointAttachmentComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJointAttachmentComponent, plComponent, plJointAttachmentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plJointAttachmentComponent

public:
  plJointAttachmentComponent();
  ~plJointAttachmentComponent();

  void SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;      // [ property ]

  plVec3 m_vLocalPositionOffset = plVec3::ZeroVector();         // [ property ]
  plQuat m_vLocalRotationOffset = plQuat::IdentityQuaternion(); // [ property ]

protected:
  void OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& msg); // [ msg handler ]

  plHashedString m_sJointToAttachTo;
  plUInt16 m_uiJointIndex = plInvalidJointIndex;
};

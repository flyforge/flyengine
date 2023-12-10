#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>

using plJointOverrideComponentManager = plComponentManager<class plJointOverrideComponent, plBlockStorageType::FreeList>;

class PLASMA_GAMEENGINE_DLL plJointOverrideComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJointOverrideComponent, plComponent, plJointOverrideComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plJointOverrideComponent

public:
  plJointOverrideComponent();
  ~plJointOverrideComponent();

  void SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;      // [ property ]

  bool m_bOverridePosition = false; // [ property ]
  bool m_bOverrideRotation = true;  // [ property ]
  bool m_bOverrideScale = false;    // [ property ]

protected:
  void OnAnimationPosePreparing(plMsgAnimationPosePreparing& msg) const; // [ msg handler ]

  plHashedString m_sJointToOverride;
  mutable plUInt16 m_uiJointIndex = plInvalidJointIndex;
};

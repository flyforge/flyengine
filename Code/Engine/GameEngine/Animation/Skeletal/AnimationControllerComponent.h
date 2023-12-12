#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>

using plSkeletonResourceHandle = plTypedResourceHandle<class plSkeletonResource>;
using plAnimGraphResourceHandle = plTypedResourceHandle<class plAnimGraphResource>;

using plAnimationControllerComponentManager = plComponentManagerSimple<class plAnimationControllerComponent, plComponentUpdateType::WhenSimulating, plBlockStorageType::FreeList>;

class PLASMA_GAMEENGINE_DLL plAnimationControllerComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plAnimationControllerComponent, plComponent, plAnimationControllerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

  public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plAnimationControllerComponent

  public:
  plAnimationControllerComponent();
  ~plAnimationControllerComponent();

  void SetAnimGraphFile(const char* szFile); // [ property ]
  const char* GetAnimGraphFile() const;      // [ property ]

  plEnum<plAnimationInvisibleUpdateRate> m_InvisibleUpdateRate; // [ property ]

  protected:
  void Update();

  plEnum<plRootMotionMode> m_RootMotionMode;

  plAnimGraphResourceHandle m_hAnimGraph;
  plAnimController m_AnimController;
  plAnimPoseGenerator m_PoseGenerator;

  plTime m_ElapsedTimeSinceUpdate = plTime::Zero();
};

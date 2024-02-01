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

/// \brief Evaluates an plAnimGraphResource and provides the result through the plMsgAnimationPoseUpdated.
///
/// plAnimGraph's contain logic to generate an animation pose. This component decides when it is necessary
/// to reevaluate the state, which mostly means it tracks when the object is visible.
///
/// The result is sent as a recursive message, which is usually consumed by an plAnimatedMeshComponent.
/// The mesh component may be on the same game object or a child object.
class PL_GAMEENGINE_DLL plAnimationControllerComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plAnimationControllerComponent, plComponent, plAnimationControllerComponentManager);

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

  /// \brief Sets the plAnimGraphResource file to use.
  void SetAnimGraphFile(const char* szFile); // [ property ]
  const char* GetAnimGraphFile() const;      // [ property ]

  /// \brief How often to update the animation while the animated mesh is invisible.
  plEnum<plAnimationInvisibleUpdateRate> m_InvisibleUpdateRate; // [ property ]

protected:
  void Update();

  plEnum<plRootMotionMode> m_RootMotionMode;

  plAnimGraphResourceHandle m_hAnimGraph;
  plAnimController m_AnimController;
  plAnimPoseGenerator m_PoseGenerator;

  plTime m_ElapsedTimeSinceUpdate = plTime::MakeZero();
};

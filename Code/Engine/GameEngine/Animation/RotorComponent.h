#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

using plRotorComponentManager = plComponentManagerSimple<class plRotorComponent, plComponentUpdateType::WhenSimulating>;

/// \brief Applies a rotation to the game object that it is attached to.
///
/// The rotation may be endless, or limited to a certain amount of rotation.
/// It may also automatically turn around and accelerate and decelerate.
class PL_GAMEENGINE_DLL plRotorComponent : public plTransformComponent
{
  PL_DECLARE_COMPONENT_TYPE(plRotorComponent, plTransformComponent, plRotorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plRotorComponent

public:
  plRotorComponent();
  ~plRotorComponent();

  /// \brief How much to rotate before reaching the end and either stopping or turning around.
  /// Set to zero for endless rotation.
  plInt32 m_iDegreeToRotate = 0; // [ property ]

  /// \brief The acceleration to reach the target speed.
  float m_fAcceleration = 1.0f; // [ property ]

  /// \brief The deceleration to brake to zero speed before reaching the end rotation.
  float m_fDeceleration = 1.0f; // [ property ]

  /// \brief The axis around which to rotate. In local space of the game object.
  plEnum<plBasisAxis> m_Axis = plBasisAxis::PositiveZ; // [ property ]

  /// \brief How much the rotation axis may randomly deviate to not have all objects rotate the same way.
  plAngle m_AxisDeviation; // [ property ]

protected:
  void Update();

  plVec3 m_vRotationAxis = plVec3(0, 0, 1);
  plQuat m_qLastRotation = plQuat::MakeIdentity();
};

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

typedef plComponentManagerSimple<class plRotorComponent, plComponentUpdateType::WhenSimulating> plRotorComponentManager;

class PLASMA_GAMEENGINE_DLL plRotorComponent : public plTransformComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plRotorComponent, plTransformComponent, plRotorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // plRotorComponent

public:
  plRotorComponent();
  ~plRotorComponent();

  plInt32 m_iDegreeToRotate = 0;                       // [ property ]
  float m_fAcceleration = 1.0f;                        // [ property ]
  float m_fDeceleration = 1.0f;                        // [ property ]
  plEnum<plBasisAxis> m_Axis = plBasisAxis::PositiveZ; // [ property ]
  plAngle m_AxisDeviation;                             // [ property ]

protected:
  void Update();

  plVec3 m_vRotationAxis = plVec3(0, 0, 1);
  plQuat m_qLastRotation = plQuat::MakeIdentity();
};

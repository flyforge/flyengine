#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

typedef plComponentManagerSimple<class plSliderComponent, plComponentUpdateType::WhenSimulating> plSliderComponentManager;

class PLASMA_GAMEENGINE_DLL plSliderComponent : public plTransformComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSliderComponent, plTransformComponent, plSliderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // plSliderComponent

public:
  plSliderComponent();
  ~plSliderComponent();

  float m_fDistanceToTravel = 1.0f;                    // [ property ]
  float m_fAcceleration = 0.0f;                        // [ property ]
  float m_fDeceleration = 0.0;                         // [ property ]
  plEnum<plBasisAxis> m_Axis = plBasisAxis::PositiveZ; // [ property ]
  plTime m_RandomStart;                                // [ property ]

protected:
  void Update();

  float m_fLastDistance = 0.0f;
};

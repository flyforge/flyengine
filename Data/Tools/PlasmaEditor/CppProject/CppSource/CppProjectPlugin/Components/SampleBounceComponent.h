#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <CppProjectPlugin/CppProjectPluginDLL.h>

using SampleBounceComponentManager = plComponentManagerSimple<class SampleBounceComponent, plComponentUpdateType::WhenSimulating>;

class SampleBounceComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(SampleBounceComponent, plComponent, SampleBounceComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // SampleBounceComponent

public:
  SampleBounceComponent();
  ~SampleBounceComponent();

private:
  void Update();

  float m_fAmplitude = 1.0f;             // [ property ]
  plAngle m_Speed = plAngle::MakeFromDegree(90); // [ property ]
};

#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

// BEGIN-DOCS-CODE-SNIPPET: customcomp-manager
using DemoComponentManager = plComponentManagerSimple<class DemoComponent, plComponentUpdateType::WhenSimulating>;
// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: customcomp-class
class DemoComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(DemoComponent, plComponent, DemoComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // DemoComponent

public:
  DemoComponent();
  ~DemoComponent();

private:
  void Update();

  float m_fAmplitude = 1.0f;             // [ property ]
  plAngle m_Speed = plAngle::MakeFromDegree(90); // [ property ]
};
// END-DOCS-CODE-SNIPPET

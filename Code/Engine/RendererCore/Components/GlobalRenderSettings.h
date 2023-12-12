#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <RendererCore/RendererCoreDLL.h>

using plGlobalRenderSettingsComponentManager = plComponentManagerSimple<class plGlobalRenderSettingsComponent, plComponentUpdateType::Always>;

class PLASMA_RENDERERCORE_DLL plGlobalRenderSettingsComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plGlobalRenderSettingsComponent, plComponent, plGlobalRenderSettingsComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  plGlobalRenderSettingsComponent();
  ~plGlobalRenderSettingsComponent();

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  void SetTaaActive(bool bActive); // [ property ]
  bool GetTaaActive() const;       // [ property ]

  void SetTaaUpscaleActive(bool bActive); // [ property ]
  bool GetTaaUpscaleActive() const;       // [ property ]

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void Update();
private:
  // TAA settings
  bool m_bTaaActive = true; // [ property ]
  bool m_bTaaUpscaleActive = true; // [ property ]
};
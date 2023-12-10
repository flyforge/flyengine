#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/RendererCoreDLL.h>

struct plMsgUpdateLocalBounds;

using plAmbientLightComponentManager = plSettingsComponentManager<class plAmbientLightComponent>;

class PLASMA_RENDERERCORE_DLL plAmbientLightComponent : public plSettingsComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plAmbientLightComponent, plSettingsComponent, plAmbientLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plAmbientLightComponent

public:
  plAmbientLightComponent();
  ~plAmbientLightComponent();

  void SetTopColor(plColorGammaUB color); // [ property ]
  plColorGammaUB GetTopColor() const;     // [ property ]

  void SetBottomColor(plColorGammaUB color); // [ property ]
  plColorGammaUB GetBottomColor() const;     // [ property ]

  void SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;          // [ property ]

private:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);
  void UpdateSkyIrradiance();

  plColorGammaUB m_TopColor = plColor(0.2f, 0.2f, 0.3f);
  plColorGammaUB m_BottomColor = plColor(0.1f, 0.1f, 0.15f);
  float m_fIntensity = 1.0f;
};

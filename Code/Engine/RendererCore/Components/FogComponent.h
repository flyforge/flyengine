#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Pipeline/RenderData.h>

struct plMsgUpdateLocalBounds;

using plFogComponentManager = plSettingsComponentManager<class plFogComponent>;

/// \brief The render data object for ambient light.
class PL_RENDERERCORE_DLL plFogRenderData : public plRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plFogRenderData, plRenderData);

public:
  plColor m_Color;
  float m_fDensity;
  float m_fHeightFalloff;
  float m_fInvSkyDistance;
};

class PL_RENDERERCORE_DLL plFogComponent : public plSettingsComponent
{
  PL_DECLARE_COMPONENT_TYPE(plFogComponent, plSettingsComponent, plFogComponentManager);

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
  // plFogComponent

public:
  plFogComponent();
  ~plFogComponent();

  void SetColor(plColor color); // [ property ]
  plColor GetColor() const;     // [ property ]

  void SetDensity(float fDensity); // [ property ]
  float GetDensity() const;        // [ property ]

  void SetHeightFalloff(float fHeightFalloff); // [ property ]
  float GetHeightFalloff() const;              // [ property ]

  void SetModulateWithSkyColor(bool bModulate); // [ property ]
  bool GetModulateWithSkyColor() const;         // [ property ]

  void SetSkyDistance(float fDistance); // [ property ]
  float GetSkyDistance() const;         // [ property ]

protected:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  plColor m_Color = plColor(0.2f, 0.2f, 0.3f);
  float m_fDensity = 1.0f;
  float m_fHeightFalloff = 10.0f;
  float m_fSkyDistance = 1000.0f;
  bool m_bModulateWithSkyColor = false;
};

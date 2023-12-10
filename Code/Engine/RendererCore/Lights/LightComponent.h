#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct plMsgSetColor;

/// \brief Base class for light render data objects.
class PLASMA_RENDERERCORE_DLL plLightRenderData : public plRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLightRenderData, plRenderData);

public:
  void FillBatchIdAndSortingKey(float fScreenSpaceSize);

  plColor m_LightColor;
  float m_fIntensity;
  plUInt32 m_uiShadowDataOffset;
};

/// \brief Base class for all pl light components containing shared properties
class PLASMA_RENDERERCORE_DLL plLightComponent : public plRenderComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plLightComponent, plRenderComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // plLightComponent

public:
  plLightComponent();
  ~plLightComponent();

  void SetLightColor(plColorGammaUB lightColor); // [ property ]
  plColorGammaUB GetLightColor() const;          // [ property ]

  void SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;          // [ property ]

  void SetCastShadows(bool bCastShadows); // [ property ]
  bool GetCastShadows() const;            // [ property ]

  void SetPenumbraSize(float fPenumbraSize); // [ property ]
  float GetPenumbraSize() const;             // [ property ]

  void SetSlopeBias(float fShadowBias); // [ property ]
  float GetSlopeBias() const;           // [ property ]

  void SetConstantBias(float fShadowBias); // [ property ]
  float GetConstantBias() const;           // [ property ]

  void OnMsgSetColor(plMsgSetColor& ref_msg); // [ msg handler ]

  static float CalculateEffectiveRange(float fRange, float fIntensity);
  static float CalculateScreenSpaceSize(const plBoundingSphere& sphere, const plCamera& camera);

protected:
  plColorGammaUB m_LightColor = plColor::White;
  float m_fIntensity = 10.0f;
  float m_fPenumbraSize = 0.1f;
  float m_fSlopeBias = 0.25f;
  float m_fConstantBias = 0.1f;
  bool m_bCastShadows = false;
};

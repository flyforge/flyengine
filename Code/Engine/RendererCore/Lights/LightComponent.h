#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct plMsgSetColor;

/// \brief Specifies in which mode this camera is configured.
struct PLASMA_RENDERERCORE_DLL plLightUnit
{
  typedef plInt8 StorageType;

  enum Enum
  {
    Lumen,
    Candela,
    Default = Lumen
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plLightUnit);


/// \brief Base class for light render data objects.
class PLASMA_RENDERERCORE_DLL plLightRenderData : public plRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLightRenderData, plRenderData);

public:
  void FillBatchIdAndSortingKey(float fScreenSpaceSize);

  plColor m_LightColor;
  float m_fIntensity;
  float m_fSpecularMultiplier;
  float m_fTemperature;
  plUInt32 m_uiShadowDataOffset;
  float m_fVolumetricIntensity;
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

  void SetLightUnit(plEnum<plLightUnit> lightUnit); // [ property ]
  plEnum<plLightUnit> GetLightUnit() const;           // [ property ]

  void SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;          // [ property ]

  void SetSpecularMultiplier(float fSpecularMultiplier); // [ property ]
  float GetSpecularMultiplier() const;          // [ property ]

  void SetVolumetricIntensity(float fVolumetricIntensity); // [ property ]
  float GetVolumetricIntensity() const;          // [ property ]

  void SetTemperature(float fTemperature); // [ property ]
  float GetTemperature() const;          // [ property ]

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
  plEnum<plLightUnit> m_LightUnit;
  float m_fIntensity = 800.0f;
  float m_fSpecularMultiplier = 1.0f;
  float m_fTemperature = 6550.0f;
  float m_fPenumbraSize = 0.1f;
  float m_fSlopeBias = 0.25f;
  float m_fConstantBias = 0.1f;
  bool m_bCastShadows = false;
  float m_fVolumetricIntensity = 1.0f;
};

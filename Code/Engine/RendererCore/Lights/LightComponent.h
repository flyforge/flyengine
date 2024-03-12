#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct plMsgSetColor;

/// \brief Base class for light render data objects.
class PL_RENDERERCORE_DLL plLightRenderData : public plRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plLightRenderData, plRenderData);

public:
  void FillBatchIdAndSortingKey(float fScreenSpaceSize);

  plColor m_LightColor;
  float m_fIntensity;
  float m_fSpecularMultiplier;
  plUInt32 m_uiShadowDataOffset;
  float m_fWidth;
  float m_fLength;
};

/// \brief Base class for dynamic light components.
class PL_RENDERERCORE_DLL plLightComponent : public plRenderComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plLightComponent, plRenderComponent);

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

  /// \brief Used to enable kelvin color values. This is a physical representation of light color using.
  /// for more detail: https://wikipedia.org/wiki/Color_temperature
  void SetUsingColorTemperature(bool bUseColorTemperature);
  bool GetUsingColorTemperature() const;

  void SetLightColor(plColorGammaUB lightColor); // [ property ]
  plColorGammaUB GetBaseLightColor() const;          // [ property ]

  plColorGammaUB GetLightColor() const;

  void SetTemperature(plUInt32 uiTemperature); // [ property ]
  plUInt32 GetTemperature() const;            // [ property ]

  /// \brief Sets the brightness of the lightsource.
  void SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;          // [ property ]

  void SetSpecularMultiplier(float fSpecularMultiplier); // [ property ]
  float GetSpecularMultiplier() const;                   // [ property ]

  /// \brief Sets whether the lightsource shall cast dynamic shadows.
  void SetCastShadows(bool bCastShadows); // [ property ]
  bool GetCastShadows() const;            // [ property ]

  /// \brief Sets the fuzziness of the shadow edges.
  void SetPenumbraSize(float fPenumbraSize); // [ property ]
  float GetPenumbraSize() const;             // [ property ]

  /// \brief Allows to tweak how dynamic shadows are applied to reduce artifacts.
  void SetSlopeBias(float fShadowBias); // [ property ]
  float GetSlopeBias() const;           // [ property ]

  /// \brief Allows to tweak how dynamic shadows are applied to reduce artifacts.
  void SetConstantBias(float fShadowBias); // [ property ]
  float GetConstantBias() const;           // [ property ]

  /// \brief Sets the width of the area light
  void SetWidth(float fWidth); // [ property ]
  float GetWidth() const;      // [ property ]

  /// \brief Sets the length of the area light
  void SetLength(float fLength); // [ property ]
  float GetLength() const;      // [ property ]

  void OnMsgSetColor(plMsgSetColor& ref_msg); // [ msg handler ]

  /// \brief Calculates how far a lightsource would shine given the specified range and intensity.
  ///
  /// If fRange is zero, the range needed for the given intensity is returned.
  /// Otherwise the smaller value of that and fRange is returned.
  static float CalculateEffectiveRange(float fRange, float fIntensity);

  /// \brief Calculates how large on screen (in pixels) the lightsource would be.
  static float CalculateScreenSpaceSize(const plBoundingSphere& sphere, const plCamera& camera);

protected:
  plColorGammaUB m_LightColor = plColor::White;
  plUInt32 m_uiTemperature = 6550;
  float m_fIntensity = 10.0f;
  float m_fSpecularMultiplier = 1.0f;
  float m_fPenumbraSize = 0.1f;
  float m_fSlopeBias = 0.25f;
  float m_fConstantBias = 0.1f;
  bool m_bCastShadows = false;
  bool m_bUseColorTemperature = false;
  float m_fWidth;
  float m_fLength;
};

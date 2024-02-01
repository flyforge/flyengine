#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>

using plDirectionalLightComponentManager = plComponentManager<class plDirectionalLightComponent, plBlockStorageType::Compact>;

/// \brief The render data object for directional lights.
class PL_RENDERERCORE_DLL plDirectionalLightRenderData : public plLightRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plDirectionalLightRenderData, plLightRenderData);

public:
};

/// \brief A directional lightsource shines light into one fixed direction and has infinite size. It is usually used for sunlight.
///
/// It is very rare to use more than one directional lightsource at the same time.
/// Directional lightsources are used to fake the large scale light of the sun (or moon).
/// They use cascaded shadow maps to reduce the performance overhead for dynamic shadows of such large lights.
class PL_RENDERERCORE_DLL plDirectionalLightComponent : public plLightComponent
{
  PL_DECLARE_COMPONENT_TYPE(plDirectionalLightComponent, plLightComponent, plDirectionalLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // plDirectionalLightComponent

public:
  plDirectionalLightComponent();
  ~plDirectionalLightComponent();

  /// \brief Sets how many shadow map cascades to use. Typically between 2 and 4.
  void SetNumCascades(plUInt32 uiNumCascades); // [ property ]
  plUInt32 GetNumCascades() const;             // [ property ]

  /// \brief Sets the distance around the main camera in which to apply dynamic shadows.
  void SetMinShadowRange(float fMinShadowRange); // [ property ]
  float GetMinShadowRange() const;               // [ property ]

  /// \brief The factor (0 to 1) at which relative distance to start fading out the shadow map. Typically 0.8 or 0.9.
  void SetFadeOutStart(float fFadeOutStart); // [ property ]
  float GetFadeOutStart() const;             // [ property ]

  /// \brief Has something to do with shadow map cascades (TODO: figure out what).
  void SetSplitModeWeight(float fSplitModeWeight); // [ property ]
  float GetSplitModeWeight() const;                // [ property ]

  /// \brief Has something to do with shadow map cascades (TODO: figure out what).
  void SetNearPlaneOffset(float fNearPlaneOffset); // [ property ]
  float GetNearPlaneOffset() const;                // [ property ]

protected:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  plUInt32 m_uiNumCascades = 3;
  float m_fMinShadowRange = 50.0f;
  float m_fFadeOutStart = 0.8f;
  float m_fSplitModeWeight = 0.7f;
  float m_fNearPlaneOffset = 100.0f;
};

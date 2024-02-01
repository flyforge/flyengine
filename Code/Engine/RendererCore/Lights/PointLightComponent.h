#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Textures/TextureCubeResource.h>

using plPointLightComponentManager = plComponentManager<class plPointLightComponent, plBlockStorageType::Compact>;

/// \brief The render data object for point lights.
class PL_RENDERERCORE_DLL plPointLightRenderData : public plLightRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plPointLightRenderData, plLightRenderData);

public:
  float m_fRange;
  // plTextureCubeResourceHandle m_hProjectedTexture;
};

/// \brief Adds a dynamic point light to the scene, optionally casting shadows.
///
/// For performance reasons, prefer to use plSpotLightComponent where possible.
/// Do not use shadows just to limit the light cone, when a spot light could achieve the same.
class PL_RENDERERCORE_DLL plPointLightComponent : public plLightComponent
{
  PL_DECLARE_COMPONENT_TYPE(plPointLightComponent, plLightComponent, plPointLightComponentManager);

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
  // plPointLightComponent

public:
  plPointLightComponent();
  ~plPointLightComponent();

  /// \brief Sets the radius of the lightsource. If zero, the radius is automatically determined from the intensity.
  void SetRange(float fRange); // [ property ]
  float GetRange() const;      // [ property ]

  /// \brief Returns the final radius of the lightsource.
  float GetEffectiveRange() const;

  // void SetProjectedTextureFile(const char* szFile); // [ property ]
  // const char* GetProjectedTextureFile() const;      // [ property ]

  // void SetProjectedTexture(const plTextureCubeResourceHandle& hProjectedTexture);
  // const plTextureCubeResourceHandle& GetProjectedTexture() const;

protected:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  float m_fRange = 0.0f;
  float m_fEffectiveRange = 0.0f;

  // plTextureCubeResourceHandle m_hProjectedTexture;
};

/// \brief A special visualizer attribute for point lights
class PL_RENDERERCORE_DLL plPointLightVisualizerAttribute : public plVisualizerAttribute
{
  PL_ADD_DYNAMIC_REFLECTION(plPointLightVisualizerAttribute, plVisualizerAttribute);

public:
  plPointLightVisualizerAttribute();
  plPointLightVisualizerAttribute(const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty);

  const plUntrackedString& GetRangeProperty() const { return m_sProperty1; }
  const plUntrackedString& GetIntensityProperty() const { return m_sProperty2; }
  const plUntrackedString& GetColorProperty() const { return m_sProperty3; }
};

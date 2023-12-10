#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Textures/TextureCubeResource.h>

using plPointLightComponentManager = plComponentManager<class plPointLightComponent, plBlockStorageType::Compact>;

/// \brief The render data object for point lights.
class PLASMA_RENDERERCORE_DLL plPointLightRenderData : public plLightRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPointLightRenderData, plLightRenderData);

public:
  float m_fRange;
  plTextureCubeResourceHandle m_hProjectedTexture;
};

/// \brief The standard point light component.
/// This component represents point lights with various properties (e.g. a projected cube map, range, etc.)
class PLASMA_RENDERERCORE_DLL plPointLightComponent : public plLightComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plPointLightComponent, plLightComponent, plPointLightComponentManager);

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

  void SetRange(float fRange); // [ property ]
  float GetRange() const;      // [ property ]

  float GetEffectiveRange() const;

  void SetProjectedTextureFile(const char* szFile); // [ property ]
  const char* GetProjectedTextureFile() const;      // [ property ]

  void SetProjectedTexture(const plTextureCubeResourceHandle& hProjectedTexture);
  const plTextureCubeResourceHandle& GetProjectedTexture() const;

protected:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  float m_fRange = 0.0f;
  float m_fEffectiveRange = 0.0f;

  plTextureCubeResourceHandle m_hProjectedTexture;
};

/// \brief A special visualizer attribute for point lights
class PLASMA_RENDERERCORE_DLL plPointLightVisualizerAttribute : public plVisualizerAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPointLightVisualizerAttribute, plVisualizerAttribute);

public:
  plPointLightVisualizerAttribute();
  plPointLightVisualizerAttribute(const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty);

  const plUntrackedString& GetRangeProperty() const { return m_sProperty1; }
  const plUntrackedString& GetIntensityProperty() const { return m_sProperty2; }
  const plUntrackedString& GetColorProperty() const { return m_sProperty3; }
};

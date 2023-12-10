#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Textures/Texture2DResource.h>

using plSpotLightComponentManager = plComponentManager<class plSpotLightComponent, plBlockStorageType::Compact>;

/// \brief The render data object for spot lights.
class PLASMA_RENDERERCORE_DLL plSpotLightRenderData : public plLightRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSpotLightRenderData, plLightRenderData);

public:
  float m_fRange;
  plAngle m_InnerSpotAngle;
  plAngle m_OuterSpotAngle;
  plTexture2DResourceHandle m_hProjectedTexture;
};

/// \brief The standard spot light component.
/// This component represents spot lights with various properties (e.g. a projected texture, range, spot angle, etc.)
class PLASMA_RENDERERCORE_DLL plSpotLightComponent : public plLightComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSpotLightComponent, plLightComponent, plSpotLightComponentManager);

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
  // plSpotLightComponent

public:
  plSpotLightComponent();
  ~plSpotLightComponent();

  void SetRange(float fRange); // [ property ]
  float GetRange() const;      // [ property ]

  float GetEffectiveRange() const;

  void SetInnerSpotAngle(plAngle spotAngle);  // [ property ]
  plAngle GetInnerSpotAngle() const;          // [ property ]

  void SetOuterSpotAngle(plAngle spotAngle);  // [ property ]
  plAngle GetOuterSpotAngle() const;          // [ property ]

  void SetProjectedTextureFile(const char* szFile); // [ property ]
  const char* GetProjectedTextureFile() const;      // [ property ]

  void SetProjectedTexture(const plTexture2DResourceHandle& hProjectedTexture);
  const plTexture2DResourceHandle& GetProjectedTexture() const;

protected:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;
  plBoundingSphere CalculateBoundingSphere(const plTransform& t, float fRange) const;

  float m_fRange = 0.0f;
  float m_fEffectiveRange = 0.0f;

  plAngle m_InnerSpotAngle = plAngle::MakeFromDegree(15.0f);
  plAngle m_OuterSpotAngle = plAngle::MakeFromDegree(30.0f);

  plTexture2DResourceHandle m_hProjectedTexture;
};

/// \brief A special visualizer attribute for spot lights
class PLASMA_RENDERERCORE_DLL plSpotLightVisualizerAttribute : public plVisualizerAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSpotLightVisualizerAttribute, plVisualizerAttribute);

public:
  plSpotLightVisualizerAttribute();
  plSpotLightVisualizerAttribute(
    const char* szAngleProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty);

  const plUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const plUntrackedString& GetRangeProperty() const { return m_sProperty2; }
  const plUntrackedString& GetIntensityProperty() const { return m_sProperty3; }
  const plUntrackedString& GetColorProperty() const { return m_sProperty4; }
};

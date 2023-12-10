#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>

using plDirectionalLightComponentManager = plComponentManager<class plDirectionalLightComponent, plBlockStorageType::Compact>;

/// \brief The render data object for directional lights.
class PLASMA_RENDERERCORE_DLL plDirectionalLightRenderData : public plLightRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDirectionalLightRenderData, plLightRenderData);

public:
};

/// \brief The standard directional light component.
/// This component represents directional lights.
class PLASMA_RENDERERCORE_DLL plDirectionalLightComponent : public plLightComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plDirectionalLightComponent, plLightComponent, plDirectionalLightComponentManager);

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

  void SetNumCascades(plUInt32 uiNumCascades); // [ property ]
  plUInt32 GetNumCascades() const;             // [ property ]

  void SetMinShadowRange(float fMinShadowRange); // [ property ]
  float GetMinShadowRange() const;               // [ property ]

  void SetFadeOutStart(float fFadeOutStart); // [ property ]
  float GetFadeOutStart() const;             // [ property ]

  void SetSplitModeWeight(float fSplitModeWeight); // [ property ]
  float GetSplitModeWeight() const;                // [ property ]

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

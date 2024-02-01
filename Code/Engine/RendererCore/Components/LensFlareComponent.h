#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct plMsgSetColor;
using plTexture2DResourceHandle = plTypedResourceHandle<class plTexture2DResource>;

class PL_RENDERERCORE_DLL plLensFlareRenderData : public plRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plLensFlareRenderData, plRenderData);

public:
  void FillBatchIdAndSortingKey();

  plTexture2DResourceHandle m_hTexture;
  plFloat16Vec4 m_Color;

  float m_fSize;
  float m_fMaxScreenSize;
  float m_fAspectRatio;
  float m_fShiftToCenter;
  float m_fOcclusionSampleRadius;
  float m_fOcclusionSampleSpread;
  float m_fOcclusionDepthOffset;

  bool m_bInverseTonemap;
  bool m_bGreyscaleTexture;
  bool m_bApplyFog;
};

/// \brief Represents an individual element of a lens flare.
struct plLensFlareElement
{
  plTexture2DResourceHandle m_hTexture;
  plColor m_Color = plColor::White;
  float m_fSize = 10000.0f;            ///< World space size
  float m_fMaxScreenSize = 1.0f;       ///< Relative screen space size in 0..1 range
  float m_fAspectRatio = 1.0f;         ///< Width:height ratio, only height is adjusted while width stays fixed
  float m_fShiftToCenter = 0.0f;       ///< Move the element along the lens flare origin to screen center line. 0 is at the lens flare origin, 1 at the screen center. Values below 0 or above 1 are also possible.
  bool m_bGreyscaleTexture = false;    ///< Whether the given texture is a greyscale or color texture.
  bool m_bModulateByLightColor = true; ///< Modulate the element's color by the light color and intensity if the lens flare component is linked to a light component.
  bool m_bInverseTonemap = false;      ///< Apply an inverse tonemapping operation on the final color. This can be useful if the lens flare is not linked to a light or does not use an hdr color since lens flares are rendered before tonemapping and can look washed out in this case.

  void SetTextureFile(const char* szFile); // [ property ]
  const char* GetTextureFile() const;      // [ property ]

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plLensFlareElement);

using plLensFlareComponentManager = plComponentManager<class plLensFlareComponent, plBlockStorageType::Compact>;

/// \brief Adds a lensflare or corona effect to a lightsource.
///
/// This component can be used to add a lensflare effect to a lightsource, typically the sun.
/// It can, however, also be used on smaller lightsources. For a full lensflare one would add multiple billboard textures
/// that are positioned along a line that rotates around the screen center.
/// If only a single billboard is added and it is always at distance 'zero' along that line, it acts like a 'corona' that is
/// only at the location of the lightsource.
///
/// The lensflare renderer determines how much the lightsource is occluded and scales the transparency of the lensflare
/// accordingly.
///
/// The component does not require a lightsource, it can be attached to any other object, as well, it is just mostly
/// used in conjunction with a point or directional lightsource.
class PL_RENDERERCORE_DLL plLensFlareComponent : public plRenderComponent
{
  PL_DECLARE_COMPONENT_TYPE(plLensFlareComponent, plRenderComponent, plLensFlareComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // plLensFlareComponent

public:
  plLensFlareComponent();
  ~plLensFlareComponent();

  /// \brief Adjusts the overall intensity of the lens flare
  float m_fIntensity = 1.0f; // [ property ]

  /// \brief Link the lens flare to the first light component on the same owner object or any of its parent objects.
  ///
  /// When a lens flare is linked it will take the light color and intensity to modulate the lens flare color and intensity
  /// for elements that have the m_bModulateByLightColor flag set.
  /// For directional lights the lens flare is positioned at far plane and moved with the camera to simulate a light that is at
  /// infinite distance, like e.g. the sun.
  /// For spot lights the lens flare intensity is additionally adjusted so that the lens flare is only visible when the camera is
  /// inside the light cone.
  void SetLinkToLightShape(bool bLink);                            // [ property ]
  bool GetLinkToLightShape() const { return m_bLinkToLightShape; } // [ property ]

  /// \brief Sets the world space radius in which the depth buffer is sampled to determine how much the lens flare is occluded.
  /// Typically this would be the size of the light emitting area, like e.g. a lamp, or slightly larger.
  void SetOcclusionSampleRadius(float fRadius);                               // [ property ]
  float GetOcclusionSampleRadius() const { return m_fOcclusionSampleRadius; } // [ property ]

  /// \brief Move the occlusion sample center towards the lens flare corner to introduce a slight gradient
  /// when the lens flare is only partially occluded. This value is relative to the sample radius.
  float m_fOcclusionSampleSpread = 0.5f; // [ property ]

  /// \brief Adjusts the occlusion sample depth in world space. Negative values will move towards the camera.
  /// This can be used to prevent self occlusion with the light source object.
  float m_fOcclusionDepthOffset = 0.0f; // [ property ]

  plSmallArray<plLensFlareElement, 1> m_Elements; // [ property ]

private:
  void FindLightComponent();

  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  float m_fOcclusionSampleRadius = 0.1f;
  bool m_bLinkToLightShape = true;
  bool m_bApplyFog = true;

  bool m_bDirectionalLight = false;
  plComponentHandle m_hLightComponent;
};

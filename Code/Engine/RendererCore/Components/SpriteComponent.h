#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct plMsgSetColor;
using plTexture2DResourceHandle = plTypedResourceHandle<class plTexture2DResource>;

struct plSpriteBlendMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Masked,
    Transparent,
    Additive,
    ShapeIcon,

    Default = Masked
  };

  static plTempHashedString GetPermutationValue(Enum blendMode);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plSpriteBlendMode);

class PL_RENDERERCORE_DLL plSpriteRenderData : public plRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plSpriteRenderData, plRenderData);

public:
  void FillBatchIdAndSortingKey();

  plTexture2DResourceHandle m_hTexture;

  float m_fSize;
  float m_fMaxScreenSize;
  float m_fAspectRatio;
  plEnum<plSpriteBlendMode> m_BlendMode;

  plColor m_color;

  plVec2 m_texCoordScale;
  plVec2 m_texCoordOffset;

  plUInt32 m_uiUniqueID;
};

using plSpriteComponentManager = plComponentManager<class plSpriteComponent, plBlockStorageType::Compact>;

/// \brief Renders a screen-oriented quad (billboard) with a maximum screen size.
///
/// This component is typically used to attach an icon to a game object.
/// The sprite becomes smaller the farther away it is, but when you come closer, its screen size gets clamped to a fixed maximum.
///
/// It can also be used to render simple projectiles.
///
/// If you want to render a glow effect for a lightsource, use the plLensFlareComponent instead.
class PL_RENDERERCORE_DLL plSpriteComponent : public plRenderComponent
{
  PL_DECLARE_COMPONENT_TYPE(plSpriteComponent, plRenderComponent, plSpriteComponentManager);

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
  // plSpriteComponent

public:
  plSpriteComponent();
  ~plSpriteComponent();

  void SetTexture(const plTexture2DResourceHandle& hTexture);
  const plTexture2DResourceHandle& GetTexture() const;

  void SetTextureFile(const char* szFile); // [ property ]
  const char* GetTextureFile() const;      // [ property ]

  void SetColor(plColor color); // [ property ]
  plColor GetColor() const;     // [ property ]

  /// \brief Sets the size of the sprite in world-space units. This determines how large the sprite will be at certain distances.
  void SetSize(float fSize); // [ property ]
  float GetSize() const;     // [ property ]

  /// \brief Sets the maximum screen-space size in pixels. Once a sprite is close enough to have reached this size, it will not grow larger.
  void SetMaxScreenSize(float fSize); // [ property ]
  float GetMaxScreenSize() const;     // [ property ]

  void OnMsgSetColor(plMsgSetColor& ref_msg); // [ property ]

private:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  plTexture2DResourceHandle m_hTexture;
  plEnum<plSpriteBlendMode> m_BlendMode;
  plColor m_Color = plColor::White;

  float m_fSize = 1.0f;
  float m_fMaxScreenSize = 64.0f;
  float m_fAspectRatio = 1.0f;
};

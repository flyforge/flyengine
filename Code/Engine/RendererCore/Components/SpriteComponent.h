#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct plMsgSetColor;
using plTexture2DResourceHandle = plTypedResourceHandle<class plTexture2DResource>;

struct plSpriteBlendMode
{
  typedef plUInt8 StorageType;

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

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plSpriteBlendMode);

class PLASMA_RENDERERCORE_DLL plSpriteRenderData : public plRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSpriteRenderData, plRenderData);

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

typedef plComponentManager<class plSpriteComponent, plBlockStorageType::Compact> plSpriteComponentManager;

class PLASMA_RENDERERCORE_DLL plSpriteComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSpriteComponent, plRenderComponent, plSpriteComponentManager);

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

  void SetSize(float fSize); // [ property ]
  float GetSize() const;     // [ property ]

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

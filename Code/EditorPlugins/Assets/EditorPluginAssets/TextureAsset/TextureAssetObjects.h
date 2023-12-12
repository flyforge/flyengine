#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <Texture/TexConv/TexConvEnums.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct plPropertyMetaStateEvent;

struct plTexture2DChannelMappingEnum
{
  typedef plInt8 StorageType;

  enum Enum
  {
    R1,

    RG1,
    R1_G2,

    RGB1,
    R1_G2_B3,

    RGBA1,
    RGB1_A2,
    RGB1_ABLACK,
    R1_G2_B3_A4,

    Default = RGB1,
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plTexture2DChannelMappingEnum);

struct plTexture2DResolution
{
  typedef plInt8 StorageType;

  enum Enum
  {
    Fixed64x64,
    Fixed128x128,
    Fixed256x256,
    Fixed512x512,
    Fixed1024x1024,
    Fixed2048x2048,
    CVarRtResolution1,
    CVarRtResolution2,

    Default = Fixed256x256
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plTexture2DResolution);

struct plRenderTargetFormat
{
  typedef plInt8 StorageType;

  enum Enum
  {
    RGBA8sRgb,
    RGBA8,
    RGB10,
    RGBA16,

    Default = RGBA8sRgb
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plRenderTargetFormat);

class plTextureAssetProperties : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTextureAssetProperties, plReflectedClass);

public:
  static void PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);

  const char* GetInputFile(plInt32 iInput) const { return m_Input[iInput]; }

  void SetInputFile0(const char* szFile) { m_Input[0] = szFile; }
  const char* GetInputFile0() const { return m_Input[0]; }
  void SetInputFile1(const char* szFile) { m_Input[1] = szFile; }
  const char* GetInputFile1() const { return m_Input[1]; }
  void SetInputFile2(const char* szFile) { m_Input[2] = szFile; }
  const char* GetInputFile2() const { return m_Input[2]; }
  void SetInputFile3(const char* szFile) { m_Input[3] = szFile; }
  const char* GetInputFile3() const { return m_Input[3]; }

  plString GetAbsoluteInputFilePath(plInt32 iInput) const;

  plTexture2DChannelMappingEnum::Enum GetChannelMapping() const { return m_ChannelMapping; }

  plInt32 GetNumInputFiles() const;

  bool m_bIsRenderTarget = false;
  bool m_bPremultipliedAlpha = false;
  bool m_bFlipHorizontal = false;
  bool m_bDilateColor = false;
  bool m_bPreserveAlphaCoverage = false;
  float m_fCVarResolutionScale = 1.0f;
  float m_fHdrExposureBias = 0;
  float m_fAlphaThreshold = 0.5f;

  plEnum<plTextureFilterSetting> m_TextureFilter;
  plEnum<plImageAddressMode> m_AddressModeU;
  plEnum<plImageAddressMode> m_AddressModeV;
  plEnum<plImageAddressMode> m_AddressModeW;
  plEnum<plTexture2DResolution> m_Resolution;
  plEnum<plTexConvUsage> m_TextureUsage;
  plEnum<plRenderTargetFormat> m_RtFormat;

  plEnum<plTexConvCompressionMode> m_CompressionMode;
  plEnum<plTexConvMipmapMode> m_MipmapMode;

private:
  plEnum<plTexture2DChannelMappingEnum> m_ChannelMapping;
  plString m_Input[4];
};

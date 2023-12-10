#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>
// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plGALResourceFormat, 1)
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAFloat),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAUInt),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAInt),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBFloat),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBUInt),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBInt),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::B5G6R5UNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BGRAUByteNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BGRAUByteNormalizedsRGB),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAHalf),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAUShort),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAUShortNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAShort),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAShortNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGFloat),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGUInt),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGInt),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGB10A2UInt),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGB10A2UIntNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RG11B10Float),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAUByteNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAUByteNormalizedsRGB),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAUByte),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAByteNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAByte),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGHalf),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGUShort),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGUShortNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGShort),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGShortNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGUByte),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGUByteNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGByte),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGByteNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::DFloat),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RFloat),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RUInt),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RInt),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RHalf),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RUShort),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RUShortNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RShort),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RShortNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RUByte),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RUByteNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RByte),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::RByteNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::AUByteNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::D16),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::D24S8),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC1),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC1sRGB),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC2),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC2sRGB),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC3),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC3sRGB),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC4UNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC4Normalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC5UNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC5Normalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC6UFloat),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC6Float),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC7UNormalized),
    PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC7UNormalizedsRGB)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plGALMSAASampleCount, 1)
  PLASMA_ENUM_CONSTANTS(plGALMSAASampleCount::None, plGALMSAASampleCount::TwoSamples, plGALMSAASampleCount::FourSamples, plGALMSAASampleCount::EightSamples)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plGALTextureType, 1)
  PLASMA_ENUM_CONSTANTS(plGALTextureType::Invalid, plGALTextureType::Texture2D, plGALTextureType::TextureCube, plGALTextureType::Texture3D, plGALTextureType::Texture2DProxy, plGALTextureType::Texture2DShared)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plGALResourceAccess, plNoBase, 1, plRTTIDefaultAllocator<plGALResourceAccess>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ReadBack", m_bReadBack),
    PLASMA_MEMBER_PROPERTY("Immutable", m_bImmutable),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plGALTextureCreationDescription, plNoBase, 1, plRTTIDefaultAllocator<plGALTextureCreationDescription>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Width", m_uiWidth),
    PLASMA_MEMBER_PROPERTY("Height", m_uiHeight),
    PLASMA_MEMBER_PROPERTY("Depth", m_uiDepth),
    PLASMA_MEMBER_PROPERTY("MipLevelCount", m_uiMipLevelCount),
    PLASMA_MEMBER_PROPERTY("ArraySize", m_uiArraySize),
    PLASMA_ENUM_MEMBER_PROPERTY("Format", plGALResourceFormat, m_Format),
    PLASMA_ENUM_MEMBER_PROPERTY("SampleCount", plGALMSAASampleCount, m_SampleCount),
    PLASMA_ENUM_MEMBER_PROPERTY("Type", plGALTextureType, m_Type),
    PLASMA_MEMBER_PROPERTY("AllowShaderResourceView", m_bAllowShaderResourceView),
    PLASMA_MEMBER_PROPERTY("AllowUAV", m_bAllowUAV),
    PLASMA_MEMBER_PROPERTY("CreateRenderTarget", m_bCreateRenderTarget),
    PLASMA_MEMBER_PROPERTY("AllowDynamicMipGeneration", m_bAllowDynamicMipGeneration),
    PLASMA_MEMBER_PROPERTY("ResourceAccess", m_ResourceAccess),
    // m_pExisitingNativeObject deliberately not reflected as it can't be serialized in any meaningful way.
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plGALPlatformSharedHandle, plNoBase, 1, plRTTIDefaultAllocator<plGALPlatformSharedHandle>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("SharedTexture", m_hSharedTexture),
    PLASMA_MEMBER_PROPERTY("Semaphore", m_hSemaphore),
    PLASMA_MEMBER_PROPERTY("ProcessId", m_uiProcessId),
    PLASMA_MEMBER_PROPERTY("MemoryTypeIndex", m_uiMemoryTypeIndex),
    PLASMA_MEMBER_PROPERTY("Size", m_uiSize),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_RendererReflection);

#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>
// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plGALResourceFormat, 1)
  PL_ENUM_CONSTANT(plGALResourceFormat::RGBAFloat),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBAUInt),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBAInt),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBFloat),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBUInt),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBInt),
    PL_ENUM_CONSTANT(plGALResourceFormat::B5G6R5UNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::BGRAUByteNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::BGRAUByteNormalizedsRGB),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBAHalf),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBAUShort),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBAUShortNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBAShort),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBAShortNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGFloat),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGUInt),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGInt),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGB10A2UInt),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGB10A2UIntNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::RG11B10Float),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBAUByteNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBAUByteNormalizedsRGB),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBAUByte),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBAByteNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGBAByte),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGHalf),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGUShort),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGUShortNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGShort),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGShortNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGUByte),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGUByteNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGByte),
    PL_ENUM_CONSTANT(plGALResourceFormat::RGByteNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::DFloat),
    PL_ENUM_CONSTANT(plGALResourceFormat::RFloat),
    PL_ENUM_CONSTANT(plGALResourceFormat::RUInt),
    PL_ENUM_CONSTANT(plGALResourceFormat::RInt),
    PL_ENUM_CONSTANT(plGALResourceFormat::RHalf),
    PL_ENUM_CONSTANT(plGALResourceFormat::RUShort),
    PL_ENUM_CONSTANT(plGALResourceFormat::RUShortNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::RShort),
    PL_ENUM_CONSTANT(plGALResourceFormat::RShortNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::RUByte),
    PL_ENUM_CONSTANT(plGALResourceFormat::RUByteNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::RByte),
    PL_ENUM_CONSTANT(plGALResourceFormat::RByteNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::AUByteNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::D16),
    PL_ENUM_CONSTANT(plGALResourceFormat::D24S8),
    PL_ENUM_CONSTANT(plGALResourceFormat::BC1),
    PL_ENUM_CONSTANT(plGALResourceFormat::BC1sRGB),
    PL_ENUM_CONSTANT(plGALResourceFormat::BC2),
    PL_ENUM_CONSTANT(plGALResourceFormat::BC2sRGB),
    PL_ENUM_CONSTANT(plGALResourceFormat::BC3),
    PL_ENUM_CONSTANT(plGALResourceFormat::BC3sRGB),
    PL_ENUM_CONSTANT(plGALResourceFormat::BC4UNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::BC4Normalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::BC5UNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::BC5Normalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::BC6UFloat),
    PL_ENUM_CONSTANT(plGALResourceFormat::BC6Float),
    PL_ENUM_CONSTANT(plGALResourceFormat::BC7UNormalized),
    PL_ENUM_CONSTANT(plGALResourceFormat::BC7UNormalizedsRGB)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plGALMSAASampleCount, 1)
  PL_ENUM_CONSTANTS(plGALMSAASampleCount::None, plGALMSAASampleCount::TwoSamples, plGALMSAASampleCount::FourSamples, plGALMSAASampleCount::EightSamples)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plGALTextureType, 1)
  PL_ENUM_CONSTANTS(plGALTextureType::Invalid, plGALTextureType::Texture2D, plGALTextureType::TextureCube, plGALTextureType::Texture3D, plGALTextureType::Texture2DProxy, plGALTextureType::Texture2DShared)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_TYPE(plGALResourceAccess, plNoBase, 1, plRTTIDefaultAllocator<plGALResourceAccess>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ReadBack", m_bReadBack),
    PL_MEMBER_PROPERTY("Immutable", m_bImmutable),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plGALTextureCreationDescription, plNoBase, 1, plRTTIDefaultAllocator<plGALTextureCreationDescription>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Width", m_uiWidth),
    PL_MEMBER_PROPERTY("Height", m_uiHeight),
    PL_MEMBER_PROPERTY("Depth", m_uiDepth),
    PL_MEMBER_PROPERTY("MipLevelCount", m_uiMipLevelCount),
    PL_MEMBER_PROPERTY("ArraySize", m_uiArraySize),
    PL_ENUM_MEMBER_PROPERTY("Format", plGALResourceFormat, m_Format),
    PL_ENUM_MEMBER_PROPERTY("SampleCount", plGALMSAASampleCount, m_SampleCount),
    PL_ENUM_MEMBER_PROPERTY("Type", plGALTextureType, m_Type),
    PL_MEMBER_PROPERTY("AllowShaderResourceView", m_bAllowShaderResourceView),
    PL_MEMBER_PROPERTY("AllowUAV", m_bAllowUAV),
    PL_MEMBER_PROPERTY("CreateRenderTarget", m_bCreateRenderTarget),
    PL_MEMBER_PROPERTY("AllowDynamicMipGeneration", m_bAllowDynamicMipGeneration),
    PL_MEMBER_PROPERTY("ResourceAccess", m_ResourceAccess),
    // m_pExisitingNativeObject deliberately not reflected as it can't be serialized in any meaningful way.
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plGALPlatformSharedHandle, plNoBase, 1, plRTTIDefaultAllocator<plGALPlatformSharedHandle>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("SharedTexture", m_hSharedTexture),
    PL_MEMBER_PROPERTY("Semaphore", m_hSemaphore),
    PL_MEMBER_PROPERTY("ProcessId", m_uiProcessId),
    PL_MEMBER_PROPERTY("MemoryTypeIndex", m_uiMemoryTypeIndex),
    PL_MEMBER_PROPERTY("Size", m_uiSize),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

PL_STATICLINK_FILE(RendererFoundation, RendererFoundation_RendererReflection);

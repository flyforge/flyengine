#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureUtils.h>

bool plTextureUtils::s_bForceFullQualityAlways = false;

plGALResourceFormat::Enum plTextureUtils::ImageFormatToGalFormat(plImageFormat::Enum format, bool bSRGB)
{
  switch (format)
  {
    case plImageFormat::R8G8B8A8_UNORM:
      if (bSRGB)
        return plGALResourceFormat::RGBAUByteNormalizedsRGB;
      else
        return plGALResourceFormat::RGBAUByteNormalized;

      // case plImageFormat::R8G8B8A8_TYPELESS:
    case plImageFormat::R8G8B8A8_UNORM_SRGB:
      return plGALResourceFormat::RGBAUByteNormalizedsRGB;

    case plImageFormat::R8G8B8A8_UINT:
      return plGALResourceFormat::RGBAUInt;

    case plImageFormat::R8G8B8A8_SNORM:
      return plGALResourceFormat::RGBAByteNormalized;

    case plImageFormat::R8G8B8A8_SINT:
      return plGALResourceFormat::RGBAInt;

    case plImageFormat::B8G8R8A8_UNORM:
      if (bSRGB)
        return plGALResourceFormat::BGRAUByteNormalizedsRGB;
      else
        return plGALResourceFormat::BGRAUByteNormalized;

    case plImageFormat::B8G8R8X8_UNORM:
      if (bSRGB)
        return plGALResourceFormat::BGRAUByteNormalizedsRGB;
      else
        return plGALResourceFormat::BGRAUByteNormalized;

      // case plImageFormat::B8G8R8A8_TYPELESS:
    case plImageFormat::B8G8R8A8_UNORM_SRGB:
      return plGALResourceFormat::BGRAUByteNormalizedsRGB;

      // case plImageFormat::B8G8R8X8_TYPELESS:
    case plImageFormat::B8G8R8X8_UNORM_SRGB:
      return plGALResourceFormat::BGRAUByteNormalizedsRGB;

      // case plImageFormat::B8G8R8_UNORM:

      // case plImageFormat::BC1_TYPELESS:
    case plImageFormat::BC1_UNORM:
      if (bSRGB)
        return plGALResourceFormat::BC1sRGB;
      else
        return plGALResourceFormat::BC1;

    case plImageFormat::BC1_UNORM_SRGB:
      return plGALResourceFormat::BC1sRGB;

      // case plImageFormat::BC2_TYPELESS:
    case plImageFormat::BC2_UNORM:
      if (bSRGB)
        return plGALResourceFormat::BC2sRGB;
      else
        return plGALResourceFormat::BC2;

    case plImageFormat::BC2_UNORM_SRGB:
      return plGALResourceFormat::BC2sRGB;

      // case plImageFormat::BC3_TYPELESS:
    case plImageFormat::BC3_UNORM:
      if (bSRGB)
        return plGALResourceFormat::BC3sRGB;
      else
        return plGALResourceFormat::BC3;

    case plImageFormat::BC3_UNORM_SRGB:
      return plGALResourceFormat::BC3sRGB;

      // case plImageFormat::BC4_TYPELESS:
    case plImageFormat::BC4_UNORM:
      return plGALResourceFormat::BC4UNormalized;

    case plImageFormat::BC4_SNORM:
      return plGALResourceFormat::BC4Normalized;

      // case plImageFormat::BC5_TYPELESS:
    case plImageFormat::BC5_UNORM:
      return plGALResourceFormat::BC5UNormalized;

    case plImageFormat::BC5_SNORM:
      return plGALResourceFormat::BC5Normalized;

      // case plImageFormat::BC6H_TYPELESS:
    case plImageFormat::BC6H_UF16:
      return plGALResourceFormat::BC6UFloat;

    case plImageFormat::BC6H_SF16:
      return plGALResourceFormat::BC6Float;

      // case plImageFormat::BC7_TYPELESS:
    case plImageFormat::BC7_UNORM:
      if (bSRGB)
        return plGALResourceFormat::BC7UNormalizedsRGB;
      else
        return plGALResourceFormat::BC7UNormalized;

    case plImageFormat::BC7_UNORM_SRGB:
      return plGALResourceFormat::BC7UNormalizedsRGB;

    case plImageFormat::B5G6R5_UNORM:
      return plGALResourceFormat::B5G6R5UNormalized; /// \todo Not supported by some GPUs ?

    case plImageFormat::R16_FLOAT:
      return plGALResourceFormat::RHalf;

    case plImageFormat::R32_FLOAT:
      return plGALResourceFormat::RFloat;

    case plImageFormat::R16G16_FLOAT:
      return plGALResourceFormat::RGHalf;

    case plImageFormat::R32G32_FLOAT:
      return plGALResourceFormat::RGFloat;

    case plImageFormat::R32G32B32_FLOAT:
      return plGALResourceFormat::RGBFloat;

    case plImageFormat::R16G16B16A16_FLOAT:
      return plGALResourceFormat::RGBAHalf;

    case plImageFormat::R32G32B32A32_FLOAT:
      return plGALResourceFormat::RGBAFloat;

    case plImageFormat::R16G16B16A16_UNORM:
      return plGALResourceFormat::RGBAUShortNormalized;

    case plImageFormat::R8_UNORM:
      return plGALResourceFormat::RUByteNormalized;

    case plImageFormat::R8G8_UNORM:
      return plGALResourceFormat::RGUByteNormalized;

    case plImageFormat::R16G16_UNORM:
      return plGALResourceFormat::RGUShortNormalized;

    case plImageFormat::R11G11B10_FLOAT:
      return plGALResourceFormat::RG11B10Float;

    default:
      PL_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return plGALResourceFormat::Invalid;
}

plImageFormat::Enum plTextureUtils::GalFormatToImageFormat(plGALResourceFormat::Enum format)
{
  switch (format)
  {
    case plGALResourceFormat::RGBAFloat:
      return plImageFormat::R32G32B32A32_FLOAT;
    case plGALResourceFormat::RGBAUInt:
      return plImageFormat::R32G32B32A32_UINT;
    case plGALResourceFormat::RGBAInt:
      return plImageFormat::R32G32B32A32_SINT;
    case plGALResourceFormat::RGBFloat:
      return plImageFormat::R32G32B32_FLOAT;
    case plGALResourceFormat::RGBUInt:
      return plImageFormat::R32G32B32_UINT;
    case plGALResourceFormat::RGBInt:
      return plImageFormat::R32G32B32_SINT;
    case plGALResourceFormat::B5G6R5UNormalized:
      return plImageFormat::B5G6R5_UNORM;
    case plGALResourceFormat::BGRAUByteNormalized:
      return plImageFormat::B8G8R8A8_UNORM;
    case plGALResourceFormat::BGRAUByteNormalizedsRGB:
      return plImageFormat::B8G8R8A8_UNORM_SRGB;
    case plGALResourceFormat::RGBAHalf:
      return plImageFormat::R16G16B16A16_FLOAT;
    case plGALResourceFormat::RGBAUShort:
      return plImageFormat::R16G16B16A16_UINT;
    case plGALResourceFormat::RGBAUShortNormalized:
      return plImageFormat::R16G16B16A16_UNORM;
    case plGALResourceFormat::RGBAShort:
      return plImageFormat::R16G16B16A16_SINT;
    case plGALResourceFormat::RGBAShortNormalized:
      return plImageFormat::R16G16B16A16_SNORM;
    case plGALResourceFormat::RGFloat:
      return plImageFormat::R32G32_FLOAT;
    case plGALResourceFormat::RGUInt:
      return plImageFormat::R32G32_UINT;
    case plGALResourceFormat::RGInt:
      return plImageFormat::R32G32_SINT;
    case plGALResourceFormat::RG11B10Float:
      return plImageFormat::R11G11B10_FLOAT;
    case plGALResourceFormat::RGBAUByteNormalized:
      return plImageFormat::R8G8B8A8_UNORM;
    case plGALResourceFormat::RGBAUByteNormalizedsRGB:
      return plImageFormat::R8G8B8A8_UNORM_SRGB;
    case plGALResourceFormat::RGBAUByte:
      return plImageFormat::R8G8B8A8_UINT;
    case plGALResourceFormat::RGBAByteNormalized:
      return plImageFormat::R8G8B8A8_SNORM;
    case plGALResourceFormat::RGBAByte:
      return plImageFormat::R8G8B8A8_SINT;
    case plGALResourceFormat::RGHalf:
      return plImageFormat::R16G16_FLOAT;
    case plGALResourceFormat::RGUShort:
      return plImageFormat::R16G16_UINT;
    case plGALResourceFormat::RGUShortNormalized:
      return plImageFormat::R16G16_UNORM;
    case plGALResourceFormat::RGShort:
      return plImageFormat::R16G16_SINT;
    case plGALResourceFormat::RGShortNormalized:
      return plImageFormat::R16G16_SNORM;
    case plGALResourceFormat::RGUByte:
      return plImageFormat::R8G8_UINT;
    case plGALResourceFormat::RGUByteNormalized:
      return plImageFormat::R8G8_UNORM;
    case plGALResourceFormat::RGByte:
      return plImageFormat::R8G8_SINT;
    case plGALResourceFormat::RGByteNormalized:
      return plImageFormat::R8G8_SNORM;
    case plGALResourceFormat::DFloat:
      return plImageFormat::R32_FLOAT;
    case plGALResourceFormat::RFloat:
      return plImageFormat::R32_FLOAT;
    case plGALResourceFormat::RUInt:
      return plImageFormat::R32_UINT;
    case plGALResourceFormat::RInt:
      return plImageFormat::R32_SINT;
    case plGALResourceFormat::RHalf:
      return plImageFormat::R16_FLOAT;
    case plGALResourceFormat::RUShort:
      return plImageFormat::R16_UINT;
    case plGALResourceFormat::RUShortNormalized:
      return plImageFormat::R16_UNORM;
    case plGALResourceFormat::RShort:
      return plImageFormat::R16_SINT;
    case plGALResourceFormat::RShortNormalized:
      return plImageFormat::R16_SNORM;
    case plGALResourceFormat::RUByte:
      return plImageFormat::R8_UINT;
    case plGALResourceFormat::RUByteNormalized:
      return plImageFormat::R8_UNORM;
    case plGALResourceFormat::RByte:
      return plImageFormat::R8_SINT;
    case plGALResourceFormat::RByteNormalized:
      return plImageFormat::R8_SNORM;
    case plGALResourceFormat::AUByteNormalized:
      return plImageFormat::R8_UNORM;
    case plGALResourceFormat::D16:
      return plImageFormat::R16_UINT;
    case plGALResourceFormat::BC1:
      return plImageFormat::BC1_UNORM;
    case plGALResourceFormat::BC1sRGB:
      return plImageFormat::BC1_UNORM_SRGB;
    case plGALResourceFormat::BC2:
      return plImageFormat::BC2_UNORM;
    case plGALResourceFormat::BC2sRGB:
      return plImageFormat::BC2_UNORM_SRGB;
    case plGALResourceFormat::BC3:
      return plImageFormat::BC3_UNORM;
    case plGALResourceFormat::BC3sRGB:
      return plImageFormat::BC3_UNORM_SRGB;
    case plGALResourceFormat::BC4UNormalized:
      return plImageFormat::BC4_UNORM;
    case plGALResourceFormat::BC4Normalized:
      return plImageFormat::BC4_SNORM;
    case plGALResourceFormat::BC5UNormalized:
      return plImageFormat::BC5_UNORM;
    case plGALResourceFormat::BC5Normalized:
      return plImageFormat::BC5_SNORM;
    case plGALResourceFormat::BC6UFloat:
      return plImageFormat::BC6H_UF16;
    case plGALResourceFormat::BC6Float:
      return plImageFormat::BC6H_SF16;
    case plGALResourceFormat::BC7UNormalized:
      return plImageFormat::BC7_UNORM;
    case plGALResourceFormat::BC7UNormalizedsRGB:
      return plImageFormat::BC7_UNORM_SRGB;
    case plGALResourceFormat::RGB10A2UInt:
    case plGALResourceFormat::RGB10A2UIntNormalized:
    case plGALResourceFormat::D24S8:
    default:
    {
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
      plStringBuilder sFormat;
      PL_ASSERT_DEBUG(plReflectionUtils::EnumerationToString(plGetStaticRTTI<plGALResourceFormat>(), format, sFormat, plReflectionUtils::EnumConversionMode::ValueNameOnly), "Cannot convert GAL format '{}' to string", format);
      PL_ASSERT_DEBUG(false, "The GL format: '{}' does not have a matching image format.", sFormat);
#endif
    }
  }
  return plImageFormat::UNKNOWN;
}

plImageFormat::Enum plTextureUtils::GalFormatToImageFormat(plGALResourceFormat::Enum format, bool bRemoveSRGB)
{
  plImageFormat::Enum imageFormat = GalFormatToImageFormat(format);
  if (bRemoveSRGB)
  {
    imageFormat = plImageFormat::AsLinear(imageFormat);
  }
  return imageFormat;
}

void plTextureUtils::ConfigureSampler(plTextureFilterSetting::Enum filter, plGALSamplerStateCreationDescription& out_sampler)
{
  const plTextureFilterSetting::Enum thisFilter = plRenderContext::GetDefaultInstance()->GetSpecificTextureFilter(filter);

  out_sampler.m_MinFilter = plGALTextureFilterMode::Linear;
  out_sampler.m_MagFilter = plGALTextureFilterMode::Linear;
  out_sampler.m_MipFilter = plGALTextureFilterMode::Linear;
  out_sampler.m_uiMaxAnisotropy = 1;

  switch (thisFilter)
  {
    case plTextureFilterSetting::FixedNearest:
      out_sampler.m_MinFilter = plGALTextureFilterMode::Point;
      out_sampler.m_MagFilter = plGALTextureFilterMode::Point;
      out_sampler.m_MipFilter = plGALTextureFilterMode::Point;
      break;
    case plTextureFilterSetting::FixedBilinear:
      out_sampler.m_MipFilter = plGALTextureFilterMode::Point;
      break;
    case plTextureFilterSetting::FixedTrilinear:
      break;
    case plTextureFilterSetting::FixedAnisotropic2x:
      out_sampler.m_MinFilter = plGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = plGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 2;
      break;
    case plTextureFilterSetting::FixedAnisotropic4x:
      out_sampler.m_MinFilter = plGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = plGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 4;
      break;
    case plTextureFilterSetting::FixedAnisotropic8x:
      out_sampler.m_MinFilter = plGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = plGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 8;
      break;
    case plTextureFilterSetting::FixedAnisotropic16x:
      out_sampler.m_MinFilter = plGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = plGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 16;
      break;
    default:
      break;
  }
}



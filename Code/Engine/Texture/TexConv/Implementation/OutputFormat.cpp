#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/TexConv/TexConvProcessor.h>

static plImageFormat::Enum DetermineOutputFormatPC(
  plTexConvUsage::Enum targetFormat, plTexConvCompressionMode::Enum compressionMode, plUInt32 uiNumChannels)
{
  if (targetFormat == plTexConvUsage::NormalMap || targetFormat == plTexConvUsage::NormalMap_Inverted || targetFormat == plTexConvUsage::BumpMap)
  {
    if (compressionMode >= plTexConvCompressionMode::High)
      return plImageFormat::BC5_UNORM;

    if (compressionMode >= plTexConvCompressionMode::Medium)
      return plImageFormat::R8G8_UNORM;

    return plImageFormat::R16G16_UNORM;
  }

  if (targetFormat == plTexConvUsage::Color)
  {
    if (compressionMode >= plTexConvCompressionMode::High && uiNumChannels < 4)
      return plImageFormat::BC1_UNORM_SRGB;

    if (compressionMode >= plTexConvCompressionMode::Medium)
      return plImageFormat::BC7_UNORM_SRGB;

    return plImageFormat::R8G8B8A8_UNORM_SRGB;
  }

  if (targetFormat == plTexConvUsage::Linear)
  {
    switch (uiNumChannels)
    {
      case 1:
        if (compressionMode >= plTexConvCompressionMode::Medium)
          return plImageFormat::BC4_UNORM;

        return plImageFormat::R8_UNORM;

      case 2:
        if (compressionMode >= plTexConvCompressionMode::Medium)
          return plImageFormat::BC5_UNORM;

        return plImageFormat::R8G8_UNORM;

      case 3:
        if (compressionMode >= plTexConvCompressionMode::High)
          return plImageFormat::BC1_UNORM;

        if (compressionMode >= plTexConvCompressionMode::Medium)
          return plImageFormat::BC7_UNORM;

        return plImageFormat::R8G8B8A8_UNORM;

      case 4:
        if (compressionMode >= plTexConvCompressionMode::Medium)
          return plImageFormat::BC7_UNORM;

        return plImageFormat::R8G8B8A8_UNORM;

      default:
        PLASMA_ASSERT_NOT_IMPLEMENTED;
    }
  }

  if (targetFormat == plTexConvUsage::Hdr)
  {
    switch (uiNumChannels)
    {
      case 1:
        if (compressionMode >= plTexConvCompressionMode::High)
          return plImageFormat::BC6H_UF16;

        return plImageFormat::R16_FLOAT;

      case 2:
        return plImageFormat::R16G16_FLOAT;

      case 3:
        if (compressionMode >= plTexConvCompressionMode::High)
          return plImageFormat::BC6H_UF16;

        if (compressionMode >= plTexConvCompressionMode::Medium)
          return plImageFormat::R11G11B10_FLOAT;

        return plImageFormat::R16G16B16A16_FLOAT;

      case 4:
        return plImageFormat::R16G16B16A16_FLOAT;
    }
  }

  return plImageFormat::UNKNOWN;
}

plResult plTexConvProcessor::ChooseOutputFormat(plEnum<plImageFormat>& out_Format, plEnum<plTexConvUsage> usage, plUInt32 uiNumChannels) const
{
  PLASMA_PROFILE_SCOPE("ChooseOutputFormat");

  PLASMA_ASSERT_DEV(out_Format == plImageFormat::UNKNOWN, "Output format already set");

  switch (m_Descriptor.m_TargetPlatform)
  {
      // case  plTexConvTargetPlatform::Android:
      //  out_Format = DetermineOutputFormatAndroid(m_Descriptor.m_TargetFormat, m_Descriptor.m_CompressionMode);
      //  break;

    case plTexConvTargetPlatform::PC:
      out_Format = DetermineOutputFormatPC(usage, m_Descriptor.m_CompressionMode, uiNumChannels);
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  if (out_Format == plImageFormat::UNKNOWN)
  {
    plLog::Error("Failed to decide for an output image format.");
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}



PLASMA_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_OutputFormat);

#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plTexConvCompressionMode, 1)
  PL_ENUM_CONSTANTS(plTexConvCompressionMode::None, plTexConvCompressionMode::Medium, plTexConvCompressionMode::High)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plTexConvMipmapMode, 1)
  PL_ENUM_CONSTANTS(plTexConvMipmapMode::None, plTexConvMipmapMode::Linear, plTexConvMipmapMode::Kaiser)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plTexConvUsage, 1)
  PL_ENUM_CONSTANT(plTexConvUsage::Auto), PL_ENUM_CONSTANT(plTexConvUsage::Color), PL_ENUM_CONSTANT(plTexConvUsage::Linear),
  PL_ENUM_CONSTANT(plTexConvUsage::Hdr), PL_ENUM_CONSTANT(plTexConvUsage::NormalMap), PL_ENUM_CONSTANT(plTexConvUsage::NormalMap_Inverted),
  PL_ENUM_CONSTANT(plTexConvUsage::BumpMap),
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

plTexConvProcessor::plTexConvProcessor() = default;

plResult plTexConvProcessor::Process()
{
  PL_PROFILE_SCOPE("plTexConvProcessor::Process");

  if (m_Descriptor.m_OutputType == plTexConvOutputType::Atlas)
  {
    plMemoryStreamWriter stream(&m_TextureAtlas);
    PL_SUCCEED_OR_RETURN(GenerateTextureAtlas(stream));
  }
  else
  {
    PL_SUCCEED_OR_RETURN(LoadInputImages());

    PL_SUCCEED_OR_RETURN(AdjustUsage(m_Descriptor.m_InputFiles[0], m_Descriptor.m_InputImages[0], m_Descriptor.m_Usage));

    plStringBuilder sUsage;
    plReflectionUtils::EnumerationToString(
      plGetStaticRTTI<plTexConvUsage>(), m_Descriptor.m_Usage.GetValue(), sUsage, plReflectionUtils::EnumConversionMode::ValueNameOnly);
    plLog::Info("-usage is '{}'", sUsage);

    PL_SUCCEED_OR_RETURN(ForceSRGBFormats());

    plUInt32 uiNumChannelsUsed = 0;
    PL_SUCCEED_OR_RETURN(DetectNumChannels(m_Descriptor.m_ChannelMappings, uiNumChannelsUsed));

    plEnum<plImageFormat> OutputImageFormat;

    PL_SUCCEED_OR_RETURN(ChooseOutputFormat(OutputImageFormat, m_Descriptor.m_Usage, uiNumChannelsUsed));

    plLog::Info("Output image format is '{}'", plImageFormat::GetName(OutputImageFormat));

    plUInt32 uiTargetResolutionX = 0;
    plUInt32 uiTargetResolutionY = 0;

    PL_SUCCEED_OR_RETURN(DetermineTargetResolution(m_Descriptor.m_InputImages[0], OutputImageFormat, uiTargetResolutionX, uiTargetResolutionY));

    plLog::Info("Target resolution is '{} x {}'", uiTargetResolutionX, uiTargetResolutionY);

    PL_SUCCEED_OR_RETURN(ConvertAndScaleInputImages(uiTargetResolutionX, uiTargetResolutionY, m_Descriptor.m_Usage));

    PL_SUCCEED_OR_RETURN(ClampInputValues(m_Descriptor.m_InputImages, m_Descriptor.m_fMaxValue));

    if (m_Descriptor.m_Usage == plTexConvUsage::BumpMap)
    {
      PL_SUCCEED_OR_RETURN(ConvertToNormalMap(m_Descriptor.m_InputImages));
      m_Descriptor.m_Usage = plTexConvUsage::NormalMap;
    }

    plImage assembledImg;
    if (m_Descriptor.m_OutputType == plTexConvOutputType::Texture2D || m_Descriptor.m_OutputType == plTexConvOutputType::None)
    {
      PL_SUCCEED_OR_RETURN(Assemble2DTexture(m_Descriptor.m_InputImages[0].GetHeader(), assembledImg));

      PL_SUCCEED_OR_RETURN(InvertNormalMap(assembledImg));

      PL_SUCCEED_OR_RETURN(DilateColor2D(assembledImg));
    }
    else if (m_Descriptor.m_OutputType == plTexConvOutputType::Cubemap)
    {
      PL_SUCCEED_OR_RETURN(AssembleCubemap(assembledImg));
    }
    else if (m_Descriptor.m_OutputType == plTexConvOutputType::Volume)
    {
      PL_SUCCEED_OR_RETURN(Assemble3DTexture(assembledImg));
    }

    PL_SUCCEED_OR_RETURN(AdjustHdrExposure(assembledImg));

    PL_SUCCEED_OR_RETURN(GenerateMipmaps(assembledImg, 0, uiNumChannelsUsed == 1 ? MipmapChannelMode::SingleChannel : MipmapChannelMode::AllChannels));

    PL_SUCCEED_OR_RETURN(PremultiplyAlpha(assembledImg));

    PL_SUCCEED_OR_RETURN(GenerateOutput(std::move(assembledImg), m_OutputImage, OutputImageFormat));

    PL_SUCCEED_OR_RETURN(GenerateThumbnailOutput(m_OutputImage, m_ThumbnailOutputImage, m_Descriptor.m_uiThumbnailOutputResolution));

    PL_SUCCEED_OR_RETURN(GenerateLowResOutput(m_OutputImage, m_LowResOutputImage, m_Descriptor.m_uiLowResMipmaps));
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::DetectNumChannels(plArrayPtr<const plTexConvSliceChannelMapping> channelMapping, plUInt32& uiNumChannels)
{
  PL_PROFILE_SCOPE("DetectNumChannels");

  uiNumChannels = 0;

  for (const auto& mapping : channelMapping)
  {
    for (plUInt32 i = 0; i < 4; ++i)
    {
      if (mapping.m_Channel[i].m_iInputImageIndex != -1 || mapping.m_Channel[i].m_ChannelValue == plTexConvChannelValue::Black)
      {
        uiNumChannels = plMath::Max(uiNumChannels, i + 1);
      }
    }
  }

  if (uiNumChannels == 0)
  {
    plLog::Error("No proper channel mapping provided.");
    return PL_FAILURE;
  }

  // special case handling to detect when the alpha channel will end up white anyway and thus uiNumChannels could be 3 instead of 4
  // which enables us to use more optimized output formats
  if (uiNumChannels == 4)
  {
    uiNumChannels = 3;

    for (const auto& mapping : channelMapping)
    {
      if (mapping.m_Channel[3].m_ChannelValue == plTexConvChannelValue::Black)
      {
        // sampling a texture without an alpha channel always returns 1, so to use all 0, we do need the channel
        uiNumChannels = 4;
        return PL_SUCCESS;
      }

      if (mapping.m_Channel[3].m_iInputImageIndex == -1)
      {
        // no fourth channel is needed for this
        continue;
      }

      plImage& img = m_Descriptor.m_InputImages[mapping.m_Channel[3].m_iInputImageIndex];

      const plUInt32 uiNumRequiredChannels = (plUInt32)mapping.m_Channel[3].m_ChannelValue + 1;
      const plUInt32 uiNumActualChannels = plImageFormat::GetNumChannels(img.GetImageFormat());

      if (uiNumActualChannels < uiNumRequiredChannels)
      {
        // channel not available -> not needed
        continue;
      }

      if (img.Convert(plImageFormat::R32G32B32A32_FLOAT).Failed())
      {
        // can't convert -> will fail later anyway
        continue;
      }

      const float* pColors = img.GetPixelPointer<float>();
      pColors += (uiNumRequiredChannels - 1); // offset by 0 to 3 to read red, green, blue or alpha

      PL_ASSERT_DEV(img.GetRowPitch() == img.GetWidth() * sizeof(float) * 4, "Unexpected row pitch");

      for (plUInt32 i = 0; i < img.GetWidth() * img.GetHeight(); ++i)
      {
        if (!plMath::IsEqual(*pColors, 1.0f, 1.0f / 255.0f))
        {
          // value is not 1.0f -> the channel is needed
          uiNumChannels = 4;
          return PL_SUCCESS;
        }

        pColors += 4;
      }
    }
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::GenerateOutput(plImage&& src, plImage& dst, plEnum<plImageFormat> format)
{
  PL_PROFILE_SCOPE("GenerateOutput");

  dst.ResetAndMove(std::move(src));

  if (dst.Convert(format).Failed())
  {
    plLog::Error("Failed to convert result image to output format '{}'", plImageFormat::GetName(format));
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::GenerateThumbnailOutput(const plImage& srcImg, plImage& dstImg, plUInt32 uiTargetRes)
{
  if (uiTargetRes == 0)
    return PL_SUCCESS;

  PL_PROFILE_SCOPE("GenerateThumbnailOutput");

  plUInt32 uiBestMip = 0;

  for (plUInt32 m = 0; m < srcImg.GetNumMipLevels(); ++m)
  {
    if (srcImg.GetWidth(m) <= uiTargetRes && srcImg.GetHeight(m) <= uiTargetRes)
    {
      uiBestMip = m;
      break;
    }

    uiBestMip = m;
  }

  plImage scratch1, scratch2;
  plImage* pCurrentScratch = &scratch1;
  plImage* pOtherScratch = &scratch2;

  pCurrentScratch->ResetAndCopy(srcImg.GetSubImageView(uiBestMip, 0));

  if (pCurrentScratch->GetWidth() > uiTargetRes || pCurrentScratch->GetHeight() > uiTargetRes)
  {
    if (pCurrentScratch->GetWidth() > pCurrentScratch->GetHeight())
    {
      const float fAspectRatio = (float)pCurrentScratch->GetWidth() / (float)uiTargetRes;
      plUInt32 uiTargetHeight = (plUInt32)(pCurrentScratch->GetHeight() / fAspectRatio);

      uiTargetHeight = plMath::Max(uiTargetHeight, 4U);

      if (plImageUtils::Scale(*pCurrentScratch, *pOtherScratch, uiTargetRes, uiTargetHeight).Failed())
      {
        plLog::Error("Failed to resize thumbnail image from {}x{} to {}x{}", pCurrentScratch->GetWidth(), pCurrentScratch->GetHeight(), uiTargetRes,
          uiTargetHeight);
        return PL_FAILURE;
      }
    }
    else
    {
      const float fAspectRatio = (float)pCurrentScratch->GetHeight() / (float)uiTargetRes;
      plUInt32 uiTargetWidth = (plUInt32)(pCurrentScratch->GetWidth() / fAspectRatio);

      uiTargetWidth = plMath::Max(uiTargetWidth, 4U);

      if (plImageUtils::Scale(*pCurrentScratch, *pOtherScratch, uiTargetWidth, uiTargetRes).Failed())
      {
        plLog::Error("Failed to resize thumbnail image from {}x{} to {}x{}", pCurrentScratch->GetWidth(), pCurrentScratch->GetHeight(), uiTargetWidth,
          uiTargetRes);
        return PL_FAILURE;
      }
    }

    plMath::Swap(pCurrentScratch, pOtherScratch);
  }

  dstImg.ResetAndMove(std::move(*pCurrentScratch));

  // we want to write out the thumbnail unchanged, so make sure it has a non-sRGB format
  dstImg.ReinterpretAs(plImageFormat::AsLinear(dstImg.GetImageFormat()));

  if (dstImg.Convert(plImageFormat::R8G8B8A8_UNORM).Failed())
  {
    plLog::Error("Failed to convert thumbnail image to RGBA8.");
    return PL_FAILURE;
  }

  // generate alpha checkerboard pattern
  {
    const float fTileSize = 16.0f;

    plColorLinearUB* pPixels = dstImg.GetPixelPointer<plColorLinearUB>();
    const plUInt64 rowPitch = dstImg.GetRowPitch();

    plInt32 checkCounter = 0;
    plColor tiles[2]{plColor::LightGray, plColor::DarkGray};


    for (plUInt32 y = 0; y < dstImg.GetHeight(); ++y)
    {
      checkCounter = (plInt32)plMath::Floor(y / fTileSize);

      for (plUInt32 x = 0; x < dstImg.GetWidth(); ++x)
      {
        plColorLinearUB& col = pPixels[x];

        if (col.a < 255)
        {
          const plColor colF = col;
          const plInt32 tileIdx = (checkCounter + (plInt32)plMath::Floor(x / fTileSize)) % 2;

          col = plMath::Lerp(tiles[tileIdx], colF, plMath::Sqrt(colF.a)).WithAlpha(colF.a);
        }
      }

      pPixels = plMemoryUtils::AddByteOffset(pPixels, static_cast<ptrdiff_t>(rowPitch));
    }
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::GenerateLowResOutput(const plImage& srcImg, plImage& dstImg, plUInt32 uiLowResMip)
{
  if (uiLowResMip == 0)
    return PL_SUCCESS;

  PL_PROFILE_SCOPE("GenerateLowResOutput");

  // don't early out here in this case, otherwise external processes may consider the output to be incomplete
  // if (srcImg.GetNumMipLevels() <= uiLowResMip)
  //{
  //  // probably just a low-resolution input image, do not generate output, but also do not fail
  //  plLog::Warning("LowRes image not generated, original resolution is already below threshold.");
  //  return PL_SUCCESS;
  //}

  if (plImageUtils::ExtractLowerMipChain(srcImg, dstImg, uiLowResMip).Failed())
  {
    plLog::Error("Failed to extract low-res mipmap chain from output image.");
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}



PL_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_Processor);

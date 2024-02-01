#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/TexConv/TexConvProcessor.h>

plResult plTexConvProcessor::Assemble2DTexture(const plImageHeader& refImg, plImage& dst) const
{
  PL_PROFILE_SCOPE("Assemble2DTexture");

  dst.ResetAndAlloc(refImg);

  plColor* pPixelOut = dst.GetPixelPointer<plColor>();

  return Assemble2DSlice(m_Descriptor.m_ChannelMappings[0], refImg.GetWidth(), refImg.GetHeight(), pPixelOut);
}

plResult plTexConvProcessor::Assemble2DSlice(const plTexConvSliceChannelMapping& mapping, plUInt32 uiResolutionX, plUInt32 uiResolutionY, plColor* pPixelOut) const
{
  plHybridArray<const plColor*, 16> pSource;
  for (plUInt32 i = 0; i < m_Descriptor.m_InputImages.GetCount(); ++i)
  {
    pSource.ExpandAndGetRef() = m_Descriptor.m_InputImages[i].GetPixelPointer<plColor>();
  }

  const float fZero = 0.0f;
  const float fOne = 1.0f;
  const float* pSourceValues[4] = {nullptr, nullptr, nullptr, nullptr};
  plUInt32 uiSourceStrides[4] = {0, 0, 0, 0};

  for (plUInt32 channel = 0; channel < 4; ++channel)
  {
    const auto& cm = mapping.m_Channel[channel];
    const plInt32 inputIndex = cm.m_iInputImageIndex;

    if (inputIndex != -1)
    {
      const plColor* pSourcePixel = pSource[inputIndex];
      uiSourceStrides[channel] = 4;

      switch (cm.m_ChannelValue)
      {
        case plTexConvChannelValue::Red:
          pSourceValues[channel] = &pSourcePixel->r;
          break;
        case plTexConvChannelValue::Green:
          pSourceValues[channel] = &pSourcePixel->g;
          break;
        case plTexConvChannelValue::Blue:
          pSourceValues[channel] = &pSourcePixel->b;
          break;
        case plTexConvChannelValue::Alpha:
          pSourceValues[channel] = &pSourcePixel->a;
          break;

        default:
          PL_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }
    else
    {
      uiSourceStrides[channel] = 0; // because of the constant value

      switch (cm.m_ChannelValue)
      {
        case plTexConvChannelValue::Black:
          pSourceValues[channel] = &fZero;
          break;

        case plTexConvChannelValue::White:
          pSourceValues[channel] = &fOne;
          break;

        default:
          if (channel == 3)
            pSourceValues[channel] = &fOne;
          else
            pSourceValues[channel] = &fZero;
          break;
      }
    }
  }

  const bool bFlip = m_Descriptor.m_bFlipHorizontal;

  if (!bFlip && (pSourceValues[0] + 1 == pSourceValues[1]) && (pSourceValues[1] + 1 == pSourceValues[2]) &&
      (pSourceValues[2] + 1 == pSourceValues[3]))
  {
    PL_PROFILE_SCOPE("Assemble2DSlice(memcpy)");

    plMemoryUtils::Copy<plColor>(pPixelOut, reinterpret_cast<const plColor*>(pSourceValues[0]), uiResolutionX * uiResolutionY);
  }
  else
  {
    PL_PROFILE_SCOPE("Assemble2DSlice(gather)");

    for (plUInt32 y = 0; y < uiResolutionY; ++y)
    {
      const plUInt32 pixelWriteRowOffset = uiResolutionX * (bFlip ? (uiResolutionY - y - 1) : y);

      for (plUInt32 x = 0; x < uiResolutionX; ++x)
      {
        float* dst = &pPixelOut[pixelWriteRowOffset + x].r;

        for (plUInt32 c = 0; c < 4; ++c)
        {
          dst[c] = *pSourceValues[c];
          pSourceValues[c] += uiSourceStrides[c];
        }
      }
    }
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::DetermineTargetResolution(const plImage& image, plEnum<plImageFormat> OutputImageFormat, plUInt32& out_uiTargetResolutionX, plUInt32& out_uiTargetResolutionY) const
{
  PL_PROFILE_SCOPE("DetermineResolution");

  PL_ASSERT_DEV(out_uiTargetResolutionX == 0 && out_uiTargetResolutionY == 0, "Target resolution already determined");

  const plUInt32 uiOrgResX = image.GetWidth();
  const plUInt32 uiOrgResY = image.GetHeight();

  out_uiTargetResolutionX = uiOrgResX;
  out_uiTargetResolutionY = uiOrgResY;

  out_uiTargetResolutionX /= (1 << m_Descriptor.m_uiDownscaleSteps);
  out_uiTargetResolutionY /= (1 << m_Descriptor.m_uiDownscaleSteps);

  out_uiTargetResolutionX = plMath::Clamp(out_uiTargetResolutionX, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution);
  out_uiTargetResolutionY = plMath::Clamp(out_uiTargetResolutionY, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution);

  // keep original aspect ratio
  if (uiOrgResX > uiOrgResY)
  {
    out_uiTargetResolutionY = (out_uiTargetResolutionX * uiOrgResY) / uiOrgResX;
  }
  else if (uiOrgResX < uiOrgResY)
  {
    out_uiTargetResolutionX = (out_uiTargetResolutionY * uiOrgResX) / uiOrgResY;
  }

  if (m_Descriptor.m_OutputType == plTexConvOutputType::Volume)
  {
    plUInt32 uiScaleFactor = uiOrgResY / out_uiTargetResolutionY;
    out_uiTargetResolutionX = uiOrgResX / uiScaleFactor;
  }

  if (OutputImageFormat != plImageFormat::UNKNOWN && plImageFormat::RequiresFirstLevelBlockAlignment(OutputImageFormat))
  {
    const plUInt32 blockWidth = plImageFormat::GetBlockWidth(OutputImageFormat);

    plUInt32 currentWidth = out_uiTargetResolutionX;
    plUInt32 currentHeight = out_uiTargetResolutionY;
    bool issueWarning = false;

    if (out_uiTargetResolutionX % blockWidth != 0)
    {
      out_uiTargetResolutionX = plMath::RoundUp(out_uiTargetResolutionX, static_cast<plUInt16>(blockWidth));
      issueWarning = true;
    }

    plUInt32 blockHeight = plImageFormat::GetBlockHeight(OutputImageFormat);
    if (out_uiTargetResolutionY % blockHeight != 0)
    {
      out_uiTargetResolutionY = plMath::RoundUp(out_uiTargetResolutionY, static_cast<plUInt16>(blockHeight));
      issueWarning = true;
    }

    if (issueWarning)
    {
      plLog::Warning(
        "Chosen output image format is compressed, but target resolution does not fulfill block size requirements. {}x{} -> downscale {} / "
        "clamp({}, {}) -> {}x{}, adjusted to {}x{}",
        uiOrgResX, uiOrgResY, m_Descriptor.m_uiDownscaleSteps, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution, currentWidth,
        currentHeight, out_uiTargetResolutionX, out_uiTargetResolutionY);
    }
  }

  return PL_SUCCESS;
}



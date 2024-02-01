#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

plResult plTexConvProcessor::ForceSRGBFormats()
{
  // if the output is going to be sRGB, assume the incoming RGB data is also already in sRGB
  if (m_Descriptor.m_Usage == plTexConvUsage::Color)
  {
    for (const auto& mapping : m_Descriptor.m_ChannelMappings)
    {
      // do not enforce sRGB conversion for textures that are mapped to the alpha channel
      for (plUInt32 i = 0; i < 3; ++i)
      {
        const plInt32 iTex = mapping.m_Channel[i].m_iInputImageIndex;
        if (iTex != -1)
        {
          auto& img = m_Descriptor.m_InputImages[iTex];
          img.ReinterpretAs(plImageFormat::AsSrgb(img.GetImageFormat()));
        }
      }
    }
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::GenerateMipmaps(plImage& img, plUInt32 uiNumMips, MipmapChannelMode channelMode /*= MipmapChannelMode::AllChannels*/) const
{
  PL_PROFILE_SCOPE("GenerateMipmaps");

  plImageUtils::MipMapOptions opt;
  opt.m_numMipMaps = uiNumMips;

  plImageFilterBox filterLinear;
  plImageFilterSincWithKaiserWindow filterKaiser;

  switch (m_Descriptor.m_MipmapMode)
  {
    case plTexConvMipmapMode::None:
      return PL_SUCCESS;

    case plTexConvMipmapMode::Linear:
      opt.m_filter = &filterLinear;
      break;

    case plTexConvMipmapMode::Kaiser:
      opt.m_filter = &filterKaiser;
      break;
  }

  opt.m_addressModeU = m_Descriptor.m_AddressModeU;
  opt.m_addressModeV = m_Descriptor.m_AddressModeV;
  opt.m_addressModeW = m_Descriptor.m_AddressModeW;

  opt.m_preserveCoverage = m_Descriptor.m_bPreserveMipmapCoverage;
  opt.m_alphaThreshold = m_Descriptor.m_fMipmapAlphaThreshold;

  opt.m_renormalizeNormals = m_Descriptor.m_Usage == plTexConvUsage::NormalMap || m_Descriptor.m_Usage == plTexConvUsage::NormalMap_Inverted || m_Descriptor.m_Usage == plTexConvUsage::BumpMap;

  // Copy red to alpha channel if we only have a single channel input texture
  if (opt.m_preserveCoverage && channelMode == MipmapChannelMode::SingleChannel)
  {
    auto imgData = img.GetBlobPtr<plColor>();
    auto pData = imgData.GetPtr();
    while (pData < imgData.GetEndPtr())
    {
      pData->a = pData->r;
      ++pData;
    }
  }

  plImage scratch;
  plImageUtils::GenerateMipMaps(img, scratch, opt);
  img.ResetAndMove(std::move(scratch));

  if (img.GetNumMipLevels() <= 1)
  {
    plLog::Error("Mipmap generation failed.");
    return PL_FAILURE;
  }

  // Copy alpha channel back to red
  if (opt.m_preserveCoverage && channelMode == MipmapChannelMode::SingleChannel)
  {
    auto imgData = img.GetBlobPtr<plColor>();
    auto pData = imgData.GetPtr();
    while (pData < imgData.GetEndPtr())
    {
      pData->r = pData->a;
      ++pData;
    }
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::PremultiplyAlpha(plImage& image) const
{
  PL_PROFILE_SCOPE("PremultiplyAlpha");

  if (!m_Descriptor.m_bPremultiplyAlpha)
    return PL_SUCCESS;

  for (plColor& col : image.GetBlobPtr<plColor>())
  {
    col.r *= col.a;
    col.g *= col.a;
    col.b *= col.a;
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::AdjustHdrExposure(plImage& img) const
{
  PL_PROFILE_SCOPE("AdjustHdrExposure");

  plImageUtils::ChangeExposure(img, m_Descriptor.m_fHdrExposureBias);
  return PL_SUCCESS;
}

plResult plTexConvProcessor::ConvertToNormalMap(plArrayPtr<plImage> imgs) const
{
  PL_PROFILE_SCOPE("ConvertToNormalMap");

  for (plImage& img : imgs)
  {
    PL_SUCCEED_OR_RETURN(ConvertToNormalMap(img));
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::ConvertToNormalMap(plImage& bumpMap) const
{
  plImageHeader newImageHeader = bumpMap.GetHeader();
  newImageHeader.SetNumMipLevels(1);
  plImage newImage;
  newImage.ResetAndAlloc(newImageHeader);

  struct Accum
  {
    float x = 0.f;
    float y = 0.f;
  };
  plDelegate<Accum(plUInt32, plUInt32)> filterKernel;

  // we'll assume that both the input bump map and the new image are using
  // RGBA 32 bit floating point as an internal format which should be tightly packed
  PL_ASSERT_DEV(bumpMap.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT && bumpMap.GetRowPitch() % sizeof(plColor) == 0, "");

  const plColor* bumpPixels = bumpMap.GetPixelPointer<plColor>(0, 0, 0, 0, 0, 0);
  const auto getBumpPixel = [&](plUInt32 x, plUInt32 y) -> float
  {
    const plColor* ptr = bumpPixels + y * bumpMap.GetWidth() + x;
    return ptr->r;
  };

  plColor* newPixels = newImage.GetPixelPointer<plColor>(0, 0, 0, 0, 0, 0);
  auto getNewPixel = [&](plUInt32 x, plUInt32 y) -> plColor&
  {
    plColor* ptr = newPixels + y * newImage.GetWidth() + x;
    return *ptr;
  };

  switch (m_Descriptor.m_BumpMapFilter)
  {
    case plTexConvBumpMapFilter::Finite:
      filterKernel = [&](plUInt32 x, plUInt32 y)
      {
        constexpr float linearKernel[3] = {-1, 0, 1};

        Accum accum;
        for (int i = -1; i <= 1; ++i)
        {
          const plInt32 rx = plMath::Clamp(i + static_cast<plInt32>(x), 0, static_cast<plInt32>(newImage.GetWidth()) - 1);
          const plInt32 ry = plMath::Clamp(i + static_cast<plInt32>(y), 0, static_cast<plInt32>(newImage.GetHeight()) - 1);

          const float depthX = getBumpPixel(rx, y);
          const float depthY = getBumpPixel(x, ry);

          accum.x += depthX * linearKernel[i + 1];
          accum.y += depthY * linearKernel[i + 1];
        }

        return accum;
      };
      break;
    case plTexConvBumpMapFilter::Sobel:
      filterKernel = [&](plUInt32 x, plUInt32 y)
      {
        constexpr float kernel[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
        constexpr float weight = 1.f / 4.f;

        Accum accum;
        for (plInt32 i = -1; i <= 1; ++i)
        {
          for (plInt32 j = -1; j <= 1; ++j)
          {
            const plInt32 rx = plMath::Clamp(j + static_cast<plInt32>(x), 0, static_cast<plInt32>(newImage.GetWidth()) - 1);
            const plInt32 ry = plMath::Clamp(i + static_cast<plInt32>(y), 0, static_cast<plInt32>(newImage.GetHeight()) - 1);

            const float depth = getBumpPixel(rx, ry);

            accum.x += depth * kernel[i + 1][j + 1];
            accum.y += depth * kernel[j + 1][i + 1];
          }
        }

        accum.x *= weight;
        accum.y *= weight;

        return accum;
      };
      break;
    case plTexConvBumpMapFilter::Scharr:
      filterKernel = [&](plUInt32 x, plUInt32 y)
      {
        constexpr float kernel[3][3] = {{-3, 0, 3}, {-10, 0, 10}, {-3, 0, 3}};
        constexpr float weight = 1.f / 16.f;

        Accum accum;
        for (plInt32 i = -1; i <= 1; ++i)
        {
          for (plInt32 j = -1; j <= 1; ++j)
          {
            const plInt32 rx = plMath::Clamp(j + static_cast<plInt32>(x), 0, static_cast<plInt32>(newImage.GetWidth()) - 1);
            const plInt32 ry = plMath::Clamp(i + static_cast<plInt32>(y), 0, static_cast<plInt32>(newImage.GetHeight()) - 1);

            const float depth = getBumpPixel(rx, ry);

            accum.x += depth * kernel[i + 1][j + 1];
            accum.y += depth * kernel[j + 1][i + 1];
          }
        }

        accum.x *= weight;
        accum.y *= weight;

        return accum;
      };
      break;
  };

  for (plUInt32 y = 0; y < bumpMap.GetHeight(); ++y)
  {
    for (plUInt32 x = 0; x < bumpMap.GetWidth(); ++x)
    {
      Accum accum = filterKernel(x, y);

      plVec3 normal = plVec3(1.f, 0.f, accum.x).CrossRH(plVec3(0.f, 1.f, accum.y));
      normal.NormalizeIfNotZero(plVec3(0, 0, 1), 0.001f).IgnoreResult();
      normal.y = -normal.y;

      normal = normal * 0.5f + plVec3(0.5f);

      plColor& newPixel = getNewPixel(x, y);
      newPixel.SetRGBA(normal.x, normal.y, normal.z, 0.f);
    }
  }

  bumpMap.ResetAndMove(std::move(newImage));

  return PL_SUCCESS;
}

plResult plTexConvProcessor::ClampInputValues(plArrayPtr<plImage> images, float maxValue) const
{
  for (plImage& image : images)
  {
    PL_SUCCEED_OR_RETURN(ClampInputValues(image, maxValue));
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::ClampInputValues(plImage& image, float maxValue) const
{
  // we'll assume that at this point in the processing pipeline, the format is
  // RGBA32F which should result in tightly packed mipmaps.
  PL_ASSERT_DEV(image.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT && image.GetRowPitch() % sizeof(float[4]) == 0, "");

  for (auto& value : image.GetBlobPtr<float>())
  {
    if (plMath::IsNaN(value))
    {
      value = 0.f;
    }
    else
    {
      value = plMath::Clamp(value, -maxValue, maxValue);
    }
  }

  return PL_SUCCESS;
}

static bool FillAvgImageColor(plImage& ref_img)
{
  plColor avg = plColor::MakeZero();
  plUInt32 uiValidCount = 0;

  for (const plColor& col : ref_img.GetBlobPtr<plColor>())
  {
    if (col.a > 0.0f)
    {
      avg += col;
      ++uiValidCount;
    }
  }

  if (uiValidCount == 0 || uiValidCount == ref_img.GetBlobPtr<plColor>().GetCount())
  {
    // nothing to do
    return false;
  }

  avg /= static_cast<float>(uiValidCount);
  avg.NormalizeToLdrRange();
  avg.a = 0.0f;

  for (plColor& col : ref_img.GetBlobPtr<plColor>())
  {
    if (col.a == 0.0f)
    {
      col = avg;
    }
  }

  return true;
}

static void ClearAlpha(plImage& ref_img, float fAlphaThreshold)
{
  for (plColor& col : ref_img.GetBlobPtr<plColor>())
  {
    if (col.a <= fAlphaThreshold)
    {
      col.a = 0.0f;
    }
  }
}

inline static plColor GetPixelValue(const plColor* pPixels, plInt32 iWidth, plInt32 x, plInt32 y)
{
  return pPixels[y * iWidth + x];
}

inline static void SetPixelValue(plColor* pPixels, plInt32 iWidth, plInt32 x, plInt32 y, const plColor& col)
{
  pPixels[y * iWidth + x] = col;
}

static plColor GetAvgColor(plColor* pPixels, plInt32 iWidth, plInt32 iHeight, plInt32 x, plInt32 y, float fMarkAlpha)
{
  plColor colAt = GetPixelValue(pPixels, iWidth, x, y);

  if (colAt.a > 0)
    return colAt;

  plColor avg = plColor::MakeZero();
  plUInt32 uiValidCount = 0;

  const plInt32 iRadius = 1;

  for (plInt32 cy = plMath::Max<plInt32>(0, y - iRadius); cy <= plMath::Min<plInt32>(y + iRadius, iHeight - 1); ++cy)
  {
    for (plInt32 cx = plMath::Max<plInt32>(0, x - iRadius); cx <= plMath::Min<plInt32>(x + iRadius, iWidth - 1); ++cx)
    {
      const plColor col = GetPixelValue(pPixels, iWidth, cx, cy);

      if (col.a > fMarkAlpha)
      {
        avg += col;
        ++uiValidCount;
      }
    }
  }

  if (uiValidCount == 0)
    return colAt;

  avg /= static_cast<float>(uiValidCount);
  avg.a = fMarkAlpha;

  return avg;
}

static void DilateColors(plColor* pPixels, plInt32 iWidth, plInt32 iHeight, float fMarkAlpha)
{
  for (plInt32 y = 0; y < iHeight; ++y)
  {
    for (plInt32 x = 0; x < iWidth; ++x)
    {
      const plColor avg = GetAvgColor(pPixels, iWidth, iHeight, x, y, fMarkAlpha);

      SetPixelValue(pPixels, iWidth, x, y, avg);
    }
  }
}

plResult plTexConvProcessor::DilateColor2D(plImage& img) const
{
  if (m_Descriptor.m_uiDilateColor == 0)
    return PL_SUCCESS;

  PL_PROFILE_SCOPE("DilateColor2D");

  if (!FillAvgImageColor(img))
    return PL_SUCCESS;

  const plUInt32 uiNumPasses = m_Descriptor.m_uiDilateColor;

  plColor* pPixels = img.GetPixelPointer<plColor>();
  const plInt32 iWidth = static_cast<plInt32>(img.GetWidth());
  const plInt32 iHeight = static_cast<plInt32>(img.GetHeight());

  for (plUInt32 pass = uiNumPasses; pass > 0; --pass)
  {
    const float fAlphaThreshold = (static_cast<float>(pass) / uiNumPasses) / 256.0f; // between 0 and 1/256
    DilateColors(pPixels, iWidth, iHeight, fAlphaThreshold);
  }

  ClearAlpha(img, 1.0f / 256.0f);

  return PL_SUCCESS;
}

plResult plTexConvProcessor::InvertNormalMap(plImage& image)
{
  if (m_Descriptor.m_Usage != plTexConvUsage::NormalMap_Inverted)
    return PL_SUCCESS;

  // we'll assume that at this point in the processing pipeline, the format is
  // RGBA32F which should result in tightly packed mipmaps.
  PL_ASSERT_DEV(image.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT && image.GetRowPitch() % sizeof(float[4]) == 0, "");

  for (auto& value : image.GetBlobPtr<plColor>())
  {
    value.g = 1.0f - value.g;
  }

  return PL_SUCCESS;
}

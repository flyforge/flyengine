#include <Texture/TexturePCH.h>

#include <Texture/TexConv/TexConvProcessor.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

struct FileSuffixToUsage
{
  const char* m_szSuffix = nullptr;
  const plTexConvUsage::Enum m_Usage = plTexConvUsage::Auto;
};

static FileSuffixToUsage suffixToUsageMap[] = {
  //
  {"_d", plTexConvUsage::Color},       //
  {"diff", plTexConvUsage::Color},     //
  {"diffuse", plTexConvUsage::Color},  //
  {"albedo", plTexConvUsage::Color},   //
  {"col", plTexConvUsage::Color},      //
  {"color", plTexConvUsage::Color},    //
  {"emissive", plTexConvUsage::Color}, //
  {"emit", plTexConvUsage::Color},     //

  {"_n", plTexConvUsage::NormalMap},      //
  {"nrm", plTexConvUsage::NormalMap},     //
  {"norm", plTexConvUsage::NormalMap},    //
  {"normal", plTexConvUsage::NormalMap},  //
  {"normals", plTexConvUsage::NormalMap}, //

  {"_r", plTexConvUsage::Linear},        //
  {"_rgh", plTexConvUsage::Linear},      //
  {"_rough", plTexConvUsage::Linear},    //
  {"roughness", plTexConvUsage::Linear}, //

  {"_m", plTexConvUsage::Linear},       //
  {"_met", plTexConvUsage::Linear},     //
  {"_metal", plTexConvUsage::Linear},   //
  {"metallic", plTexConvUsage::Linear}, //

  {"_h", plTexConvUsage::Linear},     //
  {"height", plTexConvUsage::Linear}, //
  {"_disp", plTexConvUsage::Linear},  //

  {"_ao", plTexConvUsage::Linear},       //
  {"occlusion", plTexConvUsage::Linear}, //

  {"_alpha", plTexConvUsage::Linear}, //
};


static plTexConvUsage::Enum DetectUsageFromFilename(plStringView sFile)
{
  plStringBuilder name = plPathUtils::GetFileName(sFile);
  name.ToLower();

  for (plUInt32 i = 0; i < PL_ARRAY_SIZE(suffixToUsageMap); ++i)
  {
    if (name.EndsWith_NoCase(suffixToUsageMap[i].m_szSuffix))
    {
      return suffixToUsageMap[i].m_Usage;
    }
  }

  return plTexConvUsage::Auto;
}

static plTexConvUsage::Enum DetectUsageFromImage(const plImage& image)
{
  const plImageHeader& header = image.GetHeader();
  const plImageFormat::Enum format = header.GetImageFormat();

  if (header.GetDepth() > 1)
  {
    // unsupported
    return plTexConvUsage::Auto;
  }

  if (plImageFormat::IsSrgb(format))
  {
    // already sRGB so must be color
    return plTexConvUsage::Color;
  }

  if (format == plImageFormat::BC5_UNORM)
  {
    return plTexConvUsage::NormalMap;
  }

  if (plImageFormat::GetBitsPerChannel(format, plImageFormatChannel::R) > 8 || format == plImageFormat::BC6H_SF16 ||
      format == plImageFormat::BC6H_UF16)
  {
    return plTexConvUsage::Hdr;
  }

  if (plImageFormat::GetNumChannels(format) <= 2)
  {
    return plTexConvUsage::Linear;
  }

  const plImage* pImgRGBA = &image;
  plImage convertedRGBA;

  if (image.GetImageFormat() != plImageFormat::R8G8B8A8_UNORM)
  {
    pImgRGBA = &convertedRGBA;
    if (plImageConversion::Convert(image, convertedRGBA, plImageFormat::R8G8B8A8_UNORM).Failed())
    {
      // cannot convert to RGBA -> maybe some weird lookup table format
      return plTexConvUsage::Auto;
    }
  }

  // analyze the image content
  {
    plUInt32 sr = 0;
    plUInt32 sg = 0;
    plUInt32 sb = 0;

    plUInt32 uiExtremeNormals = 0;

    plUInt32 uiNumPixels = header.GetWidth() * header.GetHeight();
    PL_ASSERT_DEBUG(uiNumPixels > 0, "Unexpected empty image");

    // Sample no more than 10000 pixels
    plUInt32 uiStride = plMath::Max(1U, uiNumPixels / 10000);
    uiNumPixels /= uiStride;

    const plUInt8* pPixel = pImgRGBA->GetPixelPointer<plUInt8>();

    for (plUInt32 uiPixel = 0; uiPixel < uiNumPixels; ++uiPixel)
    {
      // definitely not a normal map, if any Z vector points that much backwards
      uiExtremeNormals += (pPixel[2] < 90) ? 1 : 0;

      sr += pPixel[0];
      sg += pPixel[1];
      sb += pPixel[2];

      pPixel += 4 * uiStride;
    }

    // the average color in the image
    sr /= uiNumPixels; // NOLINT: not a division by zero
    sg /= uiNumPixels; // NOLINT: not a division by zero
    sb /= uiNumPixels; // NOLINT: not a division by zero

    if (sb < 230 || sr < 128 - 60 || sr > 128 + 60 || sg < 128 - 60 || sg > 128 + 60)
    {
      // if the average color is not a proper hue of blue, it cannot be a normal map
      return plTexConvUsage::Color;
    }

    if (uiExtremeNormals > uiNumPixels / 100)
    {
      // more than 1 percent of normals pointing backwards ? => probably not a normalmap
      return plTexConvUsage::Color;
    }

    // it might just be a normal map, it does have the proper hue of blue
    return plTexConvUsage::NormalMap;
  }
}

plResult plTexConvProcessor::AdjustUsage(plStringView sFilename, const plImage& srcImg, plEnum<plTexConvUsage>& inout_Usage)
{
  PL_PROFILE_SCOPE("AdjustUsage");

  if (inout_Usage == plTexConvUsage::Auto)
  {
    inout_Usage = DetectUsageFromFilename(sFilename);
  }

  if (inout_Usage == plTexConvUsage::Auto)
  {
    inout_Usage = DetectUsageFromImage(srcImg);
  }

  if (inout_Usage == plTexConvUsage::Auto)
  {
    plLog::Error("Failed to deduce target format.");
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}



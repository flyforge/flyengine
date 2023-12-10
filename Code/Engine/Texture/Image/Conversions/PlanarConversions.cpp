#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageConversion.h>

namespace
{
  // https://docs.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering#converting-8-bit-yuv-to-rgb888
  plVec3I32 RGB2YUV(plVec3I32 rgb)
  {
    plVec3I32 yuv;
    yuv.x = ((66 * rgb.x + 129 * rgb.y + 25 * rgb.z + 128) >> 8) + 16;
    yuv.y = ((-38 * rgb.x - 74 * rgb.y + 112 * rgb.z + 128) >> 8) + 128;
    yuv.z = ((112 * rgb.x - 94 * rgb.y - 18 * rgb.z + 128) >> 8) + 128;
    return yuv;
  }

  plVec3I32 YUV2RGB(plVec3I32 yuv)
  {
    plVec3I32 rgb;

    plInt32 C = yuv.x - 16;
    plInt32 D = yuv.y - 128;
    plInt32 E = yuv.z - 128;

    rgb.x = plMath::Clamp((298 * C + 409 * E + 128) >> 8, 0, 255);
    rgb.y = plMath::Clamp((298 * C - 100 * D - 208 * E + 128) >> 8, 0, 255);
    rgb.z = plMath::Clamp((298 * C + 516 * D + 128) >> 8, 0, 255);
    return rgb;
  }
} // namespace

struct plImageConversion_NV12_sRGB : public plImageConversionStepDeplanarize
{
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::NV12, plImageFormat::R8G8B8A8_UNORM_SRGB, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plArrayPtr<plImageView> source, plImage target, plUInt32 numPixelsX, plUInt32 numPixelsY, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    for (plUInt32 y = 0; y < numPixelsY; y += 2)
    {
      const plUInt8* luma0 = source[0].GetPixelPointer<plUInt8>(0, 0, 0, 0, y);
      const plUInt8* luma1 = source[0].GetPixelPointer<plUInt8>(0, 0, 0, 0, y + 1);
      const plUInt8* chroma = source[1].GetPixelPointer<plUInt8>(0, 0, 0, 0, y / 2);

      plUInt8* rgba0 = target.GetPixelPointer<plUInt8>(0, 0, 0, 0, y);
      plUInt8* rgba1 = target.GetPixelPointer<plUInt8>(0, 0, 0, 0, y + 1);

      for (plUInt32 x = 0; x < numPixelsX; x += 2)
      {
        plVec3I32 p00 = YUV2RGB(plVec3I32(luma0[0], chroma[0], chroma[1]));
        plVec3I32 p01 = YUV2RGB(plVec3I32(luma0[1], chroma[0], chroma[1]));
        plVec3I32 p10 = YUV2RGB(plVec3I32(luma1[0], chroma[0], chroma[1]));
        plVec3I32 p11 = YUV2RGB(plVec3I32(luma1[1], chroma[0], chroma[1]));

        rgba0[0] = static_cast<plUInt8>(p00.x);
        rgba0[1] = static_cast<plUInt8>(p00.y);
        rgba0[2] = static_cast<plUInt8>(p00.z);
        rgba0[3] = static_cast<plUInt8>(0xff);
        rgba0[4] = static_cast<plUInt8>(p01.x);
        rgba0[5] = static_cast<plUInt8>(p01.y);
        rgba0[6] = static_cast<plUInt8>(p01.z);
        rgba0[7] = static_cast<plUInt8>(0xff);

        rgba1[0] = static_cast<plUInt8>(p10.x);
        rgba1[1] = static_cast<plUInt8>(p10.y);
        rgba1[2] = static_cast<plUInt8>(p10.z);
        rgba1[3] = static_cast<plUInt8>(0xff);
        rgba1[4] = static_cast<plUInt8>(p11.x);
        rgba1[5] = static_cast<plUInt8>(p11.y);
        rgba1[6] = static_cast<plUInt8>(p11.z);
        rgba1[7] = static_cast<plUInt8>(0xff);

        luma0 += 2;
        luma1 += 2;
        chroma += 2;

        rgba0 += 8;
        rgba1 += 8;
      }
    }

    return PLASMA_SUCCESS;
  }
};

struct plImageConversion_sRGB_NV12 : public plImageConversionStepPlanarize
{
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM_SRGB, plImageFormat::NV12, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(const plImageView& source, plArrayPtr<plImage> target, plUInt32 numPixelsX, plUInt32 numPixelsY, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    for (plUInt32 y = 0; y < numPixelsY; y += 2)
    {
      const plUInt8* rgba0 = source.GetPixelPointer<plUInt8>(0, 0, 0, 0, y);
      const plUInt8* rgba1 = source.GetPixelPointer<plUInt8>(0, 0, 0, 0, y + 1);

      plUInt8* luma0 = target[0].GetPixelPointer<plUInt8>(0, 0, 0, 0, y);
      plUInt8* luma1 = target[0].GetPixelPointer<plUInt8>(0, 0, 0, 0, y + 1);
      plUInt8* chroma = target[1].GetPixelPointer<plUInt8>(0, 0, 0, 0, y / 2);

      for (plUInt32 x = 0; x < numPixelsX; x += 2)
      {
        plVec3I32 p00 = RGB2YUV(plVec3I32(rgba0[0], rgba0[1], rgba0[2]));
        plVec3I32 p01 = RGB2YUV(plVec3I32(rgba0[4], rgba0[5], rgba0[6]));
        plVec3I32 p10 = RGB2YUV(plVec3I32(rgba1[0], rgba1[1], rgba1[2]));
        plVec3I32 p11 = RGB2YUV(plVec3I32(rgba1[4], rgba1[5], rgba1[6]));

        luma0[0] = static_cast<plUInt8>(p00.x);
        luma0[1] = static_cast<plUInt8>(p01.x);
        luma1[0] = static_cast<plUInt8>(p10.x);
        luma1[1] = static_cast<plUInt8>(p11.x);

        plVec3I32 c = (p00 + p01 + p10 + p11);

        chroma[0] = static_cast<plUInt8>(c.y >> 2);
        chroma[1] = static_cast<plUInt8>(c.z >> 2);

        luma0 += 2;
        luma1 += 2;
        chroma += 2;

        rgba0 += 8;
        rgba1 += 8;
      }
    }

    return PLASMA_SUCCESS;
  }
};

static plImageConversion_NV12_sRGB s_conversion_NV12_sRGB;
static plImageConversion_sRGB_NV12 s_conversion_sRGB_NV12;

PLASMA_STATICLINK_FILE(Texture, Texture_Image_Conversions_PlanarConversions);

#include <Texture/TexturePCH.h>

#include <Foundation/Math/Float16.h>
#include <Texture/Image/Conversions/PixelConversions.h>
#include <Texture/Image/ImageConversion.h>

#if PL_SIMD_IMPLEMENTATION == PL_SIMD_IMPLEMENTATION_SSE && PL_SSE_LEVEL >= PL_SSE_20
#  include <emmintrin.h>
#endif

#if PL_SIMD_IMPLEMENTATION == PL_SIMD_IMPLEMENTATION_SSE && PL_SSE_LEVEL >= PL_SSE_30
#  include <tmmintrin.h>
#endif

namespace
{
  // 3D vector: 11/11/10 floating-point components
  // The 3D vector is packed into 32 bits as follows: a 5-bit biased exponent
  // and 6-bit mantissa for x component, a 5-bit biased exponent and
  // 6-bit mantissa for y component, a 5-bit biased exponent and a 5-bit
  // mantissa for z. The z component is stored in the most significant bits
  // and the x component in the least significant bits. No sign bits so
  // all partial-precision numbers are positive.
  // (Z10Y11X11): [32] ZZZZZzzz zzzYYYYY yyyyyyXX XXXxxxxx [0
  union R11G11B10
  {
    struct Parts
    {
      plUInt32 xm : 6; // x-mantissa
      plUInt32 xe : 5; // x-exponent
      plUInt32 ym : 6; // y-mantissa
      plUInt32 ye : 5; // y-exponent
      plUInt32 zm : 5; // z-mantissa
      plUInt32 ze : 5; // z-exponent
    } p;
    plUInt32 v;
  };
} // namespace

plColorBaseUB plDecompressA4B4G4R4(plUInt16 uiColor)
{
  plColorBaseUB result;
  result.r = ((uiColor & 0xF000u) * 17) >> 12;
  result.g = ((uiColor & 0x0F00u) * 17) >> 8;
  result.b = ((uiColor & 0x00F0u) * 17) >> 4;
  result.a = ((uiColor & 0x000Fu) * 17);
  return result;
}

plUInt16 plCompressA4B4G4R4(plColorBaseUB color)
{
  plUInt32 r = (color.r * 15 + 135) >> 8;
  plUInt32 g = (color.g * 15 + 135) >> 8;
  plUInt32 b = (color.b * 15 + 135) >> 8;
  plUInt32 a = (color.a * 15 + 135) >> 8;
  return static_cast<plUInt16>((r << 12) | (g << 8) | (b << 4) | a);
}

plColorBaseUB plDecompressB4G4R4A4(plUInt16 uiColor)
{
  plColorBaseUB result;
  result.r = ((uiColor & 0x0F00u) * 17) >> 8;
  result.g = ((uiColor & 0x00F0u) * 17) >> 4;
  result.b = ((uiColor & 0x000Fu) * 17);
  result.a = ((uiColor & 0xF000u) * 17) >> 12;
  return result;
}

plUInt16 plCompressB4G4R4A4(plColorBaseUB color)
{
  plUInt32 r = (color.r * 15 + 135) >> 8;
  plUInt32 g = (color.g * 15 + 135) >> 8;
  plUInt32 b = (color.b * 15 + 135) >> 8;
  plUInt32 a = (color.a * 15 + 135) >> 8;
  return static_cast<plUInt16>((a << 12) | (r << 8) | (g << 4) | b);
}

plColorBaseUB plDecompressB5G6R5(plUInt16 uiColor)
{
  plColorBaseUB result;
  result.r = static_cast<plUInt8>(((uiColor & 0xF800u) * 527 + 47104) >> 17);
  result.g = static_cast<plUInt8>(((uiColor & 0x07E0u) * 259 + 1056) >> 11);
  result.b = static_cast<plUInt8>(((uiColor & 0x001Fu) * 527 + 23) >> 6);
  result.a = 0xFF;

  return result;
}

plUInt16 plCompressB5G6R5(plColorBaseUB color)
{
  plUInt32 r = (color.r * 249 + 1024) >> 11;
  plUInt32 g = (color.g * 253 + 512) >> 10;
  plUInt32 b = (color.b * 249 + 1024) >> 11;
  return static_cast<plUInt16>((r << 11) | (g << 5) | b);
}

plColorBaseUB plDecompressB5G5R5X1(plUInt16 uiColor)
{
  plColorBaseUB result;
  result.r = static_cast<plUInt8>(((uiColor & 0x7C00u) * 527 + 23552) >> 16);
  result.g = static_cast<plUInt8>(((uiColor & 0x03E0u) * 527 + 736) >> 11);
  result.b = static_cast<plUInt8>(((uiColor & 0x001Fu) * 527 + 23) >> 6);
  result.a = 0xFF;
  return result;
}

plUInt16 plCompressB5G5R5X1(plColorBaseUB color)
{
  plUInt32 r = (color.r * 249 + 1024) >> 11;
  plUInt32 g = (color.g * 249 + 1024) >> 11;
  plUInt32 b = (color.b * 249 + 1024) >> 11;
  return static_cast<plUInt16>((1 << 15) | (r << 10) | (g << 5) | b);
}

plColorBaseUB plDecompressB5G5R5A1(plUInt16 uiColor)
{
  plColorBaseUB result;
  result.r = static_cast<plUInt8>(((uiColor & 0x7C00u) * 527 + 23552) >> 16);
  result.g = static_cast<plUInt8>(((uiColor & 0x03E0u) * 527 + 736) >> 11);
  result.b = static_cast<plUInt8>(((uiColor & 0x001Fu) * 527 + 23) >> 6);
  result.a = static_cast<plUInt8>(((uiColor & 0x8000u) * 255) >> 15);
  return result;
}

plUInt16 plCompressB5G5R5A1(plColorBaseUB color)
{
  plUInt32 r = (color.r * 249 + 1024) >> 11;
  plUInt32 g = (color.g * 249 + 1024) >> 11;
  plUInt32 b = (color.b * 249 + 1024) >> 11;
  plUInt32 a = (color.a) >> 7;
  return static_cast<plUInt16>((a << 15) | (r << 10) | (g << 5) | b);
}

plColorBaseUB plDecompressX1B5G5R5(plUInt16 uiColor)
{
  plColorBaseUB result;
  result.r = static_cast<plUInt8>(((uiColor & 0xF800u) * 527 + 23552) >> 17);
  result.g = static_cast<plUInt8>(((uiColor & 0x07C0u) * 527 + 736) >> 12);
  result.b = static_cast<plUInt8>(((uiColor & 0x003Eu) * 527 + 23) >> 7);
  result.a = 0xFF;
  return result;
}

plUInt16 plCompressX1B5G5R5(plColorBaseUB color)
{
  plUInt32 r = (color.r * 249 + 1024) >> 11;
  plUInt32 g = (color.g * 249 + 1024) >> 11;
  plUInt32 b = (color.b * 249 + 1024) >> 11;
  return static_cast<plUInt16>((r << 11) | (g << 6) | (b << 1) | 1);
}

plColorBaseUB plDecompressA1B5G5R5(plUInt16 uiColor)
{
  plColorBaseUB result;
  result.r = static_cast<plUInt8>(((uiColor & 0xF800u) * 527 + 23552) >> 17);
  result.g = static_cast<plUInt8>(((uiColor & 0x07C0u) * 527 + 736) >> 12);
  result.b = static_cast<plUInt8>(((uiColor & 0x003Eu) * 527 + 23) >> 7);
  result.a = static_cast<plUInt8>((uiColor & 0x0001u) * 255);
  return result;
}

plUInt16 plCompressA1B5G5R5(plColorBaseUB color)
{
  plUInt32 r = (color.r * 249 + 1024) >> 11;
  plUInt32 g = (color.g * 249 + 1024) >> 11;
  plUInt32 b = (color.b * 249 + 1024) >> 11;
  plUInt32 a = color.a >> 7;
  return static_cast<plUInt16>((r << 11) | (g << 6) | (b << 1) | a);
}

template <plColorBaseUB (*decompressFunc)(plUInt16), plImageFormat::Enum templateSourceFormat>
class plImageConversionStep_Decompress16bpp : plImageConversionStepLinear
{
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    plImageFormat::Enum sourceFormatSrgb = plImageFormat::AsSrgb(templateSourceFormat);
    PL_ASSERT_DEV(
      sourceFormatSrgb != templateSourceFormat, "Format '%s' should have a corresponding sRGB format", plImageFormat::GetName(templateSourceFormat));

    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(templateSourceFormat, plImageFormat::R8G8B8A8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(sourceFormatSrgb, plImageFormat::R8G8B8A8_UNORM_SRGB, plImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 numElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    plUInt32 sourceStride = 2;
    plUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      *reinterpret_cast<plColorBaseUB*>(targetPointer) = decompressFunc(*reinterpret_cast<const plUInt16*>(sourcePointer));

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return PL_SUCCESS;
  }
};

template <plUInt16 (*compressFunc)(plColorBaseUB), plImageFormat::Enum templateTargetFormat>
class plImageConversionStep_Compress16bpp : plImageConversionStepLinear
{
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    plImageFormat::Enum targetFormatSrgb = plImageFormat::AsSrgb(templateTargetFormat);
    PL_ASSERT_DEV(
      targetFormatSrgb != templateTargetFormat, "Format '%s' should have a corresponding sRGB format", plImageFormat::GetName(templateTargetFormat));

    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM, templateTargetFormat, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM_SRGB, targetFormatSrgb, plImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 numElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    plUInt32 sourceStride = 4;
    plUInt32 targetStride = 2;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      *reinterpret_cast<plUInt16*>(targetPointer) = compressFunc(*reinterpret_cast<const plColorBaseUB*>(sourcePointer));

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return PL_SUCCESS;
  }
};

#if PL_SIMD_IMPLEMENTATION == PL_SIMD_IMPLEMENTATION_SSE

static bool IsAligned(const void* pPointer)
{
  return reinterpret_cast<size_t>(pPointer) % 16 == 0;
}

#endif

struct plImageSwizzleConversion32_2103 : public plImageConversionStepLinear
{
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::B8G8R8A8_UNORM, plImageFormat::R8G8B8A8_UNORM, plImageConversionFlags::InPlace),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM, plImageFormat::B8G8R8A8_UNORM, plImageConversionFlags::InPlace),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM, plImageFormat::B8G8R8X8_UNORM, plImageConversionFlags::InPlace),
      plImageConversionEntry(plImageFormat::B8G8R8A8_UNORM_SRGB, plImageFormat::R8G8B8A8_UNORM_SRGB, plImageConversionFlags::InPlace),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM_SRGB, plImageFormat::B8G8R8A8_UNORM_SRGB, plImageConversionFlags::InPlace),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM_SRGB, plImageFormat::B8G8R8X8_UNORM_SRGB, plImageConversionFlags::InPlace),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    plUInt32 sourceStride = 4;
    plUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

#if PL_SIMD_IMPLEMENTATION == PL_SIMD_IMPLEMENTATION_SSE
    if (IsAligned(sourcePointer) && IsAligned(targetPointer))
    {
#  if PL_SSE_LEVEL >= PL_SSE_30
      const plUInt32 elementsPerBatch = 8;

      __m128i shuffleMask = _mm_set_epi8(15, 12, 13, 14, 11, 8, 9, 10, 7, 4, 5, 6, 3, 0, 1, 2);

      // Intel optimization manual, Color Pixel Format Conversion Using SSE3
      while (uiNumElements >= elementsPerBatch)
      {
        __m128i in0 = reinterpret_cast<const __m128i*>(sourcePointer)[0];
        __m128i in1 = reinterpret_cast<const __m128i*>(sourcePointer)[1];

        reinterpret_cast<__m128i*>(targetPointer)[0] = _mm_shuffle_epi8(in0, shuffleMask);
        reinterpret_cast<__m128i*>(targetPointer)[1] = _mm_shuffle_epi8(in1, shuffleMask);

        sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        uiNumElements -= elementsPerBatch;
      }
#  else
      const plUInt32 elementsPerBatch = 8;

      __m128i mask1 = _mm_set1_epi32(0xff00ff00);
      __m128i mask2 = _mm_set1_epi32(0x00ff00ff);

      // Intel optimization manual, Color Pixel Format Conversion Using SSE2
      while (numElements >= elementsPerBatch)
      {
        __m128i in0 = reinterpret_cast<const __m128i*>(sourcePointer)[0];
        __m128i in1 = reinterpret_cast<const __m128i*>(sourcePointer)[1];

        reinterpret_cast<__m128i*>(targetPointer)[0] =
          _mm_or_si128(_mm_and_si128(in0, mask1), _mm_and_si128(_mm_or_si128(_mm_slli_epi32(in0, 16), _mm_srli_epi32(in0, 16)), mask2));
        reinterpret_cast<__m128i*>(targetPointer)[1] =
          _mm_or_si128(_mm_and_si128(in1, mask1), _mm_and_si128(_mm_or_si128(_mm_slli_epi32(in1, 16), _mm_srli_epi32(in1, 16)), mask2));

        sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        numElements -= elementsPerBatch;
      }
#  endif
    }
#endif

    while (uiNumElements)
    {
      plUInt8 a, b, c, d;
      a = reinterpret_cast<const plUInt8*>(sourcePointer)[2];
      b = reinterpret_cast<const plUInt8*>(sourcePointer)[1];
      c = reinterpret_cast<const plUInt8*>(sourcePointer)[0];
      d = reinterpret_cast<const plUInt8*>(sourcePointer)[3];
      reinterpret_cast<plUInt8*>(targetPointer)[0] = a;
      reinterpret_cast<plUInt8*>(targetPointer)[1] = b;
      reinterpret_cast<plUInt8*>(targetPointer)[2] = c;
      reinterpret_cast<plUInt8*>(targetPointer)[3] = d;

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

struct plImageConversion_BGRX_BGRA : public plImageConversionStepLinear
{
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      {plImageFormat::B8G8R8X8_UNORM, plImageFormat::B8G8R8A8_UNORM, plImageConversionFlags::InPlace},
      {plImageFormat::B8G8R8X8_UNORM_SRGB, plImageFormat::B8G8R8A8_UNORM_SRGB, plImageConversionFlags::InPlace},
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    plUInt32 sourceStride = 4;
    plUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

#if PL_SIMD_IMPLEMENTATION == PL_SIMD_IMPLEMENTATION_SSE && PL_SSE_LEVEL >= PL_SSE_20
    if (IsAligned(sourcePointer) && IsAligned(targetPointer))
    {
      const plUInt32 elementsPerBatch = 4;

      __m128i mask = _mm_set1_epi32(0xFF000000);

      while (uiNumElements >= elementsPerBatch)
      {
        const __m128i* pSource = reinterpret_cast<const __m128i*>(sourcePointer);
        __m128i* pTarget = reinterpret_cast<__m128i*>(targetPointer);

        pTarget[0] = _mm_or_si128(pSource[0], mask);

        sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        uiNumElements -= elementsPerBatch;
      }
    }
#endif

    while (uiNumElements)
    {
      plUInt32 x = *(reinterpret_cast<const plUInt32*>(sourcePointer));

#if PL_ENABLED(PL_PLATFORM_LITTLE_ENDIAN)
      x |= 0xFF000000;
#else
      x |= 0x000000FF;
#endif

      *(reinterpret_cast<plUInt32*>(targetPointer)) = x;

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_F32_U8 : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R32_FLOAT, plImageFormat::R8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32_FLOAT, plImageFormat::R8G8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32_FLOAT, plImageFormat::R8G8B8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32A32_FLOAT, plImageFormat::R8G8B8A8_UNORM, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= plImageFormat::GetBitsPerPixel(targetFormat) / 8;

    plUInt32 sourceStride = 4;
    plUInt32 targetStride = 1;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

#if PL_SIMD_IMPLEMENTATION == PL_SIMD_IMPLEMENTATION_SSE && PL_SSE_LEVEL >= PL_SSE_20
    {
      const plUInt32 elementsPerBatch = 16;

      __m128 zero = _mm_setzero_ps();
      __m128 one = _mm_set1_ps(1.0f);
      __m128 scale = _mm_set1_ps(255.0f);
      __m128 half = _mm_set1_ps(0.5f);

      while (uiNumElements >= elementsPerBatch)
      {
        __m128 float0 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 0);
        __m128 float1 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 4);
        __m128 float2 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 8);
        __m128 float3 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 12);

        // Clamp NaN to zero
        float0 = _mm_and_ps(_mm_cmpord_ps(float0, zero), float0);
        float1 = _mm_and_ps(_mm_cmpord_ps(float1, zero), float1);
        float2 = _mm_and_ps(_mm_cmpord_ps(float2, zero), float2);
        float3 = _mm_and_ps(_mm_cmpord_ps(float3, zero), float3);

        // Saturate
        float0 = _mm_max_ps(zero, _mm_min_ps(one, float0));
        float1 = _mm_max_ps(zero, _mm_min_ps(one, float1));
        float2 = _mm_max_ps(zero, _mm_min_ps(one, float2));
        float3 = _mm_max_ps(zero, _mm_min_ps(one, float3));

        float0 = _mm_mul_ps(float0, scale);
        float1 = _mm_mul_ps(float1, scale);
        float2 = _mm_mul_ps(float2, scale);
        float3 = _mm_mul_ps(float3, scale);

        // Add 0.5f and truncate for rounding as required by D3D spec
        float0 = _mm_add_ps(float0, half);
        float1 = _mm_add_ps(float1, half);
        float2 = _mm_add_ps(float2, half);
        float3 = _mm_add_ps(float3, half);

        __m128i int0 = _mm_cvttps_epi32(float0);
        __m128i int1 = _mm_cvttps_epi32(float1);
        __m128i int2 = _mm_cvttps_epi32(float2);
        __m128i int3 = _mm_cvttps_epi32(float3);

        __m128i short0 = _mm_packs_epi32(int0, int1);
        __m128i short1 = _mm_packs_epi32(int2, int3);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(targetPointer), _mm_packus_epi16(short0, short1));

        sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        uiNumElements -= elementsPerBatch;
      }
    }
#endif

    while (uiNumElements)
    {

      *reinterpret_cast<plUInt8*>(targetPointer) = plMath::ColorFloatToByte(*reinterpret_cast<const float*>(sourcePointer));

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_F32_sRGB : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R32G32B32A32_FLOAT, plImageFormat::R8G8B8A8_UNORM_SRGB, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    plUInt32 sourceStride = 16;
    plUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<plColorGammaUB*>(targetPointer) = *reinterpret_cast<const plColor*>(sourcePointer);

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_F32_U16 : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R32_FLOAT, plImageFormat::R16_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32_FLOAT, plImageFormat::R16G16_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32_FLOAT, plImageFormat::R16G16B16_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32A32_FLOAT, plImageFormat::R16G16B16A16_UNORM, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= plImageFormat::GetBitsPerPixel(targetFormat) / 16;

    plUInt32 sourceStride = 4;
    plUInt32 targetStride = 2;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {

      *reinterpret_cast<plUInt16*>(targetPointer) = plMath::ColorFloatToShort(*reinterpret_cast<const float*>(sourcePointer));

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_F32_F16 : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R32_FLOAT, plImageFormat::R16_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32_FLOAT, plImageFormat::R16G16_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32A32_FLOAT, plImageFormat::R16G16B16A16_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= plImageFormat::GetBitsPerPixel(targetFormat) / 16;

    plUInt32 sourceStride = 4;
    plUInt32 targetStride = 2;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {

      *reinterpret_cast<plFloat16*>(targetPointer) = *reinterpret_cast<const float*>(sourcePointer);

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_F32_S8 : public plImageConversionStepLinear
{
public:
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R32_FLOAT, plImageFormat::R8_SNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32_FLOAT, plImageFormat::R8G8_SNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32A32_FLOAT, plImageFormat::R8G8B8A8_SNORM, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= plImageFormat::GetBitsPerPixel(targetFormat) / 8;

    plUInt32 sourceStride = 4;
    plUInt32 targetStride = 1;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {

      *reinterpret_cast<plInt8*>(targetPointer) = plMath::ColorFloatToSignedByte(*reinterpret_cast<const float*>(sourcePointer));

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_U8_F32 : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R8_UNORM, plImageFormat::R32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8_UNORM, plImageFormat::R32G32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8_UNORM, plImageFormat::R32G32B32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= plImageFormat::GetBitsPerPixel(targetFormat) / 32;

    plUInt32 sourceStride = 1;
    plUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = plMath::ColorByteToFloat(*reinterpret_cast<const plUInt8*>(sourcePointer));

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_sRGB_F32 : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM_SRGB, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    plUInt32 sourceStride = 4;
    plUInt32 targetStride = 16;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<plColor*>(targetPointer) = *reinterpret_cast<const plColorGammaUB*>(sourcePointer);

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_U16_F32 : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R16_UNORM, plImageFormat::R32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16_UNORM, plImageFormat::R32G32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16_UNORM, plImageFormat::R32G32B32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_UNORM, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= plImageFormat::GetBitsPerPixel(targetFormat) / 32;

    plUInt32 sourceStride = 2;
    plUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = plMath::ColorShortToFloat(*reinterpret_cast<const plUInt16*>(sourcePointer));

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_S16_F32 : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R16_SNORM, plImageFormat::R32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16_SNORM, plImageFormat::R32G32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_SNORM, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= plImageFormat::GetBitsPerPixel(targetFormat) / 32;

    plUInt32 sourceStride = 2;
    plUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = plMath::ColorSignedShortToFloat(*reinterpret_cast<const plInt16*>(sourcePointer));

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_F16_F32 : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R16_FLOAT, plImageFormat::R32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16_FLOAT, plImageFormat::R32G32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_FLOAT, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= plImageFormat::GetBitsPerPixel(targetFormat) / 32;

    plUInt32 sourceStride = 2;
    plUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = *reinterpret_cast<const plFloat16*>(sourcePointer);

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_S8_F32 : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R8_SNORM, plImageFormat::R32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8_SNORM, plImageFormat::R32G32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_SNORM, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= plImageFormat::GetBitsPerPixel(targetFormat) / 32;

    plUInt32 sourceStride = 1;
    plUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = plMath::ColorSignedByteToFloat(*reinterpret_cast<const plInt8*>(sourcePointer));

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

struct plImageConversion_Pad_To_RGBA_U8 : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R8_UNORM, plImageFormat::R8G8B8A8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8_UNORM, plImageFormat::R8G8B8A8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8_UNORM, plImageFormat::R8G8B8A8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8_UNORM_SRGB, plImageFormat::R8G8B8A8_UNORM_SRGB, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::B8G8R8_UNORM, plImageFormat::B8G8R8A8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::B8G8R8_UNORM_SRGB, plImageFormat::B8G8R8A8_UNORM_SRGB, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    plUInt32 sourceStride = plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    plUInt32 targetStride = plImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const plUInt8* sourcePointer = static_cast<const plUInt8*>(source.GetPtr());
    plUInt8* targetPointer = static_cast<plUInt8*>(target.GetPtr());

    const plUInt32 numChannels = sourceStride / sizeof(plUInt8);

#if PL_ENABLED(PL_PLATFORM_LITTLE_ENDIAN)
    if (numChannels == 3)
    {
      // Fast path for RGB -> RGBA
      const plUInt32 elementsPerBatch = 4;

      while (uiNumElements >= elementsPerBatch)
      {
        plUInt32 source0 = reinterpret_cast<const plUInt32*>(sourcePointer)[0];
        plUInt32 source1 = reinterpret_cast<const plUInt32*>(sourcePointer)[1];
        plUInt32 source2 = reinterpret_cast<const plUInt32*>(sourcePointer)[2];

        plUInt32 target0 = source0 | 0xFF000000;
        plUInt32 target1 = (source0 >> 24) | (source1 << 8) | 0xFF000000;
        plUInt32 target2 = (source1 >> 16) | (source2 << 16) | 0xFF000000;
        plUInt32 target3 = (source2 >> 8) | 0xFF000000;

        reinterpret_cast<plUInt32*>(targetPointer)[0] = target0;
        reinterpret_cast<plUInt32*>(targetPointer)[1] = target1;
        reinterpret_cast<plUInt32*>(targetPointer)[2] = target2;
        reinterpret_cast<plUInt32*>(targetPointer)[3] = target3;

        sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        uiNumElements -= elementsPerBatch;
      }
    }
#endif


    while (uiNumElements)
    {
      // Copy existing channels
      memcpy(targetPointer, sourcePointer, numChannels);

      // Fill others with zero
      memset(targetPointer + numChannels, 0, 3 * sizeof(plUInt8) - numChannels);

      // Set alpha to 1
      targetPointer[3] = 0xFF;

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

struct plImageConversion_Pad_To_RGBA_F32 : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R32_FLOAT, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32_FLOAT, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32_FLOAT, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    plUInt32 sourceStride = plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    plUInt32 targetStride = plImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const float* sourcePointer = static_cast<const float*>(static_cast<const void*>(source.GetPtr()));
    float* targetPointer = static_cast<float*>(static_cast<void*>(target.GetPtr()));

    const plUInt32 numChannels = sourceStride / sizeof(float);

    while (uiNumElements)
    {
      // Copy existing channels
      memcpy(targetPointer, sourcePointer, numChannels * sizeof(float));

      // Fill others with zero
      memset(targetPointer + numChannels, 0, sizeof(float) * (3 - numChannels));

      // Set alpha to 1
      targetPointer[3] = 1.0f;

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

struct plImageConversion_DiscardChannels : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R32G32B32A32_FLOAT, plImageFormat::R32G32B32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32A32_FLOAT, plImageFormat::R32G32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32A32_FLOAT, plImageFormat::R32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32A32_UINT, plImageFormat::R32G32B32_UINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32A32_UINT, plImageFormat::R32G32_UINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32A32_UINT, plImageFormat::R32_UINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32A32_SINT, plImageFormat::R32G32B32_SINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32A32_SINT, plImageFormat::R32G32_SINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32A32_SINT, plImageFormat::R32_SINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32_FLOAT, plImageFormat::R32G32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32_FLOAT, plImageFormat::R32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32_UINT, plImageFormat::R32G32_UINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32_UINT, plImageFormat::R32_UINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32_SINT, plImageFormat::R32G32_SINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32_SINT, plImageFormat::R32_SINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_FLOAT, plImageFormat::R16G16_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_FLOAT, plImageFormat::R16_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_UNORM, plImageFormat::R16G16B16_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_UNORM, plImageFormat::R16G16_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_UNORM, plImageFormat::R16_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_UINT, plImageFormat::R16G16_UINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_UINT, plImageFormat::R16_UINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_SNORM, plImageFormat::R16G16_SNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_SNORM, plImageFormat::R16_SNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_SINT, plImageFormat::R16G16_SINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_SINT, plImageFormat::R16_SINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16_UNORM, plImageFormat::R16G16_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16_UNORM, plImageFormat::R16_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32_FLOAT, plImageFormat::R32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32_UINT, plImageFormat::R32_UINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32_SINT, plImageFormat::R32_SINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::D32_FLOAT_S8X24_UINT, plImageFormat::D32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM, plImageFormat::R8G8B8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM, plImageFormat::R8G8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM, plImageFormat::R8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM_SRGB, plImageFormat::R8G8B8_UNORM_SRGB, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UINT, plImageFormat::R8G8_UINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UINT, plImageFormat::R8_UINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_SNORM, plImageFormat::R8G8_SNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_SNORM, plImageFormat::R8_SNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_SINT, plImageFormat::R8G8_SINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_SINT, plImageFormat::R8_SINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::B8G8R8A8_UNORM, plImageFormat::B8G8R8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::B8G8R8A8_UNORM_SRGB, plImageFormat::B8G8R8_UNORM_SRGB, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::B8G8R8X8_UNORM, plImageFormat::B8G8R8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::B8G8R8X8_UNORM_SRGB, plImageFormat::B8G8R8_UNORM_SRGB, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16_FLOAT, plImageFormat::R16_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16_UNORM, plImageFormat::R16_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16_UINT, plImageFormat::R16_UINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16_SNORM, plImageFormat::R16_SNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16_SINT, plImageFormat::R16_SINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8_UNORM, plImageFormat::R8G8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8_UNORM, plImageFormat::R8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8_UNORM, plImageFormat::R8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8_UINT, plImageFormat::R8_UINT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8_SNORM, plImageFormat::R8_SNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8_SINT, plImageFormat::R8_SINT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    plUInt32 sourceStride = plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    plUInt32 targetStride = plImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    if (plImageFormat::GetBitsPerPixel(sourceFormat) == 32 && plImageFormat::GetBitsPerPixel(targetFormat) == 24)
    {
      // Fast path for RGBA -> RGB
      while (uiNumElements)
      {
        const plUInt8* src = static_cast<const plUInt8*>(sourcePointer);
        plUInt8* dst = static_cast<plUInt8*>(targetPointer);

        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];

        sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
        targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
        uiNumElements--;
      }
    }

    while (uiNumElements)
    {
      memcpy(targetPointer, sourcePointer, targetStride);

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_FLOAT_to_R11G11B10 : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R32G32B32A32_FLOAT, plImageFormat::R11G11B10_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32_FLOAT, plImageFormat::R11G11B10_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    plUInt32 sourceStride = plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    plUInt32 targetStride = plImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      // Adapted from DirectXMath's XMStoreFloat3PK
      plUInt32 IValue[3];
      memcpy(IValue, sourcePointer, 12);

      plUInt32 Result[3];

      // X & Y Channels (5-bit exponent, 6-bit mantissa)
      for (plUInt32 j = 0; j < 2; ++j)
      {
        plUInt32 Sign = IValue[j] & 0x80000000;
        plUInt32 I = IValue[j] & 0x7FFFFFFF;

        if ((I & 0x7F800000) == 0x7F800000)
        {
          // INF or NAN
          Result[j] = 0x7c0;
          if ((I & 0x7FFFFF) != 0)
          {
            Result[j] = 0x7c0 | (((I >> 17) | (I >> 11) | (I >> 6) | (I)) & 0x3f);
          }
          else if (Sign)
          {
            // -INF is clamped to 0 since 3PK is positive only
            Result[j] = 0;
          }
        }
        else if (Sign)
        {
          // 3PK is positive only, so clamp to zero
          Result[j] = 0;
        }
        else if (I > 0x477E0000U)
        {
          // The number is too large to be represented as a float11, set to max
          Result[j] = 0x7BF;
        }
        else
        {
          if (I < 0x38800000U)
          {
            // The number is too small to be represented as a normalized float11
            // Convert it to a denormalized value.
            plUInt32 Shift = 113U - (I >> 23U);
            I = (0x800000U | (I & 0x7FFFFFU)) >> Shift;
          }
          else
          {
            // Rebias the exponent to represent the value as a normalized float11
            I += 0xC8000000U;
          }

          Result[j] = ((I + 0xFFFFU + ((I >> 17U) & 1U)) >> 17U) & 0x7ffU;
        }
      }

      // Z Channel (5-bit exponent, 5-bit mantissa)
      plUInt32 Sign = IValue[2] & 0x80000000;
      plUInt32 I = IValue[2] & 0x7FFFFFFF;

      if ((I & 0x7F800000) == 0x7F800000)
      {
        // INF or NAN
        Result[2] = 0x3e0;
        if (I & 0x7FFFFF)
        {
          Result[2] = 0x3e0 | (((I >> 18) | (I >> 13) | (I >> 3) | (I)) & 0x1f);
        }
        else if (Sign)
        {
          // -INF is clamped to 0 since 3PK is positive only
          Result[2] = 0;
        }
      }
      else if (Sign)
      {
        // 3PK is positive only, so clamp to zero
        Result[2] = 0;
      }
      else if (I > 0x477C0000U)
      {
        // The number is too large to be represented as a float10, set to max
        Result[2] = 0x3df;
      }
      else
      {
        if (I < 0x38800000U)
        {
          // The number is too small to be represented as a normalized float10
          // Convert it to a denormalized value.
          plUInt32 Shift = 113U - (I >> 23U);
          I = (0x800000U | (I & 0x7FFFFFU)) >> Shift;
        }
        else
        {
          // Rebias the exponent to represent the value as a normalized float10
          I += 0xC8000000U;
        }

        Result[2] = ((I + 0x1FFFFU + ((I >> 18U) & 1U)) >> 18U) & 0x3ffU;
      }

      // Pack Result into memory
      *reinterpret_cast<plUInt32*>(targetPointer) = (Result[0] & 0x7ff) | ((Result[1] & 0x7ff) << 11) | ((Result[2] & 0x3ff) << 22);

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_R11G11B10_to_FLOAT : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R11G11B10_FLOAT, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R11G11B10_FLOAT, plImageFormat::R32G32B32_FLOAT, plImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    plUInt32 sourceStride = plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    plUInt32 targetStride = plImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      const R11G11B10* pSource = reinterpret_cast<const R11G11B10*>(sourcePointer);
      plUInt32* targetUi = reinterpret_cast<plUInt32*>(targetPointer);

      // Adapted from XMLoadFloat3PK
      plUInt32 Mantissa;
      plUInt32 Exponent;

      // X Channel (6-bit mantissa)
      Mantissa = pSource->p.xm;

      if (pSource->p.xe == 0x1f) // INF or NAN
      {
        targetUi[0] = 0x7f800000 | (pSource->p.xm << 17);
      }
      else
      {
        if (pSource->p.xe != 0) // The value is normalized
        {
          Exponent = pSource->p.xe;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x40) == 0);

          Mantissa &= 0x3F;
        }
        else // The value is zero
        {
          Exponent = (plUInt32)-112;
        }

        targetUi[0] = ((Exponent + 112) << 23) | (Mantissa << 17);
      }

      // Y Channel (6-bit mantissa)
      Mantissa = pSource->p.ym;

      if (pSource->p.ye == 0x1f) // INF or NAN
      {
        targetUi[1] = 0x7f800000 | (pSource->p.ym << 17);
      }
      else
      {
        if (pSource->p.ye != 0) // The value is normalized
        {
          Exponent = pSource->p.ye;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x40) == 0);

          Mantissa &= 0x3F;
        }
        else // The value is zero
        {
          Exponent = (plUInt32)-112;
        }

        targetUi[1] = ((Exponent + 112) << 23) | (Mantissa << 17);
      }

      // Z Channel (5-bit mantissa)
      Mantissa = pSource->p.zm;

      if (pSource->p.ze == 0x1f) // INF or NAN
      {
        targetUi[2] = 0x7f800000 | (pSource->p.zm << 17);
      }
      else
      {
        if (pSource->p.ze != 0) // The value is normalized
        {
          Exponent = pSource->p.ze;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x20) == 0);

          Mantissa &= 0x1F;
        }
        else // The value is zero
        {
          Exponent = (plUInt32)-112;
        }

        targetUi[2] = ((Exponent + 112) << 23) | (Mantissa << 18);
      }

      if (targetStride > sizeof(float) * 3)
      {
        reinterpret_cast<float*>(targetPointer)[3] = 1.0f; // Write alpha channel
      }
      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_R11G11B10_to_HALF : public plImageConversionStepLinear
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R11G11B10_FLOAT, plImageFormat::R16G16B16A16_FLOAT, plImageConversionFlags::Default)};
    return supportedConversions;
  }

  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    plUInt32 sourceStride = plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    plUInt32 targetStride = plImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      plUInt16* result = reinterpret_cast<plUInt16*>(targetPointer);
      const R11G11B10* r11g11b10 = reinterpret_cast<const R11G11B10*>(sourcePointer);

      // We can do a straight forward conversion here because R11G11B10 uses the same number of bits for the exponent as a half
      // This means that all special values, e.g. denormals, inf, nan map exactly.
      result[0] = static_cast<plUInt16>((r11g11b10->p.xe << 10) | (r11g11b10->p.xm << 4));
      result[1] = static_cast<plUInt16>((r11g11b10->p.ye << 10) | (r11g11b10->p.ym << 4));
      result[2] = static_cast<plUInt16>((r11g11b10->p.ze << 10) | (r11g11b10->p.zm << 5));
      result[3] = 0x3C00; // hex value of 1.0f as half

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};


template <typename T>
class plImageConversion_Int_To_F32 : public plImageConversionStepLinear
{
public:
  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= plImageFormat::GetBitsPerPixel(targetFormat) / 32;

    const plUInt32 sourceStride = sizeof(T);
    const plUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = static_cast<float>(*reinterpret_cast<const T*>(sourcePointer));

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return PL_SUCCESS;
  }
};


class plImageConversion_UINT8_F32 : public plImageConversion_Int_To_F32<plUInt8>
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R8_UINT, plImageFormat::R32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8_UINT, plImageFormat::R32G32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UINT, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class plImageConversion_SINT8_F32 : public plImageConversion_Int_To_F32<plInt8>
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R8_SINT, plImageFormat::R32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8_SINT, plImageFormat::R32G32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_SINT, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class plImageConversion_UINT16_F32 : public plImageConversion_Int_To_F32<plUInt16>
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R16_UINT, plImageFormat::R32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16_UINT, plImageFormat::R32G32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_UINT, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class plImageConversion_SINT16_F32 : public plImageConversion_Int_To_F32<plInt16>
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R16_SINT, plImageFormat::R32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16_SINT, plImageFormat::R32G32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R16G16B16A16_SINT, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class plImageConversion_UINT32_F32 : public plImageConversion_Int_To_F32<plUInt32>
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R32_UINT, plImageFormat::R32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32_UINT, plImageFormat::R32G32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32_UINT, plImageFormat::R32G32B32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32A32_UINT, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};

class plImageConversion_SINT32_F32 : public plImageConversion_Int_To_F32<plInt32>
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R32_SINT, plImageFormat::R32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32_SINT, plImageFormat::R32G32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32_SINT, plImageFormat::R32G32B32_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R32G32B32A32_SINT, plImageFormat::R32G32B32A32_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }
};


#define ADD_16BPP_CONVERSION(format)                                                                                                                 \
  static plImageConversionStep_Decompress16bpp<plDecompress##format, plImageFormat::format##_UNORM> s_conversion_plDecompress##format;               \
  static plImageConversionStep_Compress16bpp<plCompress##format, plImageFormat::format##_UNORM> s_conversion_plCompress##format

ADD_16BPP_CONVERSION(A4B4G4R4);
ADD_16BPP_CONVERSION(B4G4R4A4);
ADD_16BPP_CONVERSION(B5G6R5);
ADD_16BPP_CONVERSION(B5G5R5X1);
ADD_16BPP_CONVERSION(B5G5R5A1);
ADD_16BPP_CONVERSION(X1B5G5R5);
ADD_16BPP_CONVERSION(A1B5G5R5);

// PL_STATICLINK_FORCE
static plImageSwizzleConversion32_2103 s_conversion_swizzle2103;
static plImageConversion_BGRX_BGRA s_conversion_BGRX_BGRA;
static plImageConversion_F32_U8 s_conversion_F32_U8;
static plImageConversion_F32_sRGB s_conversion_F32_sRGB;
static plImageConversion_F32_U16 s_conversion_F32_U16;
static plImageConversion_F32_F16 s_conversion_F32_F16;
static plImageConversion_F32_S8 s_conversion_F32_S8;
static plImageConversion_U8_F32 s_conversion_U8_F32;
static plImageConversion_sRGB_F32 s_conversion_sRGB_F32;
static plImageConversion_U16_F32 s_conversion_U16_F32;
static plImageConversion_S16_F32 s_conversion_S16_F32;
static plImageConversion_F16_F32 s_conversion_F16_F32;
static plImageConversion_S8_F32 s_conversion_S8_F32;
static plImageConversion_UINT8_F32 s_conversion_UINT8_F32;
static plImageConversion_SINT8_F32 s_conversion_SINT8_F32;
static plImageConversion_UINT16_F32 s_conversion_UINT16_F32;
static plImageConversion_SINT16_F32 s_conversion_SINT16_F32;
static plImageConversion_UINT32_F32 s_conversion_UINT32_F32;
static plImageConversion_SINT32_F32 s_conversion_SINT32_F32;

static plImageConversion_Pad_To_RGBA_U8 s_conversion_Pad_To_RGBA_U8;
static plImageConversion_Pad_To_RGBA_F32 s_conversion_Pad_To_RGBA_F32;
static plImageConversion_DiscardChannels s_conversion_DiscardChannels;

static plImageConversion_R11G11B10_to_FLOAT s_conversion_R11G11B10_to_FLOAT;
static plImageConversion_R11G11B10_to_HALF s_conversion_R11G11B10_to_HALF;
static plImageConversion_FLOAT_to_R11G11B10 s_conversion_FLOAT_to_R11G11B10;




PL_STATICLINK_FILE(Texture, Texture_Image_Conversions_PixelConversions);


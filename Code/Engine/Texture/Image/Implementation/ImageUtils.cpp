#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageUtils.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/Image/ImageFilter.h>

template <typename TYPE>
static void SetDiff(const plImageView& ImageA, const plImageView& ImageB, plImage& out_Difference, plUInt32 w, plUInt32 h, plUInt32 d, plUInt32 comp)
{
  const TYPE* pA = ImageA.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  const TYPE* pB = ImageB.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  TYPE* pR = out_Difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  for (plUInt32 i = 0; i < comp; ++i)
    pR[i] = pB[i] > pA[i] ? (pB[i] - pA[i]) : (pA[i] - pB[i]);
}

template <typename TYPE>
static plUInt32 GetError(const plImageView& Difference, plUInt32 w, plUInt32 h, plUInt32 d, plUInt32 comp, plUInt32 pixel)
{
  const TYPE* pR = Difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  plUInt32 uiErrorSum = 0;

  for (plUInt32 p = 0; p < pixel; ++p)
  {
    plUInt32 error = 0;

    for (plUInt32 c = 0; c < comp; ++c)
    {
      error += *pR;
      ++pR;
    }

    error /= comp;
    uiErrorSum += error * error;
  }

  return uiErrorSum;
}

void plImageUtils::ComputeImageDifferenceABS(const plImageView& ImageA, const plImageView& ImageB, plImage& out_Difference)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::ComputeImageDifferenceABS");

  PLASMA_ASSERT_DEV(ImageA.GetWidth() == ImageB.GetWidth(), "Dimensions do not match");
  PLASMA_ASSERT_DEV(ImageA.GetHeight() == ImageB.GetHeight(), "Dimensions do not match");
  PLASMA_ASSERT_DEV(ImageA.GetDepth() == ImageB.GetDepth(), "Dimensions do not match");
  PLASMA_ASSERT_DEV(ImageA.GetImageFormat() == ImageB.GetImageFormat(), "Format does not match");

  plImageHeader differenceHeader;

  differenceHeader.SetWidth(ImageA.GetWidth());
  differenceHeader.SetHeight(ImageA.GetHeight());
  differenceHeader.SetDepth(ImageA.GetDepth());
  differenceHeader.SetImageFormat(ImageA.GetImageFormat());
  out_Difference.ResetAndAlloc(differenceHeader);

  const plUInt32 uiSize2D = ImageA.GetHeight() * ImageA.GetWidth();

  for (plUInt32 d = 0; d < ImageA.GetDepth(); ++d)
  {
    // for (plUInt32 h = 0; h < ImageA.GetHeight(); ++h)
    {
      // for (plUInt32 w = 0; w < ImageA.GetWidth(); ++w)
      {
        switch (ImageA.GetImageFormat())
        {
          case plImageFormat::R8G8B8A8_UNORM:
          case plImageFormat::R8G8B8A8_UNORM_SRGB:
          case plImageFormat::R8G8B8A8_UINT:
          case plImageFormat::R8G8B8A8_SNORM:
          case plImageFormat::R8G8B8A8_SINT:
          case plImageFormat::B8G8R8A8_UNORM:
          case plImageFormat::B8G8R8X8_UNORM:
          case plImageFormat::B8G8R8A8_UNORM_SRGB:
          case plImageFormat::B8G8R8X8_UNORM_SRGB:
          {
            SetDiff<plUInt8>(ImageA, ImageB, out_Difference, 0, 0, d, 4 * uiSize2D);
          }
          break;

          case plImageFormat::B8G8R8_UNORM:
          {
            SetDiff<plUInt8>(ImageA, ImageB, out_Difference, 0, 0, d, 3 * uiSize2D);
          }
          break;

          default:
            PLASMA_REPORT_FAILURE("The plImageFormat {0} is not implemented", (plUInt32)ImageA.GetImageFormat());
            return;
        }
      }
    }
  }
}

plUInt32 plImageUtils::ComputeMeanSquareError(const plImageView& DifferenceImage, plUInt8 uiBlockSize, plUInt32 offsetx, plUInt32 offsety)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::ComputeMeanSquareError(detail)");

  PLASMA_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  plUInt32 uiWidth = plMath::Min(DifferenceImage.GetWidth(), offsetx + uiBlockSize) - offsetx;
  plUInt32 uiHeight = plMath::Min(DifferenceImage.GetHeight(), offsety + uiBlockSize) - offsety;

  if (uiWidth == 0 || uiHeight == 0)
    return 0;

  switch (DifferenceImage.GetImageFormat())
  {
      // Supported formats
    case plImageFormat::R8G8B8A8_UNORM:
    case plImageFormat::R8G8B8A8_UNORM_SRGB:
    case plImageFormat::R8G8B8A8_UINT:
    case plImageFormat::R8G8B8A8_SNORM:
    case plImageFormat::R8G8B8A8_SINT:
    case plImageFormat::B8G8R8A8_UNORM:
    case plImageFormat::B8G8R8A8_UNORM_SRGB:
    case plImageFormat::B8G8R8_UNORM:
      break;

    default:
      PLASMA_REPORT_FAILURE("The plImageFormat {0} is not implemented", (plUInt32)DifferenceImage.GetImageFormat());
      return 0;
  }


  plUInt32 error = 0;

  plUInt64 uiRowPitch = DifferenceImage.GetRowPitch();
  plUInt64 uiDepthPitch = DifferenceImage.GetDepthPitch();
  plUInt32 uiNumComponents = plImageFormat::GetNumChannels(DifferenceImage.GetImageFormat());

  // Treat image as single-component format and scale the width instead
  uiWidth *= uiNumComponents;

  const plUInt32 uiSize2D = uiWidth * uiHeight;
  const plUInt8* pSlicePointer = DifferenceImage.GetPixelPointer<plUInt8>(0, 0, 0, offsetx, offsety);

  for (plUInt32 d = 0; d < DifferenceImage.GetDepth(); ++d)
  {
    const plUInt8* pRowPointer = pSlicePointer;

    for (plUInt32 y = 0; y < uiHeight; ++y)
    {
      const plUInt8* pPixelPointer = pRowPointer;
      for (plUInt32 x = 0; x < uiWidth; ++x)
      {
        plUInt32 uiDiff = *pPixelPointer;
        error += uiDiff * uiDiff;

        pPixelPointer++;
      }

      pRowPointer += uiRowPitch;
    }

    pSlicePointer += uiDepthPitch;
  }

  error /= uiSize2D;
  return error;
}

plUInt32 plImageUtils::ComputeMeanSquareError(const plImageView& DifferenceImage, plUInt8 uiBlockSize)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::ComputeMeanSquareError");

  PLASMA_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  const plUInt32 uiHalfBlockSize = uiBlockSize / 2;

  const plUInt32 uiBlocksX = (DifferenceImage.GetWidth() / uiHalfBlockSize) + 1;
  const plUInt32 uiBlocksY = (DifferenceImage.GetHeight() / uiHalfBlockSize) + 1;

  plUInt32 uiMaxError = 0;

  for (plUInt32 by = 0; by < uiBlocksY; ++by)
  {
    for (plUInt32 bx = 0; bx < uiBlocksX; ++bx)
    {
      const plUInt32 uiBlockError = ComputeMeanSquareError(DifferenceImage, uiBlockSize, bx * uiHalfBlockSize, by * uiHalfBlockSize);

      uiMaxError = plMath::Max(uiMaxError, uiBlockError);
    }
  }

  return uiMaxError;
}

template <typename Func, typename ImageType>
static void ApplyFunc(ImageType& image, Func func)
{
  plUInt32 uiWidth = image.GetWidth();
  plUInt32 uiHeight = image.GetHeight();
  plUInt32 uiDepth = image.GetDepth();

  PLASMA_ASSERT_DEV(uiWidth > 0 && uiHeight > 0 && uiDepth > 0, "The image passed to FindMinMax has illegal dimension {}x{}x{}.", uiWidth, uiHeight, uiDepth);

  plUInt64 uiRowPitch = image.GetRowPitch();
  plUInt64 uiDepthPitch = image.GetDepthPitch();
  plUInt32 uiNumChannels = plImageFormat::GetNumChannels(image.GetImageFormat());

  auto pSlicePointer = image.template GetPixelPointer<plUInt8>();

  for (plUInt32 z = 0; z < image.GetDepth(); ++z)
  {
    auto pRowPointer = pSlicePointer;

    for (plUInt32 y = 0; y < uiHeight; ++y)
    {
      auto pPixelPointer = pRowPointer;
      for (plUInt32 x = 0; x < uiWidth; ++x)
      {
        for (plUInt32 c = 0; c < uiNumChannels; ++c)
        {
          func(pPixelPointer++, x, y, z, c);
        }
      }

      pRowPointer += uiRowPitch;
    }

    pSlicePointer += uiDepthPitch;
  }
}

static void FindMinMax(const plImageView& image, plUInt8& uiMinRgb, plUInt8& uiMaxRgb, plUInt8& uiMinAlpha, plUInt8& uiMaxAlpha)
{
  plImageFormat::Enum imageFormat = image.GetImageFormat();
  PLASMA_ASSERT_DEV(plImageFormat::GetBitsPerChannel(imageFormat, plImageFormatChannel::R) == 8 && plImageFormat::GetDataType(imageFormat) == plImageFormatDataType::UNORM, "Only 8bpp unorm formats are supported in FindMinMax");

  uiMinRgb = 255u;
  uiMinAlpha = 255u;
  uiMaxRgb = 0u;
  uiMaxAlpha = 0u;

  auto minMax = [&](const plUInt8* pixel, plUInt32 /*x*/, plUInt32 /*y*/, plUInt32 /*z*/, plUInt32 c)
  {
    plUInt8 val = *pixel;

    if (c < 3)
    {
      uiMinRgb = plMath::Min(uiMinRgb, val);
      uiMaxRgb = plMath::Max(uiMaxRgb, val);
    }
    else
    {
      uiMinAlpha = plMath::Min(uiMinAlpha, val);
      uiMaxAlpha = plMath::Max(uiMaxAlpha, val);
    }
  };
  ApplyFunc(image, minMax);
}

void plImageUtils::Normalize(plImage& image)
{
  plUInt8 uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha;
  Normalize(image, uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha);
}

void plImageUtils::Normalize(plImage& image, plUInt8& uiMinRgb, plUInt8& uiMaxRgb, plUInt8& uiMinAlpha, plUInt8& uiMaxAlpha)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::Normalize");

  plImageFormat::Enum imageFormat = image.GetImageFormat();

  PLASMA_ASSERT_DEV(plImageFormat::GetBitsPerChannel(imageFormat, plImageFormatChannel::R) == 8 && plImageFormat::GetDataType(imageFormat) == plImageFormatDataType::UNORM, "Only 8bpp unorm formats are supported in NormalizeImage");

  bool ignoreAlpha = false;
  if (imageFormat == plImageFormat::B8G8R8X8_UNORM || imageFormat == plImageFormat::B8G8R8X8_UNORM_SRGB)
  {
    ignoreAlpha = true;
  }

  FindMinMax(image, uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha);
  plUInt8 uiRangeRgb = uiMaxRgb - uiMinRgb;
  plUInt8 uiRangeAlpha = uiMaxAlpha - uiMinAlpha;

  auto normalize = [&](plUInt8* pixel, plUInt32 /*x*/, plUInt32 /*y*/, plUInt32 /*z*/, plUInt32 c)
  {
    plUInt8 val = *pixel;
    if (c < 3)
    {
      // color channels are uniform when min == max, in that case keep original value as scaling is not meaningful
      if (uiRangeRgb != 0)
      {
        *pixel = static_cast<plUInt8>(255u * (static_cast<float>(val - uiMinRgb) / (uiRangeRgb)));
      }
    }
    else
    {
      // alpha is uniform when minAlpha == maxAlpha, in that case keep original alpha as scaling is not meaningful
      if (!ignoreAlpha && uiRangeAlpha != 0)
      {
        *pixel = static_cast<plUInt8>(255u * (static_cast<float>(val - uiMinAlpha) / (uiRangeAlpha)));
      }
    }
  };
  ApplyFunc(image, normalize);
}

void plImageUtils::ExtractAlphaChannel(const plImageView& inputImage, plImage& outputImage)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::ExtractAlphaChannel");

  switch (plImageFormat::Enum imageFormat = inputImage.GetImageFormat())
  {
    case plImageFormat::R8G8B8A8_UNORM:
    case plImageFormat::R8G8B8A8_UNORM_SRGB:
    case plImageFormat::R8G8B8A8_UINT:
    case plImageFormat::R8G8B8A8_SNORM:
    case plImageFormat::R8G8B8A8_SINT:
    case plImageFormat::B8G8R8A8_UNORM:
    case plImageFormat::B8G8R8A8_UNORM_SRGB:
      break;
    default:
      PLASMA_REPORT_FAILURE("ExtractAlpha needs an image with 8bpp and 4 channel. The plImageFormat {} is not supported.", (plUInt32)imageFormat);
      return;
  }

  plImageHeader outputHeader = inputImage.GetHeader();
  outputHeader.SetImageFormat(plImageFormat::R8_UNORM);
  outputImage.ResetAndAlloc(outputHeader);

  const plUInt8* pInputSlice = inputImage.GetPixelPointer<plUInt8>();
  plUInt8* pOutputSlice = outputImage.GetPixelPointer<plUInt8>();

  plUInt64 uiInputRowPitch = inputImage.GetRowPitch();
  plUInt64 uiInputDepthPitch = inputImage.GetDepthPitch();

  plUInt64 uiOutputRowPitch = outputImage.GetRowPitch();
  plUInt64 uiOutputDepthPitch = outputImage.GetDepthPitch();

  for (plUInt32 d = 0; d < inputImage.GetDepth(); ++d)
  {
    const plUInt8* pInputRow = pInputSlice;
    plUInt8* pOutputRow = pOutputSlice;

    for (plUInt32 y = 0; y < inputImage.GetHeight(); ++y)
    {
      const plUInt8* pInputPixel = pInputRow;
      plUInt8* pOutputPixel = pOutputRow;
      for (plUInt32 x = 0; x < inputImage.GetWidth(); ++x)
      {
        *pOutputPixel = pInputPixel[3];

        pInputPixel += 4;
        ++pOutputPixel;
      }

      pInputRow += uiInputRowPitch;
      pOutputRow += uiOutputRowPitch;
    }

    pInputSlice += uiInputDepthPitch;
    pOutputSlice += uiOutputDepthPitch;
  }
}

void plImageUtils::CropImage(const plImageView& input, const plVec2I32& offset, const plSizeU32& newsize, plImage& output)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::CropImage");

  PLASMA_ASSERT_DEV(offset.x >= 0, "Offset is invalid");
  PLASMA_ASSERT_DEV(offset.y >= 0, "Offset is invalid");
  PLASMA_ASSERT_DEV(offset.x < (plInt32)input.GetWidth(), "Offset is invalid");
  PLASMA_ASSERT_DEV(offset.y < (plInt32)input.GetHeight(), "Offset is invalid");

  const plUInt32 uiNewWidth = plMath::Min(offset.x + newsize.width, input.GetWidth()) - offset.x;
  const plUInt32 uiNewHeight = plMath::Min(offset.y + newsize.height, input.GetHeight()) - offset.y;

  plImageHeader outputHeader;
  outputHeader.SetWidth(uiNewWidth);
  outputHeader.SetHeight(uiNewHeight);
  outputHeader.SetImageFormat(input.GetImageFormat());
  output.ResetAndAlloc(outputHeader);

  for (plUInt32 y = 0; y < uiNewHeight; ++y)
  {
    for (plUInt32 x = 0; x < uiNewWidth; ++x)
    {
      switch (input.GetImageFormat())
      {
        case plImageFormat::R8G8B8A8_UNORM:
        case plImageFormat::R8G8B8A8_UNORM_SRGB:
        case plImageFormat::R8G8B8A8_UINT:
        case plImageFormat::R8G8B8A8_SNORM:
        case plImageFormat::R8G8B8A8_SINT:
        case plImageFormat::B8G8R8A8_UNORM:
        case plImageFormat::B8G8R8X8_UNORM:
        case plImageFormat::B8G8R8A8_UNORM_SRGB:
        case plImageFormat::B8G8R8X8_UNORM_SRGB:
          output.GetPixelPointer<plUInt32>(0, 0, 0, x, y)[0] = input.GetPixelPointer<plUInt32>(0, 0, 0, offset.x + x, offset.y + y)[0];
          break;

        case plImageFormat::B8G8R8_UNORM:
          output.GetPixelPointer<plUInt8>(0, 0, 0, x, y)[0] = input.GetPixelPointer<plUInt8>(0, 0, 0, offset.x + x, offset.y + y)[0];
          output.GetPixelPointer<plUInt8>(0, 0, 0, x, y)[1] = input.GetPixelPointer<plUInt8>(0, 0, 0, offset.x + x, offset.y + y)[1];
          output.GetPixelPointer<plUInt8>(0, 0, 0, x, y)[2] = input.GetPixelPointer<plUInt8>(0, 0, 0, offset.x + x, offset.y + y)[2];
          break;

        default:
          PLASMA_REPORT_FAILURE("The plImageFormat {0} is not implemented", (plUInt32)input.GetImageFormat());
          return;
      }
    }
  }
}

namespace
{
  template <typename T>
  void rotate180(T* start, T* end)
  {
    end = end - 1;
    while (start < end)
    {
      plMath::Swap(*start, *end);
      start++;
      end--;
    }
  }
} // namespace

void plImageUtils::RotateSubImage180(plImage& image, plUInt32 uiMipLevel /*= 0*/, plUInt32 uiFace /*= 0*/, plUInt32 uiArrayIndex /*= 0*/)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::RotateSubImage180");

  plUInt8* start = image.GetPixelPointer<plUInt8>(uiMipLevel, uiFace, uiArrayIndex);
  plUInt8* end = start + image.GetDepthPitch(uiMipLevel);

  plUInt32 bytesPerPixel = plImageFormat::GetBitsPerPixel(image.GetImageFormat()) / 8;

  switch (bytesPerPixel)
  {
    case 4:
      rotate180<plUInt32>(reinterpret_cast<plUInt32*>(start), reinterpret_cast<plUInt32*>(end));
      break;
    case 12:
      rotate180<plVec3>(reinterpret_cast<plVec3*>(start), reinterpret_cast<plVec3*>(end));
      break;
    case 16:
      rotate180<plVec4>(reinterpret_cast<plVec4*>(start), reinterpret_cast<plVec4*>(end));
      break;
    default:
      // fallback version
      {
        end -= bytesPerPixel;
        while (start < end)
        {
          for (plUInt32 i = 0; i < bytesPerPixel; i++)
          {
            plMath::Swap(start[i], end[i]);
          }
          start += bytesPerPixel;
          end -= bytesPerPixel;
        }
      }
  }
}

plResult plImageUtils::Copy(const plImageView& srcImg, const plRectU32& srcRect, plImage& dstImg, const plVec3U32& dstOffset, plUInt32 uiDstMipLevel /*= 0*/, plUInt32 uiDstFace /*= 0*/, plUInt32 uiDstArrayIndex /*= 0*/)
{
  if (dstImg.GetImageFormat() != srcImg.GetImageFormat()) // Can only copy when the image formats are identical
    return PLASMA_FAILURE;

  if (plImageFormat::IsCompressed(dstImg.GetImageFormat())) // Compressed formats are not supported
    return PLASMA_FAILURE;

  PLASMA_PROFILE_SCOPE("plImageUtils::Copy");

  const plUInt64 uiDstRowPitch = dstImg.GetRowPitch(uiDstMipLevel);
  const plUInt64 uiSrcRowPitch = srcImg.GetRowPitch(uiDstMipLevel);
  const plUInt32 uiCopyBytesPerRow = plImageFormat::GetBitsPerPixel(srcImg.GetImageFormat()) * srcRect.width / 8;

  plUInt8* dstPtr = dstImg.GetPixelPointer<plUInt8>(uiDstMipLevel, uiDstFace, uiDstArrayIndex, dstOffset.x, dstOffset.y, dstOffset.z);
  const plUInt8* srcPtr = srcImg.GetPixelPointer<plUInt8>(0, 0, 0, srcRect.x, srcRect.y);

  for (plUInt32 y = 0; y < srcRect.height; y++)
  {
    plMemoryUtils::Copy(dstPtr, srcPtr, uiCopyBytesPerRow);

    dstPtr += uiDstRowPitch;
    srcPtr += uiSrcRowPitch;
  }

  return PLASMA_SUCCESS;
}

plResult plImageUtils::ExtractLowerMipChain(const plImageView& srcImg, plImage& dstImg, plUInt32 uiNumMips)
{
  const plImageHeader& srcImgHeader = srcImg.GetHeader();

  if (srcImgHeader.GetNumFaces() != 1 || srcImgHeader.GetNumArrayIndices() != 1)
  {
    // Lower mips aren't stored contiguously for array/cube textures and would require copying. This isn't implemented yet.
    return PLASMA_FAILURE;
  }

  PLASMA_PROFILE_SCOPE("plImageUtils::ExtractLowerMipChain");

  uiNumMips = plMath::Min(uiNumMips, srcImgHeader.GetNumMipLevels());

  plUInt32 startMipLevel = srcImgHeader.GetNumMipLevels() - uiNumMips;

  plImageFormat::Enum format = srcImgHeader.GetImageFormat();

  if (plImageFormat::RequiresFirstLevelBlockAlignment(format))
  {
    // Some block compressed image formats require resolutions that are divisible by block size,
    // therefore adjust startMipLevel accordingly
    while (srcImgHeader.GetWidth(startMipLevel) % plImageFormat::GetBlockWidth(format) != 0 || srcImgHeader.GetHeight(startMipLevel) % plImageFormat::GetBlockHeight(format) != 0)
    {
      if (uiNumMips >= srcImgHeader.GetNumMipLevels())
        return PLASMA_FAILURE;

      if (startMipLevel == 0)
        return PLASMA_FAILURE;

      ++uiNumMips;
      --startMipLevel;
    }
  }

  plImageHeader dstImgHeader = srcImgHeader;
  dstImgHeader.SetWidth(srcImgHeader.GetWidth(startMipLevel));
  dstImgHeader.SetHeight(srcImgHeader.GetHeight(startMipLevel));
  dstImgHeader.SetDepth(srcImgHeader.GetDepth(startMipLevel));
  dstImgHeader.SetNumFaces(srcImgHeader.GetNumFaces());
  dstImgHeader.SetNumArrayIndices(srcImgHeader.GetNumArrayIndices());
  dstImgHeader.SetNumMipLevels(uiNumMips);

  const plUInt8* pDataBegin = srcImg.GetPixelPointer<plUInt8>(startMipLevel);
  const plUInt8* pDataEnd = srcImg.GetByteBlobPtr().GetEndPtr();
  const ptrdiff_t dataSize = reinterpret_cast<ptrdiff_t>(pDataEnd) - reinterpret_cast<ptrdiff_t>(pDataBegin);

  const plConstByteBlobPtr lowResData(pDataBegin, static_cast<plUInt64>(dataSize));

  plImageView dataview;
  dataview.ResetAndViewExternalStorage(dstImgHeader, lowResData);

  dstImg.ResetAndCopy(dataview);

  return PLASMA_SUCCESS;
}

plUInt32 plImageUtils::GetSampleIndex(plUInt32 numTexels, plInt32 index, plImageAddressMode::Enum addressMode, bool& outUseBorderColor)
{
  outUseBorderColor = false;
  if (plUInt32(index) >= numTexels)
  {
    switch (addressMode)
    {
      case plImageAddressMode::Repeat:
        index %= numTexels;

        if (index < 0)
        {
          index += numTexels;
        }
        return index;

      case plImageAddressMode::Mirror:
      {
        if (index < 0)
        {
          index = -index - 1;
        }
        bool flip = (index / numTexels) & 1;
        index %= numTexels;
        if (flip)
        {
          index = numTexels - index - 1;
        }
        return index;
      }

      case plImageAddressMode::Clamp:
        return plMath::Clamp<plInt32>(index, 0, numTexels - 1);

      case plImageAddressMode::ClampBorder:
        outUseBorderColor = true;
        return 0;

      default:
        PLASMA_ASSERT_NOT_IMPLEMENTED
        return 0;
    }
  }
  return index;
}

static plSimdVec4f LoadSample(const plSimdVec4f* source, plUInt32 numSourceElements, plUInt32 stride, plInt32 index, plImageAddressMode::Enum addressMode, const plSimdVec4f& borderColor)
{
  bool useBorderColor = false;
  // result is in the range [-(w-1), (w-1)], bring it to [0, w - 1]
  index = plImageUtils::GetSampleIndex(numSourceElements, index, addressMode, useBorderColor);
  if (useBorderColor)
  {
    return borderColor;
  }
  return source[index * stride];
}

inline static void FilterLine(
  plUInt32 numSourceElements, const plSimdVec4f* __restrict sourceBegin, plSimdVec4f* __restrict targetBegin, plUInt32 stride, const plImageFilterWeights& weights, plArrayPtr<const plInt32> firstSampleIndices, plImageAddressMode::Enum addressMode, const plSimdVec4f& borderColor)
{
  // Convolve the image using the precomputed weights
  const plUInt32 numWeights = weights.GetNumWeights();

  // When the first source index for the output is between 0 and this value,
  // we can fetch all numWeights inputs without taking addressMode into consideration,
  // which makes the inner loop a lot faster.
  const plInt32 trivialSourceIndicesEnd = static_cast<plInt32>(numSourceElements) - static_cast<plInt32>(numWeights);
  const auto weightsView = weights.ViewWeights();
  const float* __restrict nextWeightPtr = weightsView.GetPtr();
  PLASMA_ASSERT_DEBUG((static_cast<plUInt32>(weightsView.GetCount()) % numWeights) == 0, "");
  for (plInt32 firstSourceIdx : firstSampleIndices)
  {
    plSimdVec4f total(0.0f, 0.0f, 0.0f, 0.0f);

    if (firstSourceIdx >= 0 && firstSourceIdx < trivialSourceIndicesEnd)
    {
      const auto* __restrict sourcePtr = sourceBegin + firstSourceIdx * stride;
      for (plUInt32 weightIdx = 0; weightIdx < numWeights; ++weightIdx)
      {
        total = plSimdVec4f::MulAdd(*sourcePtr, plSimdVec4f(*nextWeightPtr++), total);
        sourcePtr += stride;
      }
    }
    else
    {
      // Very slow fallback case that respects the addressMode
      // (not a lot of pixels are taking this path, so it's probably fine)
      plInt32 sourceIdx = firstSourceIdx;
      for (plUInt32 weightIdx = 0; weightIdx < numWeights; ++weightIdx)
      {
        total = plSimdVec4f::MulAdd(LoadSample(sourceBegin, numSourceElements, stride, sourceIdx, addressMode, borderColor), plSimdVec4f(*nextWeightPtr++), total);
        sourceIdx++;
      }
    }
    // It's ok to check this once per source index, see the assert above
    // (number of weights in weightsView is divisible by numWeights)
    if (nextWeightPtr == weightsView.GetEndPtr())
    {
      nextWeightPtr = weightsView.GetPtr();
    }
    *targetBegin = total;
    targetBegin += stride;
  }
}

static void DownScaleFastLine(plUInt32 pixelStride, const plUInt8* src, plUInt8* dest, plUInt32 lengthIn, plUInt32 strideIn, plUInt32 lengthOut, plUInt32 strideOut)
{
  const plUInt32 downScaleFactor = lengthIn / lengthOut;

  const plUInt32 downScaleFactorLog2 = plMath::Log2i(static_cast<plUInt32>(downScaleFactor));
  const plUInt32 roundOffset = downScaleFactor / 2;

  for (plUInt32 offset = 0; offset < lengthOut; ++offset)
  {
    for (plUInt32 channel = 0; channel < pixelStride; ++channel)
    {
      const plUInt32 destOffset = offset * strideOut + channel;

      plUInt32 curChannel = roundOffset;
      for (plUInt32 index = 0; index < downScaleFactor; ++index)
      {
        curChannel += static_cast<plUInt32>(src[channel + index * strideIn]);
      }

      curChannel = curChannel >> downScaleFactorLog2;
      dest[destOffset] = static_cast<plUInt8>(curChannel);
    }

    src += downScaleFactor * strideIn;
  }
}

static void DownScaleFast(const plImageView& image, plImage& out_Result, plUInt32 width, plUInt32 height)
{
  plImageFormat::Enum format = image.GetImageFormat();

  plUInt32 originalWidth = image.GetWidth();
  plUInt32 originalHeight = image.GetHeight();
  plUInt32 numArrayElements = image.GetNumArrayIndices();
  plUInt32 numFaces = image.GetNumFaces();

  plUInt32 pixelStride = plImageFormat::GetBitsPerPixel(format) / 8;

  plImageHeader intermediateHeader;
  intermediateHeader.SetWidth(width);
  intermediateHeader.SetHeight(originalHeight);
  intermediateHeader.SetNumArrayIndices(numArrayElements);
  intermediateHeader.SetNumFaces(numFaces);
  intermediateHeader.SetImageFormat(format);

  plImage intermediate;
  intermediate.ResetAndAlloc(intermediateHeader);

  for (plUInt32 arrayIndex = 0; arrayIndex < numArrayElements; arrayIndex++)
  {
    for (plUInt32 face = 0; face < numFaces; face++)
    {
      for (plUInt32 row = 0; row < originalHeight; row++)
      {
        DownScaleFastLine(pixelStride, image.GetPixelPointer<plUInt8>(0, face, arrayIndex, 0, row), intermediate.GetPixelPointer<plUInt8>(0, face, arrayIndex, 0, row), originalWidth, pixelStride, width, pixelStride);
      }
    }
  }

  // input and output images may be the same, so we can't access the original image below this point

  plImageHeader outHeader;
  outHeader.SetWidth(width);
  outHeader.SetHeight(height);
  outHeader.SetNumArrayIndices(numArrayElements);
  outHeader.SetNumArrayIndices(numFaces);
  outHeader.SetImageFormat(format);

  out_Result.ResetAndAlloc(outHeader);

  PLASMA_ASSERT_DEBUG(intermediate.GetRowPitch() < plMath::MaxValue<plUInt32>(), "Row pitch exceeds plUInt32 max value.");
  PLASMA_ASSERT_DEBUG(out_Result.GetRowPitch() < plMath::MaxValue<plUInt32>(), "Row pitch exceeds plUInt32 max value.");

  for (plUInt32 arrayIndex = 0; arrayIndex < numArrayElements; arrayIndex++)
  {
    for (plUInt32 face = 0; face < numFaces; face++)
    {
      for (plUInt32 col = 0; col < width; col++)
      {
        DownScaleFastLine(pixelStride, intermediate.GetPixelPointer<plUInt8>(0, face, arrayIndex, col), out_Result.GetPixelPointer<plUInt8>(0, face, arrayIndex, col), originalHeight, static_cast<plUInt32>(intermediate.GetRowPitch()), height, static_cast<plUInt32>(out_Result.GetRowPitch()));
      }
    }
  }
}

static float EvaluateAverageCoverage(plBlobPtr<const plColor> colors, float alphaThreshold)
{
  PLASMA_PROFILE_SCOPE("EvaluateAverageCoverage");

  plUInt64 totalPixels = colors.GetCount();
  plUInt64 count = 0;
  for (plUInt32 idx = 0; idx < totalPixels; ++idx)
  {
    count += colors[idx].a >= alphaThreshold;
  }

  return float(count) / float(totalPixels);
}

static void NormalizeCoverage(plBlobPtr<plColor> colors, float alphaThreshold, float targetCoverage)
{
  PLASMA_PROFILE_SCOPE("NormalizeCoverage");

  // Based on the idea in http://the-witness.net/news/2010/09/computing-alpha-mipmaps/. Note we're using a histogram
  // to find the new alpha threshold here rather than bisecting.

  // Generate histogram of alpha values
  plUInt64 totalPixels = colors.GetCount();
  plUInt32 alphaHistogram[256] = {};
  for (plUInt64 idx = 0; idx < totalPixels; ++idx)
  {
    alphaHistogram[plMath::ColorFloatToByte(colors[idx].a)]++;
  }

  // Find range of alpha thresholds so the number of covered pixels matches by summing up the histogram
  plInt32 targetCount = plInt32(targetCoverage * totalPixels);
  plInt32 coverageCount = 0;
  plInt32 maxThreshold = 255;
  for (; maxThreshold >= 0; maxThreshold--)
  {
    coverageCount += alphaHistogram[maxThreshold];

    if (coverageCount >= targetCount)
    {
      break;
    }
  }

  coverageCount = targetCount;
  plInt32 minThreshold = 0;
  for (; minThreshold < 256; minThreshold++)
  {
    coverageCount -= alphaHistogram[maxThreshold];

    if (coverageCount <= targetCount)
    {
      break;
    }
  }

  plInt32 currentThreshold = plMath::ColorFloatToByte(alphaThreshold);

  // Each of the alpha test thresholds in the range [minThreshold; maxThreshold] will result in the same coverage. Pick a new threshold
  // close to the old one so we scale by the smallest necessary amount.
  plInt32 newThreshold;
  if (currentThreshold < minThreshold)
  {
    newThreshold = minThreshold;
  }
  else if (currentThreshold > maxThreshold)
  {
    newThreshold = maxThreshold;
  }
  else
  {
    // Avoid rescaling altogether if the current threshold already preserves coverage
    return;
  }

  // Rescale alpha values
  float alphaScale = alphaThreshold / (newThreshold / 255.0f);
  for (plUInt64 idx = 0; idx < totalPixels; ++idx)
  {
    colors[idx].a *= alphaScale;
  }
}


plResult plImageUtils::Scale(const plImageView& source, plImage& target, plUInt32 width, plUInt32 height, const plImageFilter* filter, plImageAddressMode::Enum addressModeU, plImageAddressMode::Enum addressModeV, const plColor& borderColor)
{
  return Scale3D(source, target, width, height, 1, filter, addressModeU, addressModeV, plImageAddressMode::Clamp, borderColor);
}

plResult plImageUtils::Scale3D(const plImageView& source, plImage& target, plUInt32 width, plUInt32 height, plUInt32 depth, const plImageFilter* filter /*= pl_NULL*/, plImageAddressMode::Enum addressModeU /*= plImageAddressMode::Clamp*/,
  plImageAddressMode::Enum addressModeV /*= plImageAddressMode::Clamp*/, plImageAddressMode::Enum addressModeW /*= plImageAddressMode::Clamp*/, const plColor& borderColor /*= plColors::Black*/)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::Scale3D");

  if (width == 0 || height == 0 || depth == 0)
  {
    plImageHeader header;
    header.SetImageFormat(source.GetImageFormat());
    target.ResetAndAlloc(header);
    return PLASMA_SUCCESS;
  }

  const plImageFormat::Enum format = source.GetImageFormat();

  const plUInt32 originalWidth = source.GetWidth();
  const plUInt32 originalHeight = source.GetHeight();
  const plUInt32 originalDepth = source.GetDepth();
  const plUInt32 numFaces = source.GetNumFaces();
  const plUInt32 numArrayElements = source.GetNumArrayIndices();

  if (originalWidth == width && originalHeight == height && originalDepth == depth)
  {
    target.ResetAndCopy(source);
    return PLASMA_SUCCESS;
  }

  // Scaling down by an even factor?
  const plUInt32 downScaleFactorX = originalWidth / width;
  const plUInt32 downScaleFactorY = originalHeight / height;

  if (filter == nullptr && (format == plImageFormat::R8G8B8A8_UNORM || format == plImageFormat::B8G8R8A8_UNORM || format == plImageFormat::B8G8R8_UNORM) && downScaleFactorX * width == originalWidth && downScaleFactorY * height == originalHeight && depth == 1 && originalDepth == 1 &&
      plMath::IsPowerOf2(downScaleFactorX) && plMath::IsPowerOf2(downScaleFactorY))
  {
    DownScaleFast(source, target, width, height);
    return PLASMA_SUCCESS;
  }

  // Fallback to default filter
  plImageFilterTriangle defaultFilter;
  if (!filter)
  {
    filter = &defaultFilter;
  }

  const plImageView* stepSource;

  // Manage scratch images for intermediate conversion or filtering
  const plUInt32 maxNumScratchImages = 2;
  plImage scratch[maxNumScratchImages];
  bool scratchUsed[maxNumScratchImages] = {};
  auto allocateScratch = [&]() -> plImage&
  {
    for (plUInt32 i = 0;; ++i)
    {
      PLASMA_ASSERT_DEV(i < maxNumScratchImages, "Failed to allocate scratch image");
      if (!scratchUsed[i])
      {
        scratchUsed[i] = true;
        return scratch[i];
      }
    }
  };
  auto releaseScratch = [&](const plImageView& image)
  {
    for (plUInt32 i = 0; i < maxNumScratchImages; ++i)
    {
      if (&scratch[i] == &image)
      {
        scratchUsed[i] = false;
        return;
      }
    }
  };

  if (format == plImageFormat::R32G32B32A32_FLOAT)
  {
    stepSource = &source;
  }
  else
  {
    plImage& conversionScratch = allocateScratch();
    if (plImageConversion::Convert(source, conversionScratch, plImageFormat::R32G32B32A32_FLOAT).Failed())
    {
      return PLASMA_FAILURE;
    }

    stepSource = &conversionScratch;
  };

  plHybridArray<plInt32, 256> firstSampleIndices;
  firstSampleIndices.Reserve(plMath::Max(width, height, depth));

  if (width != originalWidth)
  {
    plImageFilterWeights weights(*filter, originalWidth, width);
    firstSampleIndices.SetCountUninitialized(width);
    for (plUInt32 x = 0; x < width; ++x)
    {
      firstSampleIndices[x] = weights.GetFirstSourceSampleIndex(x);
    }

    plImage* stepTarget;
    if (height == originalHeight && depth == originalDepth && format == plImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    plImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetWidth(width);
    stepTarget->ResetAndAlloc(stepHeader);

    for (plUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (plUInt32 face = 0; face < numFaces; ++face)
      {
        for (plUInt32 z = 0; z < originalDepth; ++z)
        {
          for (plUInt32 y = 0; y < originalHeight; ++y)
          {
            const plSimdVec4f* filterSource = stepSource->GetPixelPointer<plSimdVec4f>(0, face, arrayIndex, 0, y, z);
            plSimdVec4f* filterTarget = stepTarget->GetPixelPointer<plSimdVec4f>(0, face, arrayIndex, 0, y, z);
            FilterLine(originalWidth, filterSource, filterTarget, 1, weights, firstSampleIndices, addressModeU, plSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  if (height != originalHeight)
  {
    plImageFilterWeights weights(*filter, originalHeight, height);
    firstSampleIndices.SetCount(height);
    for (plUInt32 y = 0; y < height; ++y)
    {
      firstSampleIndices[y] = weights.GetFirstSourceSampleIndex(y);
    }

    plImage* stepTarget;
    if (depth == originalDepth && format == plImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    plImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetHeight(height);
    stepTarget->ResetAndAlloc(stepHeader);

    for (plUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (plUInt32 face = 0; face < numFaces; ++face)
      {
        for (plUInt32 z = 0; z < originalDepth; ++z)
        {
          for (plUInt32 x = 0; x < width; ++x)
          {
            const plSimdVec4f* filterSource = stepSource->GetPixelPointer<plSimdVec4f>(0, face, arrayIndex, x, 0, z);
            plSimdVec4f* filterTarget = stepTarget->GetPixelPointer<plSimdVec4f>(0, face, arrayIndex, x, 0, z);
            FilterLine(originalHeight, filterSource, filterTarget, width, weights, firstSampleIndices, addressModeV, plSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  if (depth != originalDepth)
  {
    plImageFilterWeights weights(*filter, originalDepth, depth);
    firstSampleIndices.SetCount(depth);
    for (plUInt32 z = 0; z < depth; ++z)
    {
      firstSampleIndices[z] = weights.GetFirstSourceSampleIndex(z);
    }

    plImage* stepTarget;
    if (format == plImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    plImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetDepth(depth);
    stepTarget->ResetAndAlloc(stepHeader);

    for (plUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (plUInt32 face = 0; face < numFaces; ++face)
      {
        for (plUInt32 y = 0; y < height; ++y)
        {
          for (plUInt32 x = 0; x < width; ++x)
          {
            const plSimdVec4f* filterSource = stepSource->GetPixelPointer<plSimdVec4f>(0, face, arrayIndex, x, y, 0);
            plSimdVec4f* filterTarget = stepTarget->GetPixelPointer<plSimdVec4f>(0, face, arrayIndex, x, y, 0);
            FilterLine(originalHeight, filterSource, filterTarget, width * height, weights, firstSampleIndices, addressModeW, plSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  // Convert back to original format - no-op if stepSource and target are the same
  return plImageConversion::Convert(*stepSource, target, format);
}

void plImageUtils::GenerateMipMaps(const plImageView& source, plImage& target, const MipMapOptions& options)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::GenerateMipMaps");

  plImageHeader header = source.GetHeader();
  PLASMA_ASSERT_DEV(header.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "The source image must be a RGBA 32-bit float format.");
  PLASMA_ASSERT_DEV(&source != &target, "Source and target must not be the same image.");

  // Make a local copy to be able to tweak some of the options
  plImageUtils::MipMapOptions mipMapOptions = options;

  // alpha thresholds with extreme values are not supported at the moment
  mipMapOptions.m_alphaThreshold = plMath::Clamp(mipMapOptions.m_alphaThreshold, 0.05f, 0.95f);

  // Enforce CLAMP addressing mode for cubemaps
  if (source.GetNumFaces() == 6)
  {
    mipMapOptions.m_addressModeU = plImageAddressMode::Clamp;
    mipMapOptions.m_addressModeV = plImageAddressMode::Clamp;
  }

  plUInt32 numMipMaps = header.ComputeNumberOfMipMaps();
  if (mipMapOptions.m_numMipMaps > 0 && mipMapOptions.m_numMipMaps < numMipMaps)
  {
    numMipMaps = mipMapOptions.m_numMipMaps;
  }
  header.SetNumMipLevels(numMipMaps);

  target.ResetAndAlloc(header);

  for (plUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (plUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      plImageHeader currentMipMapHeader = header;
      currentMipMapHeader.SetNumMipLevels(1);
      currentMipMapHeader.SetNumFaces(1);
      currentMipMapHeader.SetNumArrayIndices(1);

      auto sourceView = source.GetSubImageView(0, face, arrayIndex).GetByteBlobPtr();
      auto targetView = target.GetSubImageView(0, face, arrayIndex).GetByteBlobPtr();

      memcpy(targetView.GetPtr(), sourceView.GetPtr(), static_cast<size_t>(targetView.GetCount()));

      float targetCoverage = 0.0f;
      if (mipMapOptions.m_preserveCoverage)
      {
        targetCoverage = EvaluateAverageCoverage(source.GetSubImageView(0, face, arrayIndex).GetBlobPtr<plColor>(), mipMapOptions.m_alphaThreshold);
      }

      for (plUInt32 mipMapLevel = 0; mipMapLevel < numMipMaps - 1; mipMapLevel++)
      {
        plImageHeader nextMipMapHeader = currentMipMapHeader;
        nextMipMapHeader.SetWidth(plMath::Max(1u, nextMipMapHeader.GetWidth() / 2));
        nextMipMapHeader.SetHeight(plMath::Max(1u, nextMipMapHeader.GetHeight() / 2));
        nextMipMapHeader.SetDepth(plMath::Max(1u, nextMipMapHeader.GetDepth() / 2));

        auto sourceData = target.GetSubImageView(mipMapLevel, face, arrayIndex).GetByteBlobPtr();
        plImage currentMipMap;
        currentMipMap.ResetAndUseExternalStorage(currentMipMapHeader, sourceData);

        auto dstData = target.GetSubImageView(mipMapLevel + 1, face, arrayIndex).GetByteBlobPtr();
        plImage nextMipMap;
        nextMipMap.ResetAndUseExternalStorage(nextMipMapHeader, dstData);

        plImageUtils::Scale3D(currentMipMap, nextMipMap, nextMipMapHeader.GetWidth(), nextMipMapHeader.GetHeight(), nextMipMapHeader.GetDepth(), mipMapOptions.m_filter, mipMapOptions.m_addressModeU, mipMapOptions.m_addressModeV, mipMapOptions.m_addressModeW, mipMapOptions.m_borderColor)
          .IgnoreResult();

        if (mipMapOptions.m_preserveCoverage)
        {
          NormalizeCoverage(nextMipMap.GetBlobPtr<plColor>(), mipMapOptions.m_alphaThreshold, targetCoverage);
        }

        if (mipMapOptions.m_renormalizeNormals)
        {
          RenormalizeNormalMap(nextMipMap);
        }

        currentMipMapHeader = nextMipMapHeader;
      }
    }
  }
}

void plImageUtils::ReconstructNormalZ(plImage& image)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::ReconstructNormalZ");

  PLASMA_ASSERT_DEV(image.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  plSimdVec4f* cur = image.GetBlobPtr<plSimdVec4f>().GetPtr();
  plSimdVec4f* const end = image.GetBlobPtr<plSimdVec4f>().GetEndPtr();

  plSimdFloat oneScalar = 1.0f;

  plSimdVec4f two(2.0f);

  plSimdVec4f minusOne(-1.0f);

  plSimdVec4f half(0.5f);

  for (; cur < end; cur++)
  {
    plSimdVec4f normal;
    // unpack from [0,1] to [-1, 1]
    normal = plSimdVec4f::MulAdd(*cur, two, minusOne);

    // compute Z component
    normal.SetZ((oneScalar - normal.Dot<2>(normal)).GetSqrt());

    // pack back to [0,1]
    *cur = plSimdVec4f::MulAdd(half, normal, half);
  }
}

void plImageUtils::RenormalizeNormalMap(plImage& image)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::RenormalizeNormalMap");

  PLASMA_ASSERT_DEV(image.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  plSimdVec4f* start = image.GetBlobPtr<plSimdVec4f>().GetPtr();
  plSimdVec4f* const end = image.GetBlobPtr<plSimdVec4f>().GetEndPtr();

  plSimdVec4f two(2.0f);

  plSimdVec4f minusOne(-1.0f);

  plSimdVec4f half(0.5f);

  for (; start < end; start++)
  {
    plSimdVec4f normal;
    normal = plSimdVec4f::MulAdd(*start, two, minusOne);
    normal.Normalize<3>();
    *start = plSimdVec4f::MulAdd(half, normal, half);
  }
}

void plImageUtils::AdjustRoughness(plImage& roughnessMap, const plImageView& normalMap)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::AdjustRoughness");

  PLASMA_ASSERT_DEV(roughnessMap.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");
  PLASMA_ASSERT_DEV(normalMap.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  PLASMA_ASSERT_DEV(roughnessMap.GetWidth() >= normalMap.GetWidth() && roughnessMap.GetHeight() >= normalMap.GetHeight(), "The roughness map needs to be bigger or same size than the normal map.");

  plImage filteredNormalMap;
  plImageUtils::MipMapOptions options;

  // Box filter normal map without re-normalization so we have the average normal length in each mip map.
  if (roughnessMap.GetWidth() != normalMap.GetWidth() || roughnessMap.GetHeight() != normalMap.GetHeight())
  {
    plImage temp;
    plImageUtils::Scale(normalMap, temp, roughnessMap.GetWidth(), roughnessMap.GetHeight()).IgnoreResult();
    plImageUtils::RenormalizeNormalMap(temp);
    plImageUtils::GenerateMipMaps(temp, filteredNormalMap, options);
  }
  else
  {
    plImageUtils::GenerateMipMaps(normalMap, filteredNormalMap, options);
  }

  PLASMA_ASSERT_DEV(roughnessMap.GetNumMipLevels() == filteredNormalMap.GetNumMipLevels(), "Roughness and normal map must have the same number of mip maps");

  plSimdVec4f two(2.0f);
  plSimdVec4f minusOne(-1.0f);

  plUInt32 numMipLevels = roughnessMap.GetNumMipLevels();
  for (plUInt32 mipLevel = 1; mipLevel < numMipLevels; ++mipLevel)
  {
    plBlobPtr<plSimdVec4f> roughnessData = roughnessMap.GetSubImageView(mipLevel, 0, 0).GetBlobPtr<plSimdVec4f>();
    plBlobPtr<plSimdVec4f> normalData = filteredNormalMap.GetSubImageView(mipLevel, 0, 0).GetBlobPtr<plSimdVec4f>();

    for (plUInt64 i = 0; i < roughnessData.GetCount(); ++i)
    {
      plSimdVec4f normal = plSimdVec4f::MulAdd(normalData[i], two, minusOne);

      float avgNormalLength = normal.GetLength<3>();
      if (avgNormalLength < 1.0f)
      {
        float avgNormalLengthSquare = avgNormalLength * avgNormalLength;
        float kappa = (3.0f * avgNormalLength - avgNormalLength * avgNormalLengthSquare) / (1.0f - avgNormalLengthSquare);
        float variance = 1.0f / (2.0f * kappa);

        float oldRoughness = roughnessData[i].GetComponent<0>();
        float newRoughness = plMath::Sqrt(oldRoughness * oldRoughness + variance);

        roughnessData[i].Set(newRoughness);
      }
    }
  }
}

void plImageUtils::ChangeExposure(plImage& image, float bias)
{
  PLASMA_ASSERT_DEV(image.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "This function expects an RGBA 32 float image as input");

  if (bias == 0.0f)
    return;

  PLASMA_PROFILE_SCOPE("plImageUtils::ChangeExposure");

  const float multiplier = plMath::Pow2(bias);

  for (plColor& col : image.GetBlobPtr<plColor>())
  {
    col = multiplier * col;
  }
}

static plResult CopyImageRectToFace(plImage& dstImg, const plImageView& srcImg, plUInt32 offsetX, plUInt32 offsetY, plUInt32 faceIndex)
{
  plRectU32 r;
  r.x = offsetX;
  r.y = offsetY;
  r.width = dstImg.GetWidth();
  r.height = r.width;

  return plImageUtils::Copy(srcImg, r, dstImg, plVec3U32(0), 0, faceIndex);
}

plResult plImageUtils::CreateCubemapFromSingleFile(plImage& dstImg, const plImageView& srcImg)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::CreateCubemapFromSingleFile");

  if (srcImg.GetNumFaces() == 6)
  {
    dstImg.ResetAndCopy(srcImg);
    return PLASMA_SUCCESS;
  }
  else if (srcImg.GetNumFaces() == 1)
  {
    if (srcImg.GetWidth() % 3 == 0 && srcImg.GetHeight() % 4 == 0 && srcImg.GetWidth() / 3 == srcImg.GetHeight() / 4)
    {
      // Vertical cube map layout
      //     +---+
      //     | Y+|
      // +---+---+---+
      // | X-| Z+| X+|
      // +---+---+---+
      //     | Y-|
      //     +---+
      //     | Z-|
      //     +---+
      const plUInt32 faceSize = srcImg.GetWidth() / 3;

      plImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      dstImg.ResetAndAlloc(imgHeader);

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      PLASMA_SUCCEED_OR_RETURN(CopyImageRectToFace(dstImg, srcImg, faceSize * 2, faceSize, 0));

      // Negative X face
      PLASMA_SUCCEED_OR_RETURN(CopyImageRectToFace(dstImg, srcImg, 0, faceSize, 1));

      // Positive Y face
      PLASMA_SUCCEED_OR_RETURN(CopyImageRectToFace(dstImg, srcImg, faceSize, 0, 2));

      // Negative Y face
      PLASMA_SUCCEED_OR_RETURN(CopyImageRectToFace(dstImg, srcImg, faceSize, faceSize * 2, 3));

      // Positive Z face
      PLASMA_SUCCEED_OR_RETURN(CopyImageRectToFace(dstImg, srcImg, faceSize, faceSize, 4));

      // Negative Z face
      PLASMA_SUCCEED_OR_RETURN(CopyImageRectToFace(dstImg, srcImg, faceSize, faceSize * 3, 5));
      plImageUtils::RotateSubImage180(dstImg, 0, 5);
    }
    else if (srcImg.GetWidth() % 4 == 0 && srcImg.GetHeight() % 3 == 0 && srcImg.GetWidth() / 4 == srcImg.GetHeight() / 3)
    {
      // Horizontal cube map layout
      //     +---+
      //     | Y+|
      // +---+---+---+---+
      // | X-| Z+| X+| Z-|
      // +---+---+---+---+
      //     | Y-|
      //     +---+
      const plUInt32 faceSize = srcImg.GetWidth() / 4;

      plImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      dstImg.ResetAndAlloc(imgHeader);

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      PLASMA_SUCCEED_OR_RETURN(CopyImageRectToFace(dstImg, srcImg, faceSize * 2, faceSize, 0));

      // Negative X face
      PLASMA_SUCCEED_OR_RETURN(CopyImageRectToFace(dstImg, srcImg, 0, faceSize, 1));

      // Positive Y face
      PLASMA_SUCCEED_OR_RETURN(CopyImageRectToFace(dstImg, srcImg, faceSize, 0, 2));

      // Negative Y face
      PLASMA_SUCCEED_OR_RETURN(CopyImageRectToFace(dstImg, srcImg, faceSize, faceSize * 2, 3));

      // Positive Z face
      PLASMA_SUCCEED_OR_RETURN(CopyImageRectToFace(dstImg, srcImg, faceSize, faceSize, 4));

      // Negative Z face
      PLASMA_SUCCEED_OR_RETURN(CopyImageRectToFace(dstImg, srcImg, faceSize * 3, faceSize, 5));
    }
    else
    {
      // Spherical mapping
      if (srcImg.GetWidth() % 4 != 0)
      {
        plLog::Error("Width of the input image should be a multiple of 4");
        return PLASMA_FAILURE;
      }

      const plUInt32 faceSize = srcImg.GetWidth() / 4;

      plImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      dstImg.ResetAndAlloc(imgHeader);

      // Corners of the UV space for the respective faces in model space
      const plVec3 faceCorners[] = {
        plVec3(0.5, 0.5, 0.5),   // X+
        plVec3(-0.5, 0.5, -0.5), // X-
        plVec3(-0.5, 0.5, -0.5), // Y+
        plVec3(-0.5, -0.5, 0.5), // Y-
        plVec3(-0.5, 0.5, 0.5),  // Z+
        plVec3(0.5, 0.5, -0.5)   // Z-
      };

      // UV Axis of the respective faces in model space
      const plVec3 faceAxis[] = {
        plVec3(0, 0, -1), plVec3(0, -1, 0), // X+
        plVec3(0, 0, 1), plVec3(0, -1, 0),  // X-
        plVec3(1, 0, 0), plVec3(0, 0, 1),   // Y+
        plVec3(1, 0, 0), plVec3(0, 0, -1),  // Y-
        plVec3(1, 0, 0), plVec3(0, -1, 0),  // Z+
        plVec3(-1, 0, 0), plVec3(0, -1, 0)  // Z-
      };

      const float fFaceSize = (float)faceSize;
      const float fHalfPixel = 0.5f / fFaceSize;
      const float fPixel = 1.0f / fFaceSize;

      const float fHalfSrcWidth = srcImg.GetWidth() / 2.0f;
      const float fSrcHeight = (float)srcImg.GetHeight();

      const plUInt32 srcWidthMinus1 = srcImg.GetWidth() - 1;
      const plUInt32 srcHeightMinus1 = srcImg.GetHeight() - 1;

      PLASMA_ASSERT_DEBUG(srcImg.GetRowPitch() % sizeof(plColor) == 0, "Row pitch should be a multiple of sizeof(plColor)");
      const plUInt64 srcRowPitch = srcImg.GetRowPitch() / sizeof(plColor);

      PLASMA_ASSERT_DEBUG(dstImg.GetRowPitch() % sizeof(plColor) == 0, "Row pitch should be a multiple of sizeof(plColor)");
      const plUInt64 faceRowPitch = dstImg.GetRowPitch() / sizeof(plColor);

      const plColor* srcData = srcImg.GetPixelPointer<plColor>();
      const float InvPi = 1.0f / plMath::Pi<float>();

      for (plUInt32 faceIndex = 0; faceIndex < 6; faceIndex++)
      {
        plColor* faceData = dstImg.GetPixelPointer<plColor>(0, faceIndex);
        for (plUInt32 y = 0; y < faceSize; y++)
        {
          const float dstV = (float)y * fPixel + fHalfPixel;

          for (plUInt32 x = 0; x < faceSize; x++)
          {
            const float dstU = (float)x * fPixel + fHalfPixel;
            const plVec3 modelSpacePos = faceCorners[faceIndex] + dstU * faceAxis[faceIndex * 2] + dstV * faceAxis[faceIndex * 2 + 1];
            const plVec3 modelSpaceDir = modelSpacePos.GetNormalized();

            const float phi = plMath::ATan2(modelSpaceDir.x, modelSpaceDir.z).GetRadian() + plMath::Pi<float>();
            const float r = plMath::Sqrt(modelSpaceDir.x * modelSpaceDir.x + modelSpaceDir.z * modelSpaceDir.z);
            const float theta = plMath::ATan2(modelSpaceDir.y, r).GetRadian() + plMath::Pi<float>() * 0.5f;

            PLASMA_ASSERT_DEBUG(phi >= 0.0f && phi <= 2.0f * plMath::Pi<float>(), "");
            PLASMA_ASSERT_DEBUG(theta >= 0.0f && theta <= plMath::Pi<float>(), "");

            const float srcU = phi * InvPi * fHalfSrcWidth;
            const float srcV = (1.0f - theta * InvPi) * fSrcHeight;

            plUInt32 x1 = (plUInt32)plMath::Floor(srcU);
            plUInt32 x2 = x1 + 1;
            plUInt32 y1 = (plUInt32)plMath::Floor(srcV);
            plUInt32 y2 = y1 + 1;

            const float fracX = srcU - x1;
            const float fracY = srcV - y1;

            x1 = plMath::Clamp(x1, 0u, srcWidthMinus1);
            x2 = plMath::Clamp(x2, 0u, srcWidthMinus1);
            y1 = plMath::Clamp(y1, 0u, srcHeightMinus1);
            y2 = plMath::Clamp(y2, 0u, srcHeightMinus1);

            plColor A = srcData[x1 + y1 * srcRowPitch];
            plColor B = srcData[x2 + y1 * srcRowPitch];
            plColor C = srcData[x1 + y2 * srcRowPitch];
            plColor D = srcData[x2 + y2 * srcRowPitch];

            plColor interpolated = A * (1 - fracX) * (1 - fracY) + B * (fracX) * (1 - fracY) + C * (1 - fracX) * fracY + D * fracX * fracY;
            faceData[x + y * faceRowPitch] = interpolated;
          }
        }
      }
    }

    return PLASMA_SUCCESS;
  }

  plLog::Error("Unexpected number of faces in cubemap input image.");
  return PLASMA_FAILURE;
}

plResult plImageUtils::CreateCubemapFrom6Files(plImage& dstImg, const plImageView* pSourceImages)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::CreateCubemapFrom6Files");

  plImageHeader header = pSourceImages[0].GetHeader();
  header.SetNumFaces(6);

  if (header.GetWidth() != header.GetHeight())
    return PLASMA_FAILURE;

  if (!plMath::IsPowerOf2(header.GetWidth()))
    return PLASMA_FAILURE;

  dstImg.ResetAndAlloc(header);

  for (plUInt32 i = 0; i < 6; ++i)
  {
    if (pSourceImages[i].GetImageFormat() != dstImg.GetImageFormat())
      return PLASMA_FAILURE;

    if (pSourceImages[i].GetWidth() != dstImg.GetWidth())
      return PLASMA_FAILURE;

    if (pSourceImages[i].GetHeight() != dstImg.GetHeight())
      return PLASMA_FAILURE;

    PLASMA_SUCCEED_OR_RETURN(CopyImageRectToFace(dstImg, pSourceImages[i], 0, 0, i));
  }

  return PLASMA_SUCCESS;
}

plResult plImageUtils::CreateVolumeTextureFromSingleFile(plImage& dstImg, const plImageView& srcImg)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::CreateVolumeTextureFromSingleFile");

  const plUInt32 uiWidthHeight = srcImg.GetHeight();
  const plUInt32 uiDepth = srcImg.GetWidth() / uiWidthHeight;

  if (!plMath::IsPowerOf2(uiWidthHeight))
    return PLASMA_FAILURE;
  if (!plMath::IsPowerOf2(uiDepth))
    return PLASMA_FAILURE;

  plImageHeader header;
  header.SetWidth(uiWidthHeight);
  header.SetHeight(uiWidthHeight);
  header.SetDepth(uiDepth);
  header.SetImageFormat(srcImg.GetImageFormat());

  dstImg.ResetAndAlloc(header);

  const plImageView view = srcImg.GetSubImageView();

  for (plUInt32 d = 0; d < uiDepth; ++d)
  {
    plRectU32 r;
    r.x = uiWidthHeight * d;
    r.y = 0;
    r.width = uiWidthHeight;
    r.height = uiWidthHeight;

    PLASMA_SUCCEED_OR_RETURN(Copy(view, r, dstImg, plVec3U32(0, 0, d)));
  }

  return PLASMA_SUCCESS;
}

plColor plImageUtils::NearestSample(const plImageView& image, plImageAddressMode::Enum addressMode, plVec2 uv)
{
  PLASMA_ASSERT_DEBUG(image.GetDepth() == 1 && image.GetNumFaces() == 1 && image.GetNumArrayIndices() == 1, "Only 2d images are supported");
  PLASMA_ASSERT_DEBUG(image.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "Unsupported format");

  return NearestSample(image.GetPixelPointer<plColor>(), image.GetWidth(), image.GetHeight(), addressMode, uv);
}

plColor plImageUtils::NearestSample(const plColor* pPixelPointer, plUInt32 uiWidth, plUInt32 uiHeight, plImageAddressMode::Enum addressMode, plVec2 uv)
{
  const plInt32 w = uiWidth;
  const plInt32 h = uiHeight;

  uv = uv.CompMul(plVec2(static_cast<float>(w), static_cast<float>(h)));
  const plInt32 intX = (plInt32)plMath::Floor(uv.x);
  const plInt32 intY = (plInt32)plMath::Floor(uv.y);

  plInt32 x = intX;
  plInt32 y = intY;

  if (addressMode == plImageAddressMode::Clamp)
  {
    x = plMath::Clamp(x, 0, w - 1);
    y = plMath::Clamp(y, 0, h - 1);
  }
  else if (addressMode == plImageAddressMode::Repeat)
  {
    x = x % w;
    x = x < 0 ? x + w : x;
    y = y % h;
    y = y < 0 ? y + h : y;
  }
  else
  {
    PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  return *(pPixelPointer + (y * w) + x);
}

plColor plImageUtils::BilinearSample(const plImageView& image, plImageAddressMode::Enum addressMode, plVec2 uv)
{
  PLASMA_ASSERT_DEBUG(image.GetDepth() == 1 && image.GetNumFaces() == 1 && image.GetNumArrayIndices() == 1, "Only 2d images are supported");
  PLASMA_ASSERT_DEBUG(image.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "Unsupported format");

  return BilinearSample(image.GetPixelPointer<plColor>(), image.GetWidth(), image.GetHeight(), addressMode, uv);
}

plColor plImageUtils::BilinearSample(const plColor* pData, plUInt32 uiWidth, plUInt32 uiHeight, plImageAddressMode::Enum addressMode, plVec2 uv)
{
  plInt32 w = uiWidth;
  plInt32 h = uiHeight;

  uv = uv.CompMul(plVec2(static_cast<float>(w), static_cast<float>(h))) - plVec2(0.5f);
  const float floorX = plMath::Floor(uv.x);
  const float floorY = plMath::Floor(uv.y);
  const float fractionX = uv.x - floorX;
  const float fractionY = uv.y - floorY;
  const plInt32 intX = (plInt32)floorX;
  const plInt32 intY = (plInt32)floorY;

  plColor c[4];
  for (plUInt32 i = 0; i < 4; ++i)
  {
    plInt32 x = intX + (i % 2);
    plInt32 y = intY + (i / 2);

    if (addressMode == plImageAddressMode::Clamp)
    {
      x = plMath::Clamp(x, 0, w - 1);
      y = plMath::Clamp(y, 0, h - 1);
    }
    else if (addressMode == plImageAddressMode::Repeat)
    {
      x = x % w;
      x = x < 0 ? x + w : x;
      y = y % h;
      y = y < 0 ? y + h : y;
    }
    else
    {
      PLASMA_ASSERT_NOT_IMPLEMENTED;
    }

    c[i] = *(pData + (y * w) + x);
  }

  const plColor cr0 = plMath::Lerp(c[0], c[1], fractionX);
  const plColor cr1 = plMath::Lerp(c[2], c[3], fractionX);

  return plMath::Lerp(cr0, cr1, fractionY);
}

plResult plImageUtils::CopyChannel(plImage& dstImg, plUInt8 uiDstChannelIdx, const plImage& srcImg, plUInt8 uiSrcChannelIdx)
{
  PLASMA_PROFILE_SCOPE("plImageUtils::CopyChannel");

  if (uiSrcChannelIdx >= 4 || uiDstChannelIdx >= 4)
    return PLASMA_FAILURE;

  if (dstImg.GetImageFormat() != plImageFormat::R32G32B32A32_FLOAT)
    return PLASMA_FAILURE;

  if (srcImg.GetImageFormat() != dstImg.GetImageFormat())
    return PLASMA_FAILURE;

  if (srcImg.GetWidth() != dstImg.GetWidth())
    return PLASMA_FAILURE;

  if (srcImg.GetHeight() != dstImg.GetHeight())
    return PLASMA_FAILURE;

  const plUInt32 uiNumPixels = srcImg.GetWidth() * srcImg.GetHeight();
  const float* pSrcPixel = srcImg.GetPixelPointer<float>();
  float* pDstPixel = dstImg.GetPixelPointer<float>();

  pSrcPixel += uiSrcChannelIdx;
  pDstPixel += uiDstChannelIdx;

  for (plUInt32 i = 0; i < uiNumPixels; ++i)
  {
    *pDstPixel = *pSrcPixel;

    pSrcPixel += 4;
    pDstPixel += 4;
  }

  return PLASMA_SUCCESS;
}

PLASMA_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageUtils);

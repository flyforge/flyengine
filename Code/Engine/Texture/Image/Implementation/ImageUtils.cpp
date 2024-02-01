#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageUtils.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Time/Timestamp.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/Image/ImageFilter.h>

template <typename TYPE>
static void SetDiff(const plImageView& imageA, const plImageView& imageB, plImage& out_difference, plUInt32 w, plUInt32 h, plUInt32 d, plUInt32 uiComp)
{
  const TYPE* pA = imageA.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  const TYPE* pB = imageB.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  TYPE* pR = out_difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  for (plUInt32 i = 0; i < uiComp; ++i)
    pR[i] = pB[i] > pA[i] ? (pB[i] - pA[i]) : (pA[i] - pB[i]);
}

template <typename TYPE, typename ACCU, int COMP>
static void SetCompMinDiff(const plImageView& newDifference, plImage& out_minDifference, plUInt32 w, plUInt32 h, plUInt32 d, plUInt32 uiComp)
{
  const TYPE* pNew = newDifference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  TYPE* pR = out_minDifference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  for (plUInt32 i = 0; i < uiComp; i += COMP)
  {
    ACCU minDiff = 0;
    ACCU newDiff = 0;
    for (plUInt32 c = 0; c < COMP; c++)
    {
      minDiff += pR[i + c];
      newDiff += pNew[i + c];
    }
    if (minDiff > newDiff)
    {
      for (plUInt32 c = 0; c < COMP; c++)
        pR[i + c] = pNew[i + c];
    }
  }
}

template <typename TYPE>
static plUInt32 GetError(const plImageView& difference, plUInt32 w, plUInt32 h, plUInt32 d, plUInt32 uiComp, plUInt32 uiPixel)
{
  const TYPE* pR = difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  plUInt32 uiErrorSum = 0;

  for (plUInt32 p = 0; p < uiPixel; ++p)
  {
    plUInt32 error = 0;

    for (plUInt32 c = 0; c < uiComp; ++c)
    {
      error += *pR;
      ++pR;
    }

    error /= uiComp;
    uiErrorSum += error * error;
  }

  return uiErrorSum;
}

void plImageUtils::ComputeImageDifferenceABS(const plImageView& imageA, const plImageView& imageB, plImage& out_difference)
{
  PL_PROFILE_SCOPE("plImageUtils::ComputeImageDifferenceABS");

  PL_ASSERT_DEV(imageA.GetWidth() == imageB.GetWidth(), "Dimensions do not match");
  PL_ASSERT_DEV(imageA.GetHeight() == imageB.GetHeight(), "Dimensions do not match");
  PL_ASSERT_DEV(imageA.GetDepth() == imageB.GetDepth(), "Dimensions do not match");
  PL_ASSERT_DEV(imageA.GetImageFormat() == imageB.GetImageFormat(), "Format does not match");

  plImageHeader differenceHeader;

  differenceHeader.SetWidth(imageA.GetWidth());
  differenceHeader.SetHeight(imageA.GetHeight());
  differenceHeader.SetDepth(imageA.GetDepth());
  differenceHeader.SetImageFormat(imageA.GetImageFormat());
  out_difference.ResetAndAlloc(differenceHeader);

  const plUInt32 uiSize2D = imageA.GetHeight() * imageA.GetWidth();

  for (plUInt32 d = 0; d < imageA.GetDepth(); ++d)
  {
    // for (plUInt32 h = 0; h < ImageA.GetHeight(); ++h)
    {
      // for (plUInt32 w = 0; w < ImageA.GetWidth(); ++w)
      {
        switch (imageA.GetImageFormat())
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
            SetDiff<plUInt8>(imageA, imageB, out_difference, 0, 0, d, 4 * uiSize2D);
          }
          break;

          case plImageFormat::B8G8R8_UNORM:
          {
            SetDiff<plUInt8>(imageA, imageB, out_difference, 0, 0, d, 3 * uiSize2D);
          }
          break;

          default:
            PL_REPORT_FAILURE("The plImageFormat {0} is not implemented", (plUInt32)imageA.GetImageFormat());
            return;
        }
      }
    }
  }
}


void plImageUtils::ComputeImageDifferenceABSRelaxed(const plImageView& imageA, const plImageView& imageB, plImage& out_difference)
{
  PL_ASSERT_ALWAYS(imageA.GetDepth() == 1 && imageA.GetNumMipLevels() == 1, "Depth slices and mipmaps are not supported");

  PL_PROFILE_SCOPE("plImageUtils::ComputeImageDifferenceABSRelaxed");

  ComputeImageDifferenceABS(imageA, imageB, out_difference);

  plImage tempB;
  tempB.ResetAndCopy(imageB);
  plImage tempDiff;
  tempDiff.ResetAndCopy(out_difference);

  for (plInt32 yOffset = -1; yOffset <= 1; ++yOffset)
  {
    for (plInt32 xOffset = -1; xOffset <= 1; ++xOffset)
    {
      if (yOffset == 0 && xOffset == 0)
        continue;

      plImageUtils::Copy(imageB, plRectU32(plMath::Max(xOffset, 0), plMath::Max(yOffset, 0), imageB.GetWidth() - plMath::Abs(xOffset), imageB.GetHeight() - plMath::Abs(yOffset)), tempB, plVec3U32(-plMath::Min(xOffset, 0), -plMath::Min(yOffset, 0), 0)).AssertSuccess("");

      ComputeImageDifferenceABS(imageA, tempB, tempDiff);

      const plUInt32 uiSize2D = imageA.GetHeight() * imageA.GetWidth();
      switch (imageA.GetImageFormat())
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
          SetCompMinDiff<plUInt8, plUInt32, 4>(tempDiff, out_difference, 0, 0, 0, 4 * uiSize2D);
        }
        break;

        case plImageFormat::B8G8R8_UNORM:
        {
          SetCompMinDiff<plUInt8, plUInt32, 3>(tempDiff, out_difference, 0, 0, 0, 3 * uiSize2D);
        }
        break;

        default:
          PL_REPORT_FAILURE("The plImageFormat {0} is not implemented", (plUInt32)imageA.GetImageFormat());
          return;
      }
    }
  }
}

plUInt32 plImageUtils::ComputeMeanSquareError(const plImageView& differenceImage, plUInt8 uiBlockSize, plUInt32 uiOffsetx, plUInt32 uiOffsety)
{
  PL_PROFILE_SCOPE("plImageUtils::ComputeMeanSquareError(detail)");

  PL_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  plUInt32 uiNumComponents = plImageFormat::GetNumChannels(differenceImage.GetImageFormat());

  plUInt32 uiWidth = plMath::Min(differenceImage.GetWidth(), uiOffsetx + uiBlockSize) - uiOffsetx;
  plUInt32 uiHeight = plMath::Min(differenceImage.GetHeight(), uiOffsety + uiBlockSize) - uiOffsety;

  // Treat image as single-component format and scale the width instead
  uiWidth *= uiNumComponents;

  if (uiWidth == 0 || uiHeight == 0)
    return 0;

  switch (differenceImage.GetImageFormat())
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
      PL_REPORT_FAILURE("The plImageFormat {0} is not implemented", (plUInt32)differenceImage.GetImageFormat());
      return 0;
  }


  plUInt32 error = 0;

  plUInt64 uiRowPitch = differenceImage.GetRowPitch();
  plUInt64 uiDepthPitch = differenceImage.GetDepthPitch();

  const plUInt32 uiSize2D = uiWidth * uiHeight;
  const plUInt8* pSlicePointer = differenceImage.GetPixelPointer<plUInt8>(0, 0, 0, uiOffsetx, uiOffsety);

  for (plUInt32 d = 0; d < differenceImage.GetDepth(); ++d)
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

plUInt32 plImageUtils::ComputeMeanSquareError(const plImageView& differenceImage, plUInt8 uiBlockSize)
{
  PL_PROFILE_SCOPE("plImageUtils::ComputeMeanSquareError");

  PL_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  const plUInt32 uiHalfBlockSize = uiBlockSize / 2;

  const plUInt32 uiBlocksX = (differenceImage.GetWidth() / uiHalfBlockSize) + 1;
  const plUInt32 uiBlocksY = (differenceImage.GetHeight() / uiHalfBlockSize) + 1;

  plUInt32 uiMaxError = 0;

  for (plUInt32 by = 0; by < uiBlocksY; ++by)
  {
    for (plUInt32 bx = 0; bx < uiBlocksX; ++bx)
    {
      const plUInt32 uiBlockError = ComputeMeanSquareError(differenceImage, uiBlockSize, bx * uiHalfBlockSize, by * uiHalfBlockSize);

      uiMaxError = plMath::Max(uiMaxError, uiBlockError);
    }
  }

  return uiMaxError;
}

template <typename Func, typename ImageType>
static void ApplyFunc(ImageType& inout_image, Func func)
{
  plUInt32 uiWidth = inout_image.GetWidth();
  plUInt32 uiHeight = inout_image.GetHeight();
  plUInt32 uiDepth = inout_image.GetDepth();

  PL_IGNORE_UNUSED(uiDepth);
  PL_ASSERT_DEV(uiWidth > 0 && uiHeight > 0 && uiDepth > 0, "The image passed to FindMinMax has illegal dimension {}x{}x{}.", uiWidth, uiHeight, uiDepth);

  plUInt64 uiRowPitch = inout_image.GetRowPitch();
  plUInt64 uiDepthPitch = inout_image.GetDepthPitch();
  plUInt32 uiNumChannels = plImageFormat::GetNumChannels(inout_image.GetImageFormat());

  auto pSlicePointer = inout_image.template GetPixelPointer<plUInt8>();

  for (plUInt32 z = 0; z < inout_image.GetDepth(); ++z)
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

static void FindMinMax(const plImageView& image, plUInt8& out_uiMinRgb, plUInt8& out_uiMaxRgb, plUInt8& out_uiMinAlpha, plUInt8& out_uiMaxAlpha)
{
  plImageFormat::Enum imageFormat = image.GetImageFormat();
  PL_IGNORE_UNUSED(imageFormat);
  PL_ASSERT_DEV(plImageFormat::GetBitsPerChannel(imageFormat, plImageFormatChannel::R) == 8 && plImageFormat::GetDataType(imageFormat) == plImageFormatDataType::UNORM, "Only 8bpp unorm formats are supported in FindMinMax");

  out_uiMinRgb = 255u;
  out_uiMinAlpha = 255u;
  out_uiMaxRgb = 0u;
  out_uiMaxAlpha = 0u;

  auto minMax = [&](const plUInt8* pPixel, plUInt32 /*x*/, plUInt32 /*y*/, plUInt32 /*z*/, plUInt32 c) {
    plUInt8 val = *pPixel;

    if (c < 3)
    {
      out_uiMinRgb = plMath::Min(out_uiMinRgb, val);
      out_uiMaxRgb = plMath::Max(out_uiMaxRgb, val);
    }
    else
    {
      out_uiMinAlpha = plMath::Min(out_uiMinAlpha, val);
      out_uiMaxAlpha = plMath::Max(out_uiMaxAlpha, val);
    }
  };
  ApplyFunc(image, minMax);
}

void plImageUtils::Normalize(plImage& inout_image)
{
  plUInt8 uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha;
  Normalize(inout_image, uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha);
}

void plImageUtils::Normalize(plImage& inout_image, plUInt8& out_uiMinRgb, plUInt8& out_uiMaxRgb, plUInt8& out_uiMinAlpha, plUInt8& out_uiMaxAlpha)
{
  PL_PROFILE_SCOPE("plImageUtils::Normalize");

  plImageFormat::Enum imageFormat = inout_image.GetImageFormat();

  PL_ASSERT_DEV(plImageFormat::GetBitsPerChannel(imageFormat, plImageFormatChannel::R) == 8 && plImageFormat::GetDataType(imageFormat) == plImageFormatDataType::UNORM, "Only 8bpp unorm formats are supported in NormalizeImage");

  bool ignoreAlpha = false;
  if (imageFormat == plImageFormat::B8G8R8X8_UNORM || imageFormat == plImageFormat::B8G8R8X8_UNORM_SRGB)
  {
    ignoreAlpha = true;
  }

  FindMinMax(inout_image, out_uiMinRgb, out_uiMaxRgb, out_uiMinAlpha, out_uiMaxAlpha);
  plUInt8 uiRangeRgb = out_uiMaxRgb - out_uiMinRgb;
  plUInt8 uiRangeAlpha = out_uiMaxAlpha - out_uiMinAlpha;

  auto normalize = [&](plUInt8* pPixel, plUInt32 /*x*/, plUInt32 /*y*/, plUInt32 /*z*/, plUInt32 c) {
    plUInt8 val = *pPixel;
    if (c < 3)
    {
      // color channels are uniform when min == max, in that case keep original value as scaling is not meaningful
      if (uiRangeRgb != 0)
      {
        *pPixel = static_cast<plUInt8>(255u * (static_cast<float>(val - out_uiMinRgb) / (uiRangeRgb)));
      }
    }
    else
    {
      // alpha is uniform when minAlpha == maxAlpha, in that case keep original alpha as scaling is not meaningful
      if (!ignoreAlpha && uiRangeAlpha != 0)
      {
        *pPixel = static_cast<plUInt8>(255u * (static_cast<float>(val - out_uiMinAlpha) / (uiRangeAlpha)));
      }
    }
  };
  ApplyFunc(inout_image, normalize);
}

void plImageUtils::ExtractAlphaChannel(const plImageView& inputImage, plImage& inout_outputImage)
{
  PL_PROFILE_SCOPE("plImageUtils::ExtractAlphaChannel");

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
      PL_REPORT_FAILURE("ExtractAlpha needs an image with 8bpp and 4 channel. The plImageFormat {} is not supported.", (plUInt32)imageFormat);
      return;
  }

  plImageHeader outputHeader = inputImage.GetHeader();
  outputHeader.SetImageFormat(plImageFormat::R8_UNORM);
  inout_outputImage.ResetAndAlloc(outputHeader);

  const plUInt8* pInputSlice = inputImage.GetPixelPointer<plUInt8>();
  plUInt8* pOutputSlice = inout_outputImage.GetPixelPointer<plUInt8>();

  plUInt64 uiInputRowPitch = inputImage.GetRowPitch();
  plUInt64 uiInputDepthPitch = inputImage.GetDepthPitch();

  plUInt64 uiOutputRowPitch = inout_outputImage.GetRowPitch();
  plUInt64 uiOutputDepthPitch = inout_outputImage.GetDepthPitch();

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

void plImageUtils::CropImage(const plImageView& input, const plVec2I32& vOffset, const plSizeU32& newsize, plImage& out_output)
{
  PL_PROFILE_SCOPE("plImageUtils::CropImage");

  PL_ASSERT_DEV(vOffset.x >= 0, "Offset is invalid");
  PL_ASSERT_DEV(vOffset.y >= 0, "Offset is invalid");
  PL_ASSERT_DEV(vOffset.x < (plInt32)input.GetWidth(), "Offset is invalid");
  PL_ASSERT_DEV(vOffset.y < (plInt32)input.GetHeight(), "Offset is invalid");

  const plUInt32 uiNewWidth = plMath::Min(vOffset.x + newsize.width, input.GetWidth()) - vOffset.x;
  const plUInt32 uiNewHeight = plMath::Min(vOffset.y + newsize.height, input.GetHeight()) - vOffset.y;

  plImageHeader outputHeader;
  outputHeader.SetWidth(uiNewWidth);
  outputHeader.SetHeight(uiNewHeight);
  outputHeader.SetImageFormat(input.GetImageFormat());
  out_output.ResetAndAlloc(outputHeader);

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
          out_output.GetPixelPointer<plUInt32>(0, 0, 0, x, y)[0] = input.GetPixelPointer<plUInt32>(0, 0, 0, vOffset.x + x, vOffset.y + y)[0];
          break;

        case plImageFormat::B8G8R8_UNORM:
          out_output.GetPixelPointer<plUInt8>(0, 0, 0, x, y)[0] = input.GetPixelPointer<plUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[0];
          out_output.GetPixelPointer<plUInt8>(0, 0, 0, x, y)[1] = input.GetPixelPointer<plUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[1];
          out_output.GetPixelPointer<plUInt8>(0, 0, 0, x, y)[2] = input.GetPixelPointer<plUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[2];
          break;

        default:
          PL_REPORT_FAILURE("The plImageFormat {0} is not implemented", (plUInt32)input.GetImageFormat());
          return;
      }
    }
  }
}

namespace
{
  template <typename T>
  void rotate180(T* pStart, T* pEnd)
  {
    pEnd = pEnd - 1;
    while (pStart < pEnd)
    {
      plMath::Swap(*pStart, *pEnd);
      pStart++;
      pEnd--;
    }
  }
} // namespace

void plImageUtils::RotateSubImage180(plImage& inout_image, plUInt32 uiMipLevel /*= 0*/, plUInt32 uiFace /*= 0*/, plUInt32 uiArrayIndex /*= 0*/)
{
  PL_PROFILE_SCOPE("plImageUtils::RotateSubImage180");

  plUInt8* start = inout_image.GetPixelPointer<plUInt8>(uiMipLevel, uiFace, uiArrayIndex);
  plUInt8* end = start + inout_image.GetDepthPitch(uiMipLevel);

  plUInt32 bytesPerPixel = plImageFormat::GetBitsPerPixel(inout_image.GetImageFormat()) / 8;

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

plResult plImageUtils::Copy(const plImageView& srcImg, const plRectU32& srcRect, plImage& inout_dstImg, const plVec3U32& vDstOffset, plUInt32 uiDstMipLevel /*= 0*/, plUInt32 uiDstFace /*= 0*/, plUInt32 uiDstArrayIndex /*= 0*/)
{
  if (inout_dstImg.GetImageFormat() != srcImg.GetImageFormat()) // Can only copy when the image formats are identical
    return PL_FAILURE;

  if (plImageFormat::IsCompressed(inout_dstImg.GetImageFormat())) // Compressed formats are not supported
    return PL_FAILURE;

  PL_PROFILE_SCOPE("plImageUtils::Copy");

  const plUInt64 uiDstRowPitch = inout_dstImg.GetRowPitch(uiDstMipLevel);
  const plUInt64 uiSrcRowPitch = srcImg.GetRowPitch(uiDstMipLevel);
  const plUInt32 uiCopyBytesPerRow = plImageFormat::GetBitsPerPixel(srcImg.GetImageFormat()) * srcRect.width / 8;

  plUInt8* dstPtr = inout_dstImg.GetPixelPointer<plUInt8>(uiDstMipLevel, uiDstFace, uiDstArrayIndex, vDstOffset.x, vDstOffset.y, vDstOffset.z);
  const plUInt8* srcPtr = srcImg.GetPixelPointer<plUInt8>(0, 0, 0, srcRect.x, srcRect.y);

  for (plUInt32 y = 0; y < srcRect.height; y++)
  {
    plMemoryUtils::Copy(dstPtr, srcPtr, uiCopyBytesPerRow);

    dstPtr += uiDstRowPitch;
    srcPtr += uiSrcRowPitch;
  }

  return PL_SUCCESS;
}

plResult plImageUtils::ExtractLowerMipChain(const plImageView& srcImg, plImage& ref_dstImg, plUInt32 uiNumMips)
{
  const plImageHeader& srcImgHeader = srcImg.GetHeader();

  if (srcImgHeader.GetNumFaces() != 1 || srcImgHeader.GetNumArrayIndices() != 1)
  {
    // Lower mips aren't stored contiguously for array/cube textures and would require copying. This isn't implemented yet.
    return PL_FAILURE;
  }

  PL_PROFILE_SCOPE("plImageUtils::ExtractLowerMipChain");

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
        return PL_FAILURE;

      if (startMipLevel == 0)
        return PL_FAILURE;

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

  ref_dstImg.ResetAndCopy(dataview);

  return PL_SUCCESS;
}

plUInt32 plImageUtils::GetSampleIndex(plUInt32 uiNumTexels, plInt32 iIndex, plImageAddressMode::Enum addressMode, bool& out_bUseBorderColor)
{
  out_bUseBorderColor = false;
  if (plUInt32(iIndex) >= uiNumTexels)
  {
    switch (addressMode)
    {
      case plImageAddressMode::Repeat:
        iIndex %= uiNumTexels;

        if (iIndex < 0)
        {
          iIndex += uiNumTexels;
        }
        return iIndex;

      case plImageAddressMode::Mirror:
      {
        if (iIndex < 0)
        {
          iIndex = -iIndex - 1;
        }
        bool flip = (iIndex / uiNumTexels) & 1;
        iIndex %= uiNumTexels;
        if (flip)
        {
          iIndex = uiNumTexels - iIndex - 1;
        }
        return iIndex;
      }

      case plImageAddressMode::Clamp:
        return plMath::Clamp<plInt32>(iIndex, 0, uiNumTexels - 1);

      case plImageAddressMode::ClampBorder:
        out_bUseBorderColor = true;
        return 0;

      default:
        PL_ASSERT_NOT_IMPLEMENTED
        return 0;
    }
  }
  return iIndex;
}

static plSimdVec4f LoadSample(const plSimdVec4f* pSource, plUInt32 uiNumSourceElements, plUInt32 uiStride, plInt32 iIndex, plImageAddressMode::Enum addressMode, const plSimdVec4f& vBorderColor)
{
  bool useBorderColor = false;
  // result is in the range [-(w-1), (w-1)], bring it to [0, w - 1]
  iIndex = plImageUtils::GetSampleIndex(uiNumSourceElements, iIndex, addressMode, useBorderColor);
  if (useBorderColor)
  {
    return vBorderColor;
  }
  return pSource[iIndex * uiStride];
}

inline static void FilterLine(
  plUInt32 uiNumSourceElements, const plSimdVec4f* __restrict pSourceBegin, plSimdVec4f* __restrict pTargetBegin, plUInt32 uiStride, const plImageFilterWeights& weights, plArrayPtr<const plInt32> firstSampleIndices, plImageAddressMode::Enum addressMode, const plSimdVec4f& vBorderColor)
{
  // Convolve the image using the precomputed weights
  const plUInt32 numWeights = weights.GetNumWeights();

  // When the first source index for the output is between 0 and this value,
  // we can fetch all numWeights inputs without taking addressMode into consideration,
  // which makes the inner loop a lot faster.
  const plInt32 trivialSourceIndicesEnd = static_cast<plInt32>(uiNumSourceElements) - static_cast<plInt32>(numWeights);
  const auto weightsView = weights.ViewWeights();
  const float* __restrict nextWeightPtr = weightsView.GetPtr();
  PL_ASSERT_DEBUG((static_cast<plUInt32>(weightsView.GetCount()) % numWeights) == 0, "");
  for (plInt32 firstSourceIdx : firstSampleIndices)
  {
    plSimdVec4f total(0.0f, 0.0f, 0.0f, 0.0f);

    if (firstSourceIdx >= 0 && firstSourceIdx < trivialSourceIndicesEnd)
    {
      const auto* __restrict sourcePtr = pSourceBegin + firstSourceIdx * uiStride;
      for (plUInt32 weightIdx = 0; weightIdx < numWeights; ++weightIdx)
      {
        total = plSimdVec4f::MulAdd(*sourcePtr, plSimdVec4f(*nextWeightPtr++), total);
        sourcePtr += uiStride;
      }
    }
    else
    {
      // Very slow fallback case that respects the addressMode
      // (not a lot of pixels are taking this path, so it's probably fine)
      plInt32 sourceIdx = firstSourceIdx;
      for (plUInt32 weightIdx = 0; weightIdx < numWeights; ++weightIdx)
      {
        total = plSimdVec4f::MulAdd(LoadSample(pSourceBegin, uiNumSourceElements, uiStride, sourceIdx, addressMode, vBorderColor), plSimdVec4f(*nextWeightPtr++), total);
        sourceIdx++;
      }
    }
    // It's ok to check this once per source index, see the assert above
    // (number of weights in weightsView is divisible by numWeights)
    if (nextWeightPtr == weightsView.GetEndPtr())
    {
      nextWeightPtr = weightsView.GetPtr();
    }
    *pTargetBegin = total;
    pTargetBegin += uiStride;
  }
}

static void DownScaleFastLine(plUInt32 uiPixelStride, const plUInt8* pSrc, plUInt8* pDest, plUInt32 uiLengthIn, plUInt32 uiStrideIn, plUInt32 uiLengthOut, plUInt32 uiStrideOut)
{
  const plUInt32 downScaleFactor = uiLengthIn / uiLengthOut;
  PL_ASSERT_DEBUG(downScaleFactor >= 1, "Can't upscale");

  const plUInt32 downScaleFactorLog2 = plMath::Log2i(static_cast<plUInt32>(downScaleFactor));
  const plUInt32 roundOffset = downScaleFactor / 2;

  for (plUInt32 offset = 0; offset < uiLengthOut; ++offset)
  {
    for (plUInt32 channel = 0; channel < uiPixelStride; ++channel)
    {
      const plUInt32 destOffset = offset * uiStrideOut + channel;

      plUInt32 curChannel = roundOffset;
      for (plUInt32 index = 0; index < downScaleFactor; ++index)
      {
        curChannel += static_cast<plUInt32>(pSrc[channel + index * uiStrideIn]);
      }

      curChannel = curChannel >> downScaleFactorLog2;
      pDest[destOffset] = static_cast<plUInt8>(curChannel);
    }

    pSrc += downScaleFactor * uiStrideIn;
  }
}

static void DownScaleFast(const plImageView& image, plImage& out_result, plUInt32 uiWidth, plUInt32 uiHeight)
{
  plImageFormat::Enum format = image.GetImageFormat();

  plUInt32 originalWidth = image.GetWidth();
  plUInt32 originalHeight = image.GetHeight();
  plUInt32 numArrayElements = image.GetNumArrayIndices();
  plUInt32 numFaces = image.GetNumFaces();

  plUInt32 pixelStride = plImageFormat::GetBitsPerPixel(format) / 8;

  plImageHeader intermediateHeader;
  intermediateHeader.SetWidth(uiWidth);
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
        DownScaleFastLine(pixelStride, image.GetPixelPointer<plUInt8>(0, face, arrayIndex, 0, row), intermediate.GetPixelPointer<plUInt8>(0, face, arrayIndex, 0, row), originalWidth, pixelStride, uiWidth, pixelStride);
      }
    }
  }

  // input and output images may be the same, so we can't access the original image below this point

  plImageHeader outHeader;
  outHeader.SetWidth(uiWidth);
  outHeader.SetHeight(uiHeight);
  outHeader.SetNumArrayIndices(numArrayElements);
  outHeader.SetNumArrayIndices(numFaces);
  outHeader.SetImageFormat(format);

  out_result.ResetAndAlloc(outHeader);

  PL_ASSERT_DEBUG(intermediate.GetRowPitch() < plMath::MaxValue<plUInt32>(), "Row pitch exceeds plUInt32 max value.");
  PL_ASSERT_DEBUG(out_result.GetRowPitch() < plMath::MaxValue<plUInt32>(), "Row pitch exceeds plUInt32 max value.");

  for (plUInt32 arrayIndex = 0; arrayIndex < numArrayElements; arrayIndex++)
  {
    for (plUInt32 face = 0; face < numFaces; face++)
    {
      for (plUInt32 col = 0; col < uiWidth; col++)
      {
        DownScaleFastLine(pixelStride, intermediate.GetPixelPointer<plUInt8>(0, face, arrayIndex, col), out_result.GetPixelPointer<plUInt8>(0, face, arrayIndex, col), originalHeight, static_cast<plUInt32>(intermediate.GetRowPitch()), uiHeight, static_cast<plUInt32>(out_result.GetRowPitch()));
      }
    }
  }
}

static float EvaluateAverageCoverage(plBlobPtr<const plColor> colors, float fAlphaThreshold)
{
  PL_PROFILE_SCOPE("EvaluateAverageCoverage");

  plUInt64 totalPixels = colors.GetCount();
  plUInt64 count = 0;
  for (plUInt32 idx = 0; idx < totalPixels; ++idx)
  {
    count += colors[idx].a >= fAlphaThreshold;
  }

  return float(count) / float(totalPixels);
}

static void NormalizeCoverage(plBlobPtr<plColor> colors, float fAlphaThreshold, float fTargetCoverage)
{
  PL_PROFILE_SCOPE("NormalizeCoverage");

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
  plInt32 targetCount = plInt32(fTargetCoverage * totalPixels);
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

  plInt32 currentThreshold = plMath::ColorFloatToByte(fAlphaThreshold);

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
  float alphaScale = fAlphaThreshold / (newThreshold / 255.0f);
  for (plUInt64 idx = 0; idx < totalPixels; ++idx)
  {
    colors[idx].a *= alphaScale;
  }
}


plResult plImageUtils::Scale(const plImageView& source, plImage& ref_target, plUInt32 uiWidth, plUInt32 uiHeight, const plImageFilter* pFilter, plImageAddressMode::Enum addressModeU, plImageAddressMode::Enum addressModeV, const plColor& borderColor)
{
  return Scale3D(source, ref_target, uiWidth, uiHeight, 1, pFilter, addressModeU, addressModeV, plImageAddressMode::Clamp, borderColor);
}

plResult plImageUtils::Scale3D(const plImageView& source, plImage& ref_target, plUInt32 uiWidth, plUInt32 uiHeight, plUInt32 uiDepth, const plImageFilter* pFilter /*= PL_NULL*/, plImageAddressMode::Enum addressModeU /*= plImageAddressMode::Clamp*/,
  plImageAddressMode::Enum addressModeV /*= plImageAddressMode::Clamp*/, plImageAddressMode::Enum addressModeW /*= plImageAddressMode::Clamp*/, const plColor& borderColor /*= plColors::Black*/)
{
  PL_PROFILE_SCOPE("plImageUtils::Scale3D");

  if (uiWidth == 0 || uiHeight == 0 || uiDepth == 0)
  {
    plImageHeader header;
    header.SetImageFormat(source.GetImageFormat());
    ref_target.ResetAndAlloc(header);
    return PL_SUCCESS;
  }

  const plImageFormat::Enum format = source.GetImageFormat();

  const plUInt32 originalWidth = source.GetWidth();
  const plUInt32 originalHeight = source.GetHeight();
  const plUInt32 originalDepth = source.GetDepth();
  const plUInt32 numFaces = source.GetNumFaces();
  const plUInt32 numArrayElements = source.GetNumArrayIndices();

  if (originalWidth == uiWidth && originalHeight == uiHeight && originalDepth == uiDepth)
  {
    ref_target.ResetAndCopy(source);
    return PL_SUCCESS;
  }

  // Scaling down by an even factor?
  const plUInt32 downScaleFactorX = originalWidth / uiWidth;
  const plUInt32 downScaleFactorY = originalHeight / uiHeight;

  if (pFilter == nullptr && (format == plImageFormat::R8G8B8A8_UNORM || format == plImageFormat::B8G8R8A8_UNORM || format == plImageFormat::B8G8R8_UNORM) && downScaleFactorX * uiWidth == originalWidth && downScaleFactorY * uiHeight == originalHeight && uiDepth == 1 && originalDepth == 1 &&
      plMath::IsPowerOf2(downScaleFactorX) && plMath::IsPowerOf2(downScaleFactorY))
  {
    DownScaleFast(source, ref_target, uiWidth, uiHeight);
    return PL_SUCCESS;
  }

  // Fallback to default filter
  plImageFilterTriangle defaultFilter;
  if (!pFilter)
  {
    pFilter = &defaultFilter;
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
      PL_ASSERT_DEV(i < maxNumScratchImages, "Failed to allocate scratch image");
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
      return PL_FAILURE;
    }

    stepSource = &conversionScratch;
  };

  plHybridArray<plInt32, 256> firstSampleIndices;
  firstSampleIndices.Reserve(plMath::Max(uiWidth, uiHeight, uiDepth));

  if (uiWidth != originalWidth)
  {
    plImageFilterWeights weights(*pFilter, originalWidth, uiWidth);
    firstSampleIndices.SetCountUninitialized(uiWidth);
    for (plUInt32 x = 0; x < uiWidth; ++x)
    {
      firstSampleIndices[x] = weights.GetFirstSourceSampleIndex(x);
    }

    plImage* stepTarget;
    if (uiHeight == originalHeight && uiDepth == originalDepth && format == plImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    plImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetWidth(uiWidth);
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

  if (uiHeight != originalHeight)
  {
    plImageFilterWeights weights(*pFilter, originalHeight, uiHeight);
    firstSampleIndices.SetCount(uiHeight);
    for (plUInt32 y = 0; y < uiHeight; ++y)
    {
      firstSampleIndices[y] = weights.GetFirstSourceSampleIndex(y);
    }

    plImage* stepTarget;
    if (uiDepth == originalDepth && format == plImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    plImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetHeight(uiHeight);
    stepTarget->ResetAndAlloc(stepHeader);

    for (plUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (plUInt32 face = 0; face < numFaces; ++face)
      {
        for (plUInt32 z = 0; z < originalDepth; ++z)
        {
          for (plUInt32 x = 0; x < uiWidth; ++x)
          {
            const plSimdVec4f* filterSource = stepSource->GetPixelPointer<plSimdVec4f>(0, face, arrayIndex, x, 0, z);
            plSimdVec4f* filterTarget = stepTarget->GetPixelPointer<plSimdVec4f>(0, face, arrayIndex, x, 0, z);
            FilterLine(originalHeight, filterSource, filterTarget, uiWidth, weights, firstSampleIndices, addressModeV, plSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  if (uiDepth != originalDepth)
  {
    plImageFilterWeights weights(*pFilter, originalDepth, uiDepth);
    firstSampleIndices.SetCount(uiDepth);
    for (plUInt32 z = 0; z < uiDepth; ++z)
    {
      firstSampleIndices[z] = weights.GetFirstSourceSampleIndex(z);
    }

    plImage* stepTarget;
    if (format == plImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    plImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetDepth(uiDepth);
    stepTarget->ResetAndAlloc(stepHeader);

    for (plUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (plUInt32 face = 0; face < numFaces; ++face)
      {
        for (plUInt32 y = 0; y < uiHeight; ++y)
        {
          for (plUInt32 x = 0; x < uiWidth; ++x)
          {
            const plSimdVec4f* filterSource = stepSource->GetPixelPointer<plSimdVec4f>(0, face, arrayIndex, x, y, 0);
            plSimdVec4f* filterTarget = stepTarget->GetPixelPointer<plSimdVec4f>(0, face, arrayIndex, x, y, 0);
            FilterLine(originalHeight, filterSource, filterTarget, uiWidth * uiHeight, weights, firstSampleIndices, addressModeW, plSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  // Convert back to original format - no-op if stepSource and target are the same
  return plImageConversion::Convert(*stepSource, ref_target, format);
}

void plImageUtils::GenerateMipMaps(const plImageView& source, plImage& ref_target, const MipMapOptions& options)
{
  PL_PROFILE_SCOPE("plImageUtils::GenerateMipMaps");

  plImageHeader header = source.GetHeader();
  PL_ASSERT_DEV(header.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "The source image must be a RGBA 32-bit float format.");
  PL_ASSERT_DEV(&source != &ref_target, "Source and target must not be the same image.");

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

  ref_target.ResetAndAlloc(header);

  for (plUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (plUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      plImageHeader currentMipMapHeader = header;
      currentMipMapHeader.SetNumMipLevels(1);
      currentMipMapHeader.SetNumFaces(1);
      currentMipMapHeader.SetNumArrayIndices(1);

      auto sourceView = source.GetSubImageView(0, face, arrayIndex).GetByteBlobPtr();
      auto targetView = ref_target.GetSubImageView(0, face, arrayIndex).GetByteBlobPtr();

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

        auto sourceData = ref_target.GetSubImageView(mipMapLevel, face, arrayIndex).GetByteBlobPtr();
        plImage currentMipMap;
        currentMipMap.ResetAndUseExternalStorage(currentMipMapHeader, sourceData);

        auto dstData = ref_target.GetSubImageView(mipMapLevel + 1, face, arrayIndex).GetByteBlobPtr();
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

void plImageUtils::ReconstructNormalZ(plImage& ref_image)
{
  PL_PROFILE_SCOPE("plImageUtils::ReconstructNormalZ");

  PL_ASSERT_DEV(ref_image.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  plSimdVec4f* cur = ref_image.GetBlobPtr<plSimdVec4f>().GetPtr();
  plSimdVec4f* const end = ref_image.GetBlobPtr<plSimdVec4f>().GetEndPtr();

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

void plImageUtils::RenormalizeNormalMap(plImage& ref_image)
{
  PL_PROFILE_SCOPE("plImageUtils::RenormalizeNormalMap");

  PL_ASSERT_DEV(ref_image.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  plSimdVec4f* start = ref_image.GetBlobPtr<plSimdVec4f>().GetPtr();
  plSimdVec4f* const end = ref_image.GetBlobPtr<plSimdVec4f>().GetEndPtr();

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

void plImageUtils::AdjustRoughness(plImage& ref_roughnessMap, const plImageView& normalMap)
{
  PL_PROFILE_SCOPE("plImageUtils::AdjustRoughness");

  PL_ASSERT_DEV(ref_roughnessMap.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");
  PL_ASSERT_DEV(normalMap.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  PL_ASSERT_DEV(ref_roughnessMap.GetWidth() >= normalMap.GetWidth() && ref_roughnessMap.GetHeight() >= normalMap.GetHeight(), "The roughness map needs to be bigger or same size than the normal map.");

  plImage filteredNormalMap;
  plImageUtils::MipMapOptions options;

  // Box filter normal map without re-normalization so we have the average normal length in each mip map.
  if (ref_roughnessMap.GetWidth() != normalMap.GetWidth() || ref_roughnessMap.GetHeight() != normalMap.GetHeight())
  {
    plImage temp;
    plImageUtils::Scale(normalMap, temp, ref_roughnessMap.GetWidth(), ref_roughnessMap.GetHeight()).IgnoreResult();
    plImageUtils::RenormalizeNormalMap(temp);
    plImageUtils::GenerateMipMaps(temp, filteredNormalMap, options);
  }
  else
  {
    plImageUtils::GenerateMipMaps(normalMap, filteredNormalMap, options);
  }

  PL_ASSERT_DEV(ref_roughnessMap.GetNumMipLevels() == filteredNormalMap.GetNumMipLevels(), "Roughness and normal map must have the same number of mip maps");

  plSimdVec4f two(2.0f);
  plSimdVec4f minusOne(-1.0f);

  plUInt32 numMipLevels = ref_roughnessMap.GetNumMipLevels();
  for (plUInt32 mipLevel = 1; mipLevel < numMipLevels; ++mipLevel)
  {
    plBlobPtr<plSimdVec4f> roughnessData = ref_roughnessMap.GetSubImageView(mipLevel, 0, 0).GetBlobPtr<plSimdVec4f>();
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

void plImageUtils::ChangeExposure(plImage& ref_image, float fBias)
{
  PL_ASSERT_DEV(ref_image.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "This function expects an RGBA 32 float image as input");

  if (fBias == 0.0f)
    return;

  PL_PROFILE_SCOPE("plImageUtils::ChangeExposure");

  const float multiplier = plMath::Pow2(fBias);

  for (plColor& col : ref_image.GetBlobPtr<plColor>())
  {
    col = multiplier * col;
  }
}

static plResult CopyImageRectToFace(plImage& ref_dstImg, const plImageView& srcImg, plUInt32 uiOffsetX, plUInt32 uiOffsetY, plUInt32 uiFaceIndex)
{
  plRectU32 r;
  r.x = uiOffsetX;
  r.y = uiOffsetY;
  r.width = ref_dstImg.GetWidth();
  r.height = r.width;

  return plImageUtils::Copy(srcImg, r, ref_dstImg, plVec3U32(0), 0, uiFaceIndex);
}

plResult plImageUtils::CreateCubemapFromSingleFile(plImage& ref_dstImg, const plImageView& srcImg)
{
  PL_PROFILE_SCOPE("plImageUtils::CreateCubemapFromSingleFile");

  if (srcImg.GetNumFaces() == 6)
  {
    ref_dstImg.ResetAndCopy(srcImg);
    return PL_SUCCESS;
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

      ref_dstImg.ResetAndAlloc(imgHeader);

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      PL_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 2, faceSize, 0));

      // Negative X face
      PL_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, 0, faceSize, 1));

      // Positive Y face
      PL_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, 0, 2));

      // Negative Y face
      PL_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 2, 3));

      // Positive Z face
      PL_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize, 4));

      // Negative Z face
      PL_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 3, 5));
      plImageUtils::RotateSubImage180(ref_dstImg, 0, 5);
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

      ref_dstImg.ResetAndAlloc(imgHeader);

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      PL_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 2, faceSize, 0));

      // Negative X face
      PL_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, 0, faceSize, 1));

      // Positive Y face
      PL_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, 0, 2));

      // Negative Y face
      PL_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 2, 3));

      // Positive Z face
      PL_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize, 4));

      // Negative Z face
      PL_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 3, faceSize, 5));
    }
    else
    {
      // Spherical mapping
      if (srcImg.GetWidth() % 4 != 0)
      {
        plLog::Error("Width of the input image should be a multiple of 4");
        return PL_FAILURE;
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

      ref_dstImg.ResetAndAlloc(imgHeader);

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

      PL_ASSERT_DEBUG(srcImg.GetRowPitch() % sizeof(plColor) == 0, "Row pitch should be a multiple of sizeof(plColor)");
      const plUInt64 srcRowPitch = srcImg.GetRowPitch() / sizeof(plColor);

      PL_ASSERT_DEBUG(ref_dstImg.GetRowPitch() % sizeof(plColor) == 0, "Row pitch should be a multiple of sizeof(plColor)");
      const plUInt64 faceRowPitch = ref_dstImg.GetRowPitch() / sizeof(plColor);

      const plColor* srcData = srcImg.GetPixelPointer<plColor>();
      const float InvPi = 1.0f / plMath::Pi<float>();

      for (plUInt32 faceIndex = 0; faceIndex < 6; faceIndex++)
      {
        plColor* faceData = ref_dstImg.GetPixelPointer<plColor>(0, faceIndex);
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

            PL_ASSERT_DEBUG(phi >= 0.0f && phi <= 2.0f * plMath::Pi<float>(), "");
            PL_ASSERT_DEBUG(theta >= 0.0f && theta <= plMath::Pi<float>(), "");

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

    return PL_SUCCESS;
  }

  plLog::Error("Unexpected number of faces in cubemap input image.");
  return PL_FAILURE;
}

plResult plImageUtils::CreateCubemapFrom6Files(plImage& ref_dstImg, const plImageView* pSourceImages)
{
  PL_PROFILE_SCOPE("plImageUtils::CreateCubemapFrom6Files");

  plImageHeader header = pSourceImages[0].GetHeader();
  header.SetNumFaces(6);

  if (header.GetWidth() != header.GetHeight())
    return PL_FAILURE;

  if (!plMath::IsPowerOf2(header.GetWidth()))
    return PL_FAILURE;

  ref_dstImg.ResetAndAlloc(header);

  for (plUInt32 i = 0; i < 6; ++i)
  {
    if (pSourceImages[i].GetImageFormat() != ref_dstImg.GetImageFormat())
      return PL_FAILURE;

    if (pSourceImages[i].GetWidth() != ref_dstImg.GetWidth())
      return PL_FAILURE;

    if (pSourceImages[i].GetHeight() != ref_dstImg.GetHeight())
      return PL_FAILURE;

    PL_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, pSourceImages[i], 0, 0, i));
  }

  return PL_SUCCESS;
}

plResult plImageUtils::CreateVolumeTextureFromSingleFile(plImage& ref_dstImg, const plImageView& srcImg)
{
  PL_PROFILE_SCOPE("plImageUtils::CreateVolumeTextureFromSingleFile");

  const plUInt32 uiWidthHeight = srcImg.GetHeight();
  const plUInt32 uiDepth = srcImg.GetWidth() / uiWidthHeight;

  if (!plMath::IsPowerOf2(uiWidthHeight))
    return PL_FAILURE;
  if (!plMath::IsPowerOf2(uiDepth))
    return PL_FAILURE;

  plImageHeader header;
  header.SetWidth(uiWidthHeight);
  header.SetHeight(uiWidthHeight);
  header.SetDepth(uiDepth);
  header.SetImageFormat(srcImg.GetImageFormat());

  ref_dstImg.ResetAndAlloc(header);

  const plImageView view = srcImg.GetSubImageView();

  for (plUInt32 d = 0; d < uiDepth; ++d)
  {
    plRectU32 r;
    r.x = uiWidthHeight * d;
    r.y = 0;
    r.width = uiWidthHeight;
    r.height = uiWidthHeight;

    PL_SUCCEED_OR_RETURN(Copy(view, r, ref_dstImg, plVec3U32(0, 0, d)));
  }

  return PL_SUCCESS;
}

plColor plImageUtils::NearestSample(const plImageView& image, plImageAddressMode::Enum addressMode, plVec2 vUv)
{
  PL_ASSERT_DEBUG(image.GetDepth() == 1 && image.GetNumFaces() == 1 && image.GetNumArrayIndices() == 1, "Only 2d images are supported");
  PL_ASSERT_DEBUG(image.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "Unsupported format");

  return NearestSample(image.GetPixelPointer<plColor>(), image.GetWidth(), image.GetHeight(), addressMode, vUv);
}

plColor plImageUtils::NearestSample(const plColor* pPixelPointer, plUInt32 uiWidth, plUInt32 uiHeight, plImageAddressMode::Enum addressMode, plVec2 vUv)
{
  const plInt32 w = uiWidth;
  const plInt32 h = uiHeight;

  vUv = vUv.CompMul(plVec2(static_cast<float>(w), static_cast<float>(h)));
  const plInt32 intX = (plInt32)plMath::Floor(vUv.x);
  const plInt32 intY = (plInt32)plMath::Floor(vUv.y);

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
    PL_ASSERT_NOT_IMPLEMENTED;
  }

  return *(pPixelPointer + (y * w) + x);
}

plColor plImageUtils::BilinearSample(const plImageView& image, plImageAddressMode::Enum addressMode, plVec2 vUv)
{
  PL_ASSERT_DEBUG(image.GetDepth() == 1 && image.GetNumFaces() == 1 && image.GetNumArrayIndices() == 1, "Only 2d images are supported");
  PL_ASSERT_DEBUG(image.GetImageFormat() == plImageFormat::R32G32B32A32_FLOAT, "Unsupported format");

  return BilinearSample(image.GetPixelPointer<plColor>(), image.GetWidth(), image.GetHeight(), addressMode, vUv);
}

plColor plImageUtils::BilinearSample(const plColor* pData, plUInt32 uiWidth, plUInt32 uiHeight, plImageAddressMode::Enum addressMode, plVec2 vUv)
{
  plInt32 w = uiWidth;
  plInt32 h = uiHeight;

  vUv = vUv.CompMul(plVec2(static_cast<float>(w), static_cast<float>(h))) - plVec2(0.5f);
  const float floorX = plMath::Floor(vUv.x);
  const float floorY = plMath::Floor(vUv.y);
  const float fractionX = vUv.x - floorX;
  const float fractionY = vUv.y - floorY;
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
      PL_ASSERT_NOT_IMPLEMENTED;
    }

    c[i] = *(pData + (y * w) + x);
  }

  const plColor cr0 = plMath::Lerp(c[0], c[1], fractionX);
  const plColor cr1 = plMath::Lerp(c[2], c[3], fractionX);

  return plMath::Lerp(cr0, cr1, fractionY);
}

plResult plImageUtils::CopyChannel(plImage& ref_dstImg, plUInt8 uiDstChannelIdx, const plImage& srcImg, plUInt8 uiSrcChannelIdx)
{
  PL_PROFILE_SCOPE("plImageUtils::CopyChannel");

  if (uiSrcChannelIdx >= 4 || uiDstChannelIdx >= 4)
    return PL_FAILURE;

  if (ref_dstImg.GetImageFormat() != plImageFormat::R32G32B32A32_FLOAT)
    return PL_FAILURE;

  if (srcImg.GetImageFormat() != ref_dstImg.GetImageFormat())
    return PL_FAILURE;

  if (srcImg.GetWidth() != ref_dstImg.GetWidth())
    return PL_FAILURE;

  if (srcImg.GetHeight() != ref_dstImg.GetHeight())
    return PL_FAILURE;

  const plUInt32 uiNumPixels = srcImg.GetWidth() * srcImg.GetHeight();
  const float* pSrcPixel = srcImg.GetPixelPointer<float>();
  float* pDstPixel = ref_dstImg.GetPixelPointer<float>();

  pSrcPixel += uiSrcChannelIdx;
  pDstPixel += uiDstChannelIdx;

  for (plUInt32 i = 0; i < uiNumPixels; ++i)
  {
    *pDstPixel = *pSrcPixel;

    pSrcPixel += 4;
    pDstPixel += 4;
  }

  return PL_SUCCESS;
}

static const plUInt8 s_Base64EncodingTable[64] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
  'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

static const plUInt8 BASE64_CHARS_PER_LINE = 76;

static plUInt32 GetBase64EncodedLength(plUInt32 uiInputLength, bool bInsertLineBreaks)
{
  plUInt32 outputLength = (uiInputLength + 2) / 3 * 4;

  if (bInsertLineBreaks)
  {
    outputLength += outputLength / BASE64_CHARS_PER_LINE;
  }

  return outputLength;
}

static plDynamicArray<char> ArrayToBase64(plArrayPtr<const plUInt8> in, bool bInsertLineBreaks = true)
{
  plDynamicArray<char> out;
  out.SetCountUninitialized(GetBase64EncodedLength(in.GetCount(), bInsertLineBreaks));

  plUInt32 offsetIn = 0;
  plUInt32 offsetOut = 0;

  plUInt32 blocksTillNewline = BASE64_CHARS_PER_LINE / 4;
  while (offsetIn < in.GetCount())
  {
    plUInt8 ibuf[3] = {0};

    plUInt32 ibuflen = plMath::Min(in.GetCount() - offsetIn, 3u);

    for (plUInt32 i = 0; i < ibuflen; ++i)
    {
      ibuf[i] = in[offsetIn++];
    }

    char obuf[4];
    obuf[0] = s_Base64EncodingTable[(ibuf[0] >> 2)];
    obuf[1] = s_Base64EncodingTable[((ibuf[0] << 4) & 0x30) | (ibuf[1] >> 4)];
    obuf[2] = s_Base64EncodingTable[((ibuf[1] << 2) & 0x3c) | (ibuf[2] >> 6)];
    obuf[3] = s_Base64EncodingTable[(ibuf[2] & 0x3f)];

    if (ibuflen >= 3)
    {
      out[offsetOut++] = obuf[0];
      out[offsetOut++] = obuf[1];
      out[offsetOut++] = obuf[2];
      out[offsetOut++] = obuf[3];
    }
    else // need to pad up to 4
    {
      switch (ibuflen)
      {
        case 1:
          out[offsetOut++] = obuf[0];
          out[offsetOut++] = obuf[1];
          out[offsetOut++] = '=';
          out[offsetOut++] = '=';
          break;
        case 2:
          out[offsetOut++] = obuf[0];
          out[offsetOut++] = obuf[1];
          out[offsetOut++] = obuf[2];
          out[offsetOut++] = '=';
          break;
      }
    }

    if (--blocksTillNewline == 0)
    {
      if (bInsertLineBreaks)
      {
        out[offsetOut++] = '\n';
      }
      blocksTillNewline = 19;
    }
  }

  PL_ASSERT_DEV(offsetOut == out.GetCount(), "All output data should have been written");
  return out;
}

void plImageUtils::EmbedImageData(plStringBuilder& out_sHtml, const plImage& image)
{
  plImageFileFormat* format = plImageFileFormat::GetWriterFormat("png");
  PL_ASSERT_DEV(format != nullptr, "No PNG writer found");

  plDynamicArray<plUInt8> imgData;
  plMemoryStreamContainerWrapperStorage<plDynamicArray<plUInt8>> storage(&imgData);
  plMemoryStreamWriter writer(&storage);
  format->WriteImage(writer, image, "png").IgnoreResult();

  plDynamicArray<char> imgDataBase64 = ArrayToBase64(imgData.GetArrayPtr());
  plStringView imgDataBase64StringView(imgDataBase64.GetArrayPtr().GetPtr(), imgDataBase64.GetArrayPtr().GetEndPtr());
  out_sHtml.AppendFormat("data:image/png;base64,{0}", imgDataBase64StringView);
}

void plImageUtils::CreateImageDiffHtml(plStringBuilder& out_sHtml, plStringView sTitle, const plImage& referenceImgRgb, const plImage& referenceImgAlpha, const plImage& capturedImgRgb, const plImage& capturedImgAlpha, const plImage& diffImgRgb, const plImage& diffImgAlpha, plUInt32 uiError, plUInt32 uiThreshold, plUInt8 uiMinDiffRgb, plUInt8 uiMaxDiffRgb, plUInt8 uiMinDiffAlpha, plUInt8 uiMaxDiffAlpha)
{
  plStringBuilder& output = out_sHtml;
  output.Append("<!DOCTYPE html PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
                "<!DOCTYPE html PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
                "<HTML> <HEAD>\n");

  output.AppendFormat("<TITLE>{}</TITLE>\n", sTitle);
  output.Append("<script type = \"text/javascript\">\n"
                "function showReferenceImage()\n"
                "{\n"
                "    document.getElementById('image_current_rgb').style.display = 'none'\n"
                "    document.getElementById('image_current_a').style.display = 'none'\n"
                "    document.getElementById('image_reference_rgb').style.display = 'inline-block'\n"
                "    document.getElementById('image_reference_a').style.display = 'inline-block'\n"
                "    document.getElementById('image_caption_rgb').innerHTML = 'Displaying: Reference Image RGB'\n"
                "    document.getElementById('image_caption_a').innerHTML = 'Displaying: Reference Image Alpha'\n"
                "}\n"
                "function showCurrentImage()\n"
                "{\n"
                "    document.getElementById('image_current_rgb').style.display = 'inline-block'\n"
                "    document.getElementById('image_current_a').style.display = 'inline-block'\n"
                "    document.getElementById('image_reference_rgb').style.display = 'none'\n"
                "    document.getElementById('image_reference_a').style.display = 'none'\n"
                "    document.getElementById('image_caption_rgb').innerHTML = 'Displaying: Current Image RGB'\n"
                "    document.getElementById('image_caption_a').innerHTML = 'Displaying: Current Image Alpha'\n"
                "}\n"
                "function imageover()\n"
                "{\n"
                "    var mode = document.querySelector('input[name=\"image_interaction_mode\"]:checked').value\n"
                "    if (mode == 'interactive')\n"
                "    {\n"
                "        showReferenceImage()\n"
                "    }\n"
                "}\n"
                "function imageout()\n"
                "{\n"
                "    var mode = document.querySelector('input[name=\"image_interaction_mode\"]:checked').value\n"
                "    if (mode == 'interactive')\n"
                "    {\n"
                "        showCurrentImage()\n"
                "    }\n"
                "}\n"
                "function handleModeClick(clickedItem)\n"
                "{\n"
                "    if (clickedItem.value == 'current_image' || clickedItem.value == 'interactive')\n"
                "    {\n"
                "        showCurrentImage()\n"
                "    }\n"
                "    else if (clickedItem.value == 'reference_image')\n"
                "    {\n"
                "        showReferenceImage()\n"
                "    }\n"
                "}\n"
                "</script>\n"
                "</HEAD>\n"
                "<BODY bgcolor=\"#ccdddd\">\n"
                "<div style=\"line-height: 1.5; margin-top: 0px; margin-left: 10px; font-family: sans-serif;\">\n");

  output.AppendFormat("<b>Test result for \"{}\" from ", sTitle);
  plDateTime dateTime = plDateTime::MakeFromTimestamp(plTimestamp::CurrentTimestamp());
  output.AppendFormat("{}-{}-{} {}:{}:{}</b><br>\n", dateTime.GetYear(), plArgI(dateTime.GetMonth(), 2, true), plArgI(dateTime.GetDay(), 2, true), plArgI(dateTime.GetHour(), 2, true), plArgI(dateTime.GetMinute(), 2, true), plArgI(dateTime.GetSecond(), 2, true));

  output.Append("<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\">\n");

  output.Append("<!-- STATS-TABLE-START -->\n");

  output.AppendFormat("<tr>\n"
                      "<td>Error metric:</td>\n"
                      "<td align=\"right\" style=\"padding-left: 2em;\">{}</td>\n"
                      "</tr>\n",
    uiError);
  output.AppendFormat("<tr>\n"
                      "<td>Error threshold:</td>\n"
                      "<td align=\"right\" style=\"padding-left: 2em;\">{}</td>\n"
                      "</tr>\n",
    uiThreshold);

  output.Append("<!-- STATS-TABLE-END -->\n");

  output.Append("</table>\n"
                "<div style=\"margin-top: 0.5em; margin-bottom: -0.75em\">\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"interactive\" "
                "checked=\"checked\"> Mouse-Over Image Switching\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"current_image\"> "
                "Current Image\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"reference_image\"> "
                "Reference Image\n"
                "</div>\n");

  output.AppendFormat("<div style=\"width:{}px;display: inline-block;\">\n", capturedImgRgb.GetWidth());

  output.Append("<p id=\"image_caption_rgb\">Displaying: Current Image RGB</p>\n"

                "<div style=\"block;\" onmouseover=\"imageover()\" onmouseout=\"imageout()\">\n"
                "<img id=\"image_current_rgb\" alt=\"Captured Image RGB\" src=\"");
  EmbedImageData(output, capturedImgRgb);
  output.Append("\" />\n"
                "<img id=\"image_reference_rgb\" style=\"display: none\" alt=\"Reference Image RGB\" src=\"");
  EmbedImageData(output, referenceImgRgb);
  output.Append("\" />\n"
                "</div>\n"
                "<div style=\"display: block;\">\n");
  output.AppendFormat("<p>RGB Difference (min: {}, max: {}):</p>\n", uiMinDiffRgb, uiMaxDiffRgb);
  output.Append("<img alt=\"Diff Image RGB\" src=\"");
  EmbedImageData(output, diffImgRgb);
  output.Append("\" />\n"
                "</div>\n"
                "</div>\n");

  output.AppendFormat("<div style=\"width:{}px;display: inline-block;\">\n", capturedImgAlpha.GetWidth());

  output.Append("<p id=\"image_caption_a\">Displaying: Current Image Alpha</p>\n"
                "<div style=\"display: block;\" onmouseover=\"imageover()\" onmouseout=\"imageout()\">\n"
                "<img id=\"image_current_a\" alt=\"Captured Image Alpha\" src=\"");
  EmbedImageData(output, capturedImgAlpha);
  output.Append("\" />\n"
                "<img id=\"image_reference_a\" style=\"display: none\" alt=\"Reference Image Alpha\" src=\"");
  EmbedImageData(output, referenceImgAlpha);
  output.Append("\" />\n"
                "</div>\n"
                "<div style=\"px;display: block;\">\n");
  output.AppendFormat("<p>Alpha Difference (min: {}, max: {}):</p>\n", uiMinDiffAlpha, uiMaxDiffAlpha);
  output.Append("<img alt=\"Diff Image Alpha\" src=\"");
  EmbedImageData(output, diffImgAlpha);
  output.Append("\" />\n"
                "</div>\n"
                "</div>\n"
                "</div>\n"
                "</BODY> </HTML>");
}

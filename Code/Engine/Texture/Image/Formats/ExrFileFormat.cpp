#include <Texture/TexturePCH.h>

#if PL_DISABLED(PL_PLATFORM_WINDOWS_UWP)

#  include <Texture/Image/Formats/ExrFileFormat.h>
#  include <Texture/Image/Image.h>

#  include <Foundation/IO/MemoryStream.h>
#  include <Foundation/IO/StreamUtils.h>
#  include <Foundation/Profiling/Profiling.h>

#  include <tinyexr/tinyexr.h>

// PL_STATICLINK_FORCE
plExrFileFormat g_ExrFileFormat;

plResult ReadImageData(plStreamReader& ref_stream, plDynamicArray<plUInt8>& ref_fileBuffer, plImageHeader& ref_header, EXRHeader& ref_exrHeader, EXRImage& ref_exrImage)
{
  // read the entire file to memory
  plStreamUtils::ReadAllAndAppend(ref_stream, ref_fileBuffer);

  // read the EXR version
  EXRVersion exrVersion;

  if (ParseEXRVersionFromMemory(&exrVersion, ref_fileBuffer.GetData(), ref_fileBuffer.GetCount()) != 0)
  {
    plLog::Error("Invalid EXR file: Cannot read version.");
    return PL_FAILURE;
  }

  if (exrVersion.multipart)
  {
    plLog::Error("Invalid EXR file: Multi-part formats are not supported.");
    return PL_FAILURE;
  }

  // read the EXR header
  const char* err = nullptr;
  if (ParseEXRHeaderFromMemory(&ref_exrHeader, &exrVersion, ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &err) != 0)
  {
    plLog::Error("Invalid EXR file: '{0}'", err);
    FreeEXRErrorMessage(err);
    return PL_FAILURE;
  }

  for (int c = 1; c < ref_exrHeader.num_channels; ++c)
  {
    if (ref_exrHeader.pixel_types[c - 1] != ref_exrHeader.pixel_types[c])
    {
      plLog::Error("Unsupported EXR file: all channels should have the same size.");
      break;
    }
  }

  if (LoadEXRImageFromMemory(&ref_exrImage, &ref_exrHeader, ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &err) != 0)
  {
    plLog::Error("Invalid EXR file: '{0}'", err);

    FreeEXRHeader(&ref_exrHeader);
    FreeEXRErrorMessage(err);
    return PL_FAILURE;
  }

  plImageFormat::Enum imageFormat = plImageFormat::UNKNOWN;

  switch (ref_exrHeader.num_channels)
  {
    case 1:
    {
      switch (ref_exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = plImageFormat::R32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = plImageFormat::R16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = plImageFormat::R32_UINT;
          break;
      }

      break;
    }

    case 2:
    {
      switch (ref_exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = plImageFormat::R32G32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = plImageFormat::R16G16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = plImageFormat::R32G32_UINT;
          break;
      }

      break;
    }

    case 3:
    {
      switch (ref_exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = plImageFormat::R32G32B32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = plImageFormat::R16G16B16A16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = plImageFormat::R32G32B32_UINT;
          break;
      }

      break;
    }

    case 4:
    {
      switch (ref_exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = plImageFormat::R32G32B32A32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = plImageFormat::R16G16B16A16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = plImageFormat::R32G32B32A32_UINT;
          break;
      }

      break;
    }
  }

  if (imageFormat == plImageFormat::UNKNOWN)
  {
    plLog::Error("Unsupported EXR file: {}-channel files with format '{}' are unsupported.", ref_exrHeader.num_channels, ref_exrHeader.pixel_types[0]);
    return PL_FAILURE;
  }

  ref_header.SetWidth(ref_exrImage.width);
  ref_header.SetHeight(ref_exrImage.height);
  ref_header.SetImageFormat(imageFormat);

  ref_header.SetNumMipLevels(1);
  ref_header.SetNumArrayIndices(1);
  ref_header.SetNumFaces(1);
  ref_header.SetDepth(1);

  return PL_SUCCESS;
}

plResult plExrFileFormat::ReadImageHeader(plStreamReader& ref_stream, plImageHeader& ref_header, plStringView sFileExtension) const
{
  PL_PROFILE_SCOPE("plExrFileFormat::ReadImageHeader");

  EXRHeader exrHeader;
  InitEXRHeader(&exrHeader);
  PL_SCOPE_EXIT(FreeEXRHeader(&exrHeader));

  EXRImage exrImage;
  InitEXRImage(&exrImage);
  PL_SCOPE_EXIT(FreeEXRImage(&exrImage));

  plDynamicArray<plUInt8> fileBuffer;
  return ReadImageData(ref_stream, fileBuffer, ref_header, exrHeader, exrImage);
}

static void CopyChannel(plUInt8* pDst, const plUInt8* pSrc, plUInt32 uiNumElements, plUInt32 uiElementSize, plUInt32 uiDstStride)
{
  if (uiDstStride == uiElementSize)
  {
    // fast path to copy everything in one operation
    // this only happens for single-channel formats
    plMemoryUtils::RawByteCopy(pDst, pSrc, uiNumElements * uiElementSize);
  }
  else
  {
    for (plUInt32 i = 0; i < uiNumElements; ++i)
    {
      plMemoryUtils::RawByteCopy(pDst, pSrc, uiElementSize);

      pSrc = plMemoryUtils::AddByteOffset(pSrc, uiElementSize);
      pDst = plMemoryUtils::AddByteOffset(pDst, uiDstStride);
    }
  }
}

plResult plExrFileFormat::ReadImage(plStreamReader& ref_stream, plImage& ref_image, plStringView sFileExtension) const
{
  PL_PROFILE_SCOPE("plExrFileFormat::ReadImage");

  EXRHeader exrHeader;
  InitEXRHeader(&exrHeader);
  PL_SCOPE_EXIT(FreeEXRHeader(&exrHeader));

  EXRImage exrImage;
  InitEXRImage(&exrImage);
  PL_SCOPE_EXIT(FreeEXRImage(&exrImage));

  plImageHeader header;
  plDynamicArray<plUInt8> fileBuffer;

  PL_SUCCEED_OR_RETURN(ReadImageData(ref_stream, fileBuffer, header, exrHeader, exrImage));

  ref_image.ResetAndAlloc(header);

  const plUInt32 uiPixelCount = header.GetWidth() * header.GetHeight();
  const plUInt32 uiNumDstChannels = plImageFormat::GetNumChannels(header.GetImageFormat());
  const plUInt32 uiNumSrcChannels = exrHeader.num_channels;

  plUInt32 uiSrcStride = 0;
  switch (exrHeader.pixel_types[0])
  {
    case TINYEXR_PIXELTYPE_FLOAT:
      uiSrcStride = sizeof(float);
      break;

    case TINYEXR_PIXELTYPE_HALF:
      uiSrcStride = sizeof(float) / 2;
      break;

    case TINYEXR_PIXELTYPE_UINT:
      uiSrcStride = sizeof(plUInt32);
      break;

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }


  // src and dst element size is always identical, we only copy from float->float, half->half or uint->uint
  // however data is interleaved in dst, but not interleaved in src

  const plUInt32 uiDstStride = uiSrcStride * uiNumDstChannels;
  plUInt8* pDstBytes = ref_image.GetBlobPtr<plUInt8>().GetPtr();

  if (uiNumDstChannels > uiNumSrcChannels)
  {
    // if we have more dst channels, than in the input data, fill everything with white
    plMemoryUtils::PatternFill(pDstBytes, 0xFF, uiDstStride * uiPixelCount);
  }

  plUInt32 c = 0;

  if (uiNumSrcChannels >= 4)
  {
    const plUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 4)
    {
      // copy to alpha
      CopyChannel(pDstBytes + 3 * uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 3)
  {
    const plUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 3)
    {
      // copy to blue
      CopyChannel(pDstBytes + 2 * uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 2)
  {
    const plUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 2)
    {
      // copy to green
      CopyChannel(pDstBytes + uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 1)
  {
    const plUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 1)
    {
      // copy to red
      CopyChannel(pDstBytes, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  return PL_SUCCESS;
}

plResult plExrFileFormat::WriteImage(plStreamWriter& ref_stream, const plImageView& image, plStringView sFileExtension) const
{
  PL_ASSERT_NOT_IMPLEMENTED;
  return PL_FAILURE;
}

bool plExrFileFormat::CanReadFileType(plStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("exr");
}

bool plExrFileFormat::CanWriteFileType(plStringView sExtension) const
{
  return false;
}

#endif




PL_STATICLINK_FILE(Texture, Texture_Image_Formats_ExrFileFormat);


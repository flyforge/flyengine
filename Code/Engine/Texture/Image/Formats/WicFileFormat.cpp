#include <Texture/TexturePCH.h>

#include <Foundation/Basics/Platform/Win/HResultUtils.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/IO/Stream.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>
#include <Texture/Image/Formats/WicFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/IO/StreamUtils.h>
#  include <Foundation/Profiling/Profiling.h>
#  include <Texture/DirectXTex/DirectXTex.h>

using namespace DirectX;

PL_DEFINE_AS_POD_TYPE(DirectX::Image); // Allow for storing this struct in pl containers

// PL_STATICLINK_FORCE
plWicFileFormat g_wicFormat;

namespace
{
  /// \brief Try to init COM, return true if we are the first(!) to successfully do so
  bool InitializeCOM()
  {
    HRESULT result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (result == S_OK)
    {
      // We were the first one - deinit on shutdown
      return true;
    }
    else if (SUCCEEDED(result))
    {
      // We were not the first one, but we still succeeded, so we deinit COM right away.
      // Otherwise we might be the last one to call CoUninitialize(), but that is supposed to be the one who called
      // CoInitialize[Ex]() first.
      CoUninitialize();
    }

    // We won't call CoUninitialize() on shutdown as either we were not the first one to init COM successfully (and uninitialized it right away),
    // or our call to CoInitializeEx() didn't succeed, because it was already called with another concurrency model specifier.
    return false;
  }
} // namespace


plWicFileFormat::plWicFileFormat() = default;

plWicFileFormat::~plWicFileFormat()
{
  if (m_bCoUninitOnShutdown)
  {
    // We were the first one to successfully initialize COM, so we are the one who needs to shut it down.
    CoUninitialize();
  }
}

plResult plWicFileFormat::ReadFileData(plStreamReader& stream, plDynamicArray<plUInt8>& storage) const
{
  if (m_bTryCoInit)
  {
    m_bCoUninitOnShutdown = InitializeCOM();
    m_bTryCoInit = false;
  }

  plStreamUtils::ReadAllAndAppend(stream, storage);

  if (storage.IsEmpty())
  {
    plLog::Error("Failure to retrieve image data.");
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

static void SetHeader(plImageHeader& ref_header, plImageFormat::Enum imageFormat, const TexMetadata& metadata)
{
  ref_header.SetImageFormat(imageFormat);

  ref_header.SetWidth(plUInt32(metadata.width));
  ref_header.SetHeight(plUInt32(metadata.height));
  ref_header.SetDepth(plUInt32(metadata.depth));

  ref_header.SetNumMipLevels(1);
  ref_header.SetNumArrayIndices(plUInt32(metadata.IsCubemap() ? (metadata.arraySize / 6) : metadata.arraySize));
  ref_header.SetNumFaces(metadata.IsCubemap() ? 6 : 1);
}

plResult plWicFileFormat::ReadImageHeader(plStreamReader& inout_stream, plImageHeader& ref_header, plStringView sFileExtension) const
{
  PL_PROFILE_SCOPE("plWicFileFormat::ReadImageHeader");

  plDynamicArray<plUInt8> storage;
  PL_SUCCEED_OR_RETURN(ReadFileData(inout_stream, storage));

  TexMetadata metadata;
  ScratchImage scratchImage;
  WIC_FLAGS wicFlags = WIC_FLAGS_ALL_FRAMES | WIC_FLAGS_IGNORE_SRGB /* just treat PNG, JPG etc as non-sRGB, we determine this through our 'Usage' later */;

  HRESULT loadResult = GetMetadataFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, metadata);
  if (FAILED(loadResult))
  {
    plLog::Error("Failure to load image metadata. HRESULT:{}", plArgErrorCode(loadResult));
    return PL_FAILURE;
  }

  plImageFormat::Enum imageFormat = plImageFormatMappings::FromDxgiFormat(metadata.format);

  if (imageFormat == plImageFormat::UNKNOWN)
  {
    plLog::Warning("Unable to use image format from '{}' file - trying conversion.", sFileExtension);
    wicFlags |= WIC_FLAGS_FORCE_RGB;
    GetMetadataFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, metadata);
    imageFormat = plImageFormatMappings::FromDxgiFormat(metadata.format);
  }

  if (imageFormat == plImageFormat::UNKNOWN)
  {
    plLog::Error("Unable to use image format from '{}' file.", sFileExtension);
    return PL_FAILURE;
  }

  SetHeader(ref_header, imageFormat, metadata);

  return PL_SUCCESS;
}

plResult plWicFileFormat::ReadImage(plStreamReader& inout_stream, plImage& ref_image, plStringView sFileExtension) const
{
  PL_PROFILE_SCOPE("plWicFileFormat::ReadImage");

  plDynamicArray<plUInt8> storage;
  PL_SUCCEED_OR_RETURN(ReadFileData(inout_stream, storage));

  TexMetadata metadata;
  ScratchImage scratchImage;
  WIC_FLAGS wicFlags = WIC_FLAGS_ALL_FRAMES | WIC_FLAGS_IGNORE_SRGB /* just treat PNG, JPG etc as non-sRGB, we determine this through our 'Usage' later */;

  // Read WIC data from local storage
  HRESULT loadResult = LoadFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, nullptr, scratchImage);
  if (FAILED(loadResult))
  {
    plLog::Error("Failure to load image data. HRESULT:{}", plArgErrorCode(loadResult));
    return PL_FAILURE;
  }

  // Determine image format, re-reading image data if necessary
  metadata = scratchImage.GetMetadata();

  plImageFormat::Enum imageFormat = plImageFormatMappings::FromDxgiFormat(metadata.format);

  if (imageFormat == plImageFormat::UNKNOWN)
  {
    plLog::Warning("Unable to use image format from '{}' file - trying conversion.", sFileExtension);
    wicFlags |= WIC_FLAGS_FORCE_RGB;
    LoadFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, nullptr, scratchImage);
    imageFormat = plImageFormatMappings::FromDxgiFormat(metadata.format);
  }

  if (imageFormat == plImageFormat::UNKNOWN)
  {
    plLog::Error("Unable to use image format from '{}' file.", sFileExtension);
    return PL_FAILURE;
  }

  // Prepare destination image header and allocate storage
  plImageHeader imageHeader;
  SetHeader(imageHeader, imageFormat, metadata);

  ref_image.ResetAndAlloc(imageHeader);

  // Read image data into destination image
  plUInt64 destRowPitch = imageHeader.GetRowPitch();
  plUInt32 itemIdx = 0;
  for (plUInt32 arrayIdx = 0; arrayIdx < imageHeader.GetNumArrayIndices(); ++arrayIdx)
  {
    for (plUInt32 faceIdx = 0; faceIdx < imageHeader.GetNumFaces(); ++faceIdx, ++itemIdx)
    {
      for (plUInt32 sliceIdx = 0; sliceIdx < imageHeader.GetDepth(); ++sliceIdx)
      {
        const Image* sourceImage = scratchImage.GetImage(0, itemIdx, sliceIdx);
        plUInt8* destPixels = ref_image.GetPixelPointer<plUInt8>(0, faceIdx, arrayIdx, 0, 0, sliceIdx);

        if (sourceImage && destPixels && sourceImage->pixels)
        {
          if (destRowPitch == sourceImage->rowPitch)
          {
            // Fast path: Just copy the entire thing
            plMemoryUtils::Copy(destPixels, sourceImage->pixels, static_cast<size_t>(imageHeader.GetHeight() * destRowPitch));
          }
          else
          {
            // Row pitches don't match - copy row by row
            plUInt64 bytesPerRow = plMath::Min(destRowPitch, plUInt64(sourceImage->rowPitch));
            const uint8_t* sourcePixels = sourceImage->pixels;
            for (plUInt32 rowIdx = 0; rowIdx < imageHeader.GetHeight(); ++rowIdx)
            {
              plMemoryUtils::Copy(destPixels, sourcePixels, static_cast<size_t>(bytesPerRow));

              destPixels += destRowPitch;
              sourcePixels += sourceImage->rowPitch;
            }
          }
        }
      }
    }
  }

  return PL_SUCCESS;
}

plResult plWicFileFormat::WriteImage(plStreamWriter& inout_stream, const plImageView& image, plStringView sFileExtension) const
{
  if (m_bTryCoInit)
  {
    m_bCoUninitOnShutdown = InitializeCOM();
    m_bTryCoInit = false;
  }

  using namespace DirectX;

  // Convert into suitable output format
  plImageFormat::Enum compatibleFormats[] = {
    plImageFormat::R8G8B8A8_UNORM,
    plImageFormat::R8G8B8A8_UNORM_SRGB,
    plImageFormat::R8_UNORM,
    plImageFormat::R16G16B16A16_UNORM,
    plImageFormat::R16_UNORM,
    plImageFormat::R32G32B32A32_FLOAT,
    plImageFormat::R32G32B32_FLOAT,
  };

  // Find a compatible format closest to the one the image currently has
  plImageFormat::Enum format = plImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == plImageFormat::UNKNOWN)
  {
    plLog::Error("No conversion from format '{0}' to a format suitable for '{}' files known.", plImageFormat::GetName(image.GetImageFormat()), sFileExtension);
    return PL_FAILURE;
  }

  // Convert if not already in a compatible format
  if (format != image.GetImageFormat())
  {
    plImage convertedImage;
    if (plImageConversion::Convert(image, convertedImage, format) != PL_SUCCESS)
    {
      // This should never happen
      PL_ASSERT_DEV(false, "plImageConversion::Convert failed even though the conversion was to the format returned by FindClosestCompatibleFormat.");
      return PL_FAILURE;
    }

    return WriteImage(inout_stream, convertedImage, sFileExtension);
  }

  // Store plImage data in DirectXTex images
  plDynamicArray<Image> outputImages;
  DXGI_FORMAT imageFormat = DXGI_FORMAT(plImageFormatMappings::ToDxgiFormat(image.GetImageFormat()));
  for (plUInt32 arrayIdx = 0; arrayIdx < image.GetNumArrayIndices(); ++arrayIdx)
  {
    for (plUInt32 faceIdx = 0; faceIdx < image.GetNumFaces(); ++faceIdx)
    {
      for (plUInt32 sliceIdx = 0; sliceIdx < image.GetDepth(); ++sliceIdx)
      {
        Image& currentImage = outputImages.ExpandAndGetRef();
        currentImage.width = image.GetWidth();
        currentImage.height = image.GetHeight();
        currentImage.format = imageFormat;
        currentImage.rowPitch = static_cast<size_t>(image.GetRowPitch());
        currentImage.slicePitch = static_cast<size_t>(image.GetDepthPitch());
        currentImage.pixels = const_cast<uint8_t*>(image.GetPixelPointer<uint8_t>(0, faceIdx, arrayIdx, 0, 0, sliceIdx));
      }
    }
  }

  if (!outputImages.IsEmpty())
  {
    // Store images in output blob
    Blob targetBlob;
    WIC_FLAGS flags = WIC_FLAGS_NONE;
    HRESULT res = SaveToWICMemory(outputImages.GetData(), outputImages.GetCount(), flags, GetWICCodec(WIC_CODEC_TIFF), targetBlob);
    if (FAILED(res))
    {
      plLog::Error("Failed to save image data to local memory blob - result: {}!", plHRESULTtoString(res));
      return PL_FAILURE;
    }

    // Push blob into output stream
    if (inout_stream.WriteBytes(targetBlob.GetBufferPointer(), targetBlob.GetBufferSize()) != PL_SUCCESS)
    {
      plLog::Error("Failed to write image data!");
      return PL_FAILURE;
    }
  }

  return PL_SUCCESS;
}

bool plWicFileFormat::CanReadFileType(plStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("png") || sExtension.IsEqual_NoCase("jpg") || sExtension.IsEqual_NoCase("jpeg") ||
         sExtension.IsEqual_NoCase("tif") || sExtension.IsEqual_NoCase("tiff");
}

bool plWicFileFormat::CanWriteFileType(plStringView sExtension) const
{
  // png, jpg and jpeg are handled by STB (plStbImageFileFormats)
  return sExtension.IsEqual_NoCase("tif") || sExtension.IsEqual_NoCase("tiff");
}

#endif




PL_STATICLINK_FILE(Texture, Texture_Image_Formats_WicFileFormat);


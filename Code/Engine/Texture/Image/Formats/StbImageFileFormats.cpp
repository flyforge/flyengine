#include <Texture/TexturePCH.h>

#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/Image/Image.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/StreamUtils.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>
#include <stb_image/stb_image.h>
#include <stb_image/stb_image_write.h>

// PL_STATICLINK_FORCE
plStbImageFileFormats g_StbImageFormats;

// stb_image callbacks would be better than loading the entire file into memory.
// However, it turned out that it does not map well to plStreamReader

// namespace
//{
//  // fill 'data' with 'size' bytes.  return number of bytes actually read
//  int read(void *user, char *data, int size)
//  {
//    plStreamReader* pStream = static_cast<plStreamReader*>(user);
//    return static_cast<int>(pStream->ReadBytes(data, size));
//  }
//  // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
//  void skip(void *user, int n)
//  {
//    plStreamReader* pStream = static_cast<plStreamReader*>(user);
//    if(n > 0)
//      pStream->SkipBytes(n);
//    else
//      // ?? We cannot reverse skip.
//
//  }
//  // returns nonzero if we are at end of file/data
//  int eof(void *user)
//  {
//    plStreamReader* pStream = static_cast<plStreamReader*>(user);
//    // ?
//  }
//}

namespace
{

  void write_func(void* pContext, void* pData, int iSize)
  {
    plStreamWriter* writer = static_cast<plStreamWriter*>(pContext);
    writer->WriteBytes(pData, iSize).IgnoreResult();
  }

  void* ReadImageData(plStreamReader& inout_stream, plDynamicArray<plUInt8>& ref_fileBuffer, plImageHeader& ref_imageHeader, bool& ref_bIsHDR)
  {
    plStreamUtils::ReadAllAndAppend(inout_stream, ref_fileBuffer);

    int width, height, numComp;

    ref_bIsHDR = !!stbi_is_hdr_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount());

    void* sourceImageData = nullptr;
    if (ref_bIsHDR)
    {
      sourceImageData = stbi_loadf_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &width, &height, &numComp, 0);
    }
    else
    {
      sourceImageData = stbi_load_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &width, &height, &numComp, 0);
    }
    if (!sourceImageData)
    {
      plLog::Error("stb_image failed to load: {0}", stbi_failure_reason());
      return nullptr;
    }
    ref_fileBuffer.Clear();

    plImageFormat::Enum format = plImageFormat::UNKNOWN;
    switch (numComp)
    {
      case 1:
        format = (ref_bIsHDR) ? plImageFormat::R32_FLOAT : plImageFormat::R8_UNORM;
        break;
      case 2:
        format = (ref_bIsHDR) ? plImageFormat::R32G32_FLOAT : plImageFormat::R8G8_UNORM;
        break;
      case 3:
        format = (ref_bIsHDR) ? plImageFormat::R32G32B32_FLOAT : plImageFormat::R8G8B8_UNORM;
        break;
      case 4:
        format = (ref_bIsHDR) ? plImageFormat::R32G32B32A32_FLOAT : plImageFormat::R8G8B8A8_UNORM;
        break;
    }

    // Set properties and allocate.
    ref_imageHeader.SetImageFormat(format);
    ref_imageHeader.SetNumMipLevels(1);
    ref_imageHeader.SetNumArrayIndices(1);
    ref_imageHeader.SetNumFaces(1);

    ref_imageHeader.SetWidth(width);
    ref_imageHeader.SetHeight(height);
    ref_imageHeader.SetDepth(1);

    return sourceImageData;
  }

} // namespace

plResult plStbImageFileFormats::ReadImageHeader(plStreamReader& inout_stream, plImageHeader& ref_header, plStringView sFileExtension) const
{
  PL_PROFILE_SCOPE("plStbImageFileFormats::ReadImageHeader");

  bool isHDR = false;
  plDynamicArray<plUInt8> fileBuffer;
  void* sourceImageData = ReadImageData(inout_stream, fileBuffer, ref_header, isHDR);

  if (sourceImageData == nullptr)
    return PL_FAILURE;

  stbi_image_free(sourceImageData);
  return PL_SUCCESS;
}

plResult plStbImageFileFormats::ReadImage(plStreamReader& inout_stream, plImage& ref_image, plStringView sFileExtension) const
{
  PL_PROFILE_SCOPE("plStbImageFileFormats::ReadImage");

  bool isHDR = false;
  plDynamicArray<plUInt8> fileBuffer;
  plImageHeader imageHeader;
  void* sourceImageData = ReadImageData(inout_stream, fileBuffer, imageHeader, isHDR);

  if (sourceImageData == nullptr)
    return PL_FAILURE;

  ref_image.ResetAndAlloc(imageHeader);

  const size_t numComp = plImageFormat::GetNumChannels(imageHeader.GetImageFormat());

  const size_t elementsToCopy = static_cast<size_t>(imageHeader.GetWidth()) * static_cast<size_t>(imageHeader.GetHeight()) * numComp;

  // Set pixels. Different strategies depending on component count.
  if (isHDR)
  {
    float* targetImageData = ref_image.GetBlobPtr<float>().GetPtr();
    plMemoryUtils::Copy(targetImageData, (const float*)sourceImageData, elementsToCopy);
  }
  else
  {
    plUInt8* targetImageData = ref_image.GetBlobPtr<plUInt8>().GetPtr();
    plMemoryUtils::Copy(targetImageData, (const plUInt8*)sourceImageData, elementsToCopy);
  }

  stbi_image_free((void*)sourceImageData);
  return PL_SUCCESS;
}

plResult plStbImageFileFormats::WriteImage(plStreamWriter& inout_stream, const plImageView& image, plStringView sFileExtension) const
{
  plImageFormat::Enum compatibleFormats[] = {plImageFormat::R8_UNORM, plImageFormat::R8G8B8_UNORM, plImageFormat::R8G8B8A8_UNORM};

  // Find a compatible format closest to the one the image currently has
  plImageFormat::Enum format = plImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == plImageFormat::UNKNOWN)
  {
    plLog::Error("No conversion from format '{0}' to a format suitable for PNG files known.", plImageFormat::GetName(image.GetImageFormat()));
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

  if (sFileExtension.IsEqual_NoCase("png"))
  {
    if (stbi_write_png_to_func(write_func, &inout_stream, image.GetWidth(), image.GetHeight(), plImageFormat::GetNumChannels(image.GetImageFormat()), image.GetByteBlobPtr().GetPtr(), 0))
    {
      return PL_SUCCESS;
    }
  }

  if (sFileExtension.IsEqual_NoCase("jpg") || sFileExtension.IsEqual_NoCase("jpeg"))
  {
    if (stbi_write_jpg_to_func(write_func, &inout_stream, image.GetWidth(), image.GetHeight(), plImageFormat::GetNumChannels(image.GetImageFormat()), image.GetByteBlobPtr().GetPtr(), 95))
    {
      return PL_SUCCESS;
    }
  }

  return PL_FAILURE;
}

bool plStbImageFileFormats::CanReadFileType(plStringView sExtension) const
{
  if (sExtension.IsEqual_NoCase("hdr"))
    return true;

#if PL_DISABLED(PL_PLATFORM_WINDOWS_DESKTOP)

  // on Windows Desktop, we prefer to use WIC (plWicFileFormat)
  if (sExtension.IsEqual_NoCase("png") || sExtension.IsEqual_NoCase("jpg") || sExtension.IsEqual_NoCase("jpeg"))
  {
    return true;
  }
#endif

  return false;
}

bool plStbImageFileFormats::CanWriteFileType(plStringView sExtension) const
{
  // even when WIC is available, prefer to write these files through STB, to get consistent output
  if (sExtension.IsEqual_NoCase("png") || sExtension.IsEqual_NoCase("jpg") || sExtension.IsEqual_NoCase("jpeg"))
  {
    return true;
  }

  return false;
}




PL_STATICLINK_FILE(Texture, Texture_Image_Formats_StbImageFileFormats);


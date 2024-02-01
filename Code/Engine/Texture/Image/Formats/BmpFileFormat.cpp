#include <Texture/TexturePCH.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/BmpFileFormat.h>
#include <Texture/Image/ImageConversion.h>

// PL_STATICLINK_FORCE
plBmpFileFormat g_bmpFormat;

enum plBmpCompression
{
  RGB = 0L,
  RLE8 = 1L,
  RLE4 = 2L,
  BITFIELDS = 3L,
  JPEG = 4L,
  PNG = 5L,
};


#pragma pack(push, 1)
struct plBmpFileHeader
{
  plUInt16 m_type = 0;
  plUInt32 m_size = 0;
  plUInt16 m_reserved1 = 0;
  plUInt16 m_reserved2 = 0;
  plUInt32 m_offBits = 0;
};
#pragma pack(pop)

struct plBmpFileInfoHeader
{
  plUInt32 m_size = 0;
  plUInt32 m_width = 0;
  plUInt32 m_height = 0;
  plUInt16 m_planes = 0;
  plUInt16 m_bitCount = 0;
  plBmpCompression m_compression = plBmpCompression::RGB;
  plUInt32 m_sizeImage = 0;
  plUInt32 m_xPelsPerMeter = 0;
  plUInt32 m_yPelsPerMeter = 0;
  plUInt32 m_clrUsed = 0;
  plUInt32 m_clrImportant = 0;
};

struct plCIEXYZ
{
  int ciexyzX = 0;
  int ciexyzY = 0;
  int ciexyzZ = 0;
};

struct plCIEXYZTRIPLE
{
  plCIEXYZ ciexyzRed;
  plCIEXYZ ciexyzGreen;
  plCIEXYZ ciexyzBlue;
};

struct plBmpFileInfoHeaderV4
{
  plUInt32 m_redMask = 0;
  plUInt32 m_greenMask = 0;
  plUInt32 m_blueMask = 0;
  plUInt32 m_alphaMask = 0;
  plUInt32 m_csType = 0;
  plCIEXYZTRIPLE m_endpoints;
  plUInt32 m_gammaRed = 0;
  plUInt32 m_gammaGreen = 0;
  plUInt32 m_gammaBlue = 0;
};

PL_CHECK_AT_COMPILETIME(sizeof(plCIEXYZTRIPLE) == 3 * 3 * 4);

// just to be on the safe side
#if PL_ENABLED(PL_PLATFORM_WINDOWS)
PL_CHECK_AT_COMPILETIME(sizeof(plCIEXYZTRIPLE) == sizeof(CIEXYZTRIPLE));
#endif

struct plBmpFileInfoHeaderV5
{
  plUInt32 m_intent;
  plUInt32 m_profileData;
  plUInt32 m_profileSize;
  plUInt32 m_reserved;
};

static const plUInt16 plBmpFileMagic = 0x4D42u;

struct plBmpBgrxQuad
{
  PL_DECLARE_POD_TYPE();

  plBmpBgrxQuad() = default;

  plBmpBgrxQuad(plUInt8 uiRed, plUInt8 uiGreen, plUInt8 uiBlue)
    : m_blue(uiBlue)
    , m_green(uiGreen)
    , m_red(uiRed)
    , m_reserved(0)
  {
  }

  plUInt8 m_blue;
  plUInt8 m_green;
  plUInt8 m_red;
  plUInt8 m_reserved;
};

plResult plBmpFileFormat::WriteImage(plStreamWriter& inout_stream, const plImageView& image, plStringView sFileExtension) const
{
  // Technically almost arbitrary formats are supported, but we only use the common ones.
  plImageFormat::Enum compatibleFormats[] = {
    plImageFormat::B8G8R8X8_UNORM,
    plImageFormat::B8G8R8A8_UNORM,
    plImageFormat::B8G8R8_UNORM,
    plImageFormat::B5G5R5X1_UNORM,
    plImageFormat::B5G6R5_UNORM,
  };

  // Find a compatible format closest to the one the image currently has
  plImageFormat::Enum format = plImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == plImageFormat::UNKNOWN)
  {
    plLog::Error("No conversion from format '{0}' to a format suitable for BMP files known.", plImageFormat::GetName(image.GetImageFormat()));
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

  plUInt64 uiRowPitch = image.GetRowPitch(0);

  plUInt32 uiHeight = image.GetHeight(0);

  plUInt64 dataSize = uiRowPitch * uiHeight;
  if (dataSize >= plMath::MaxValue<plUInt32>())
  {
    PL_ASSERT_DEV(false, "Size overflow in BMP file format.");
    return PL_FAILURE;
  }

  plBmpFileInfoHeader fileInfoHeader;
  fileInfoHeader.m_width = image.GetWidth(0);
  fileInfoHeader.m_height = uiHeight;
  fileInfoHeader.m_planes = 1;
  fileInfoHeader.m_bitCount = static_cast<plUInt16>(plImageFormat::GetBitsPerPixel(format));

  fileInfoHeader.m_sizeImage = 0; // Can be zero unless we store the data compressed

  fileInfoHeader.m_xPelsPerMeter = 0;
  fileInfoHeader.m_yPelsPerMeter = 0;
  fileInfoHeader.m_clrUsed = 0;
  fileInfoHeader.m_clrImportant = 0;

  bool bWriteColorMask = false;

  // Prefer to write a V3 header
  plUInt32 uiHeaderVersion = 3;

  switch (format)
  {
    case plImageFormat::B8G8R8X8_UNORM:
    case plImageFormat::B5G5R5X1_UNORM:
    case plImageFormat::B8G8R8_UNORM:
      fileInfoHeader.m_compression = RGB;
      break;

    case plImageFormat::B8G8R8A8_UNORM:
      fileInfoHeader.m_compression = BITFIELDS;
      uiHeaderVersion = 4;
      break;

    case plImageFormat::B5G6R5_UNORM:
      fileInfoHeader.m_compression = BITFIELDS;
      bWriteColorMask = true;
      break;

    default:
      return PL_FAILURE;
  }

  PL_ASSERT_DEV(!bWriteColorMask || uiHeaderVersion <= 3, "Internal bug");

  plUInt32 uiFileInfoHeaderSize = sizeof(plBmpFileInfoHeader);
  plUInt32 uiHeaderSize = sizeof(plBmpFileHeader);

  if (uiHeaderVersion >= 4)
  {
    uiFileInfoHeaderSize += sizeof(plBmpFileInfoHeaderV4);
  }
  else if (bWriteColorMask)
  {
    uiHeaderSize += 3 * sizeof(plUInt32);
  }

  uiHeaderSize += uiFileInfoHeaderSize;

  fileInfoHeader.m_size = uiFileInfoHeaderSize;

  plBmpFileHeader header;
  header.m_type = plBmpFileMagic;
  header.m_size = uiHeaderSize + static_cast<plUInt32>(dataSize);
  header.m_reserved1 = 0;
  header.m_reserved2 = 0;
  header.m_offBits = uiHeaderSize;

  // Write all data
  if (inout_stream.WriteBytes(&header, sizeof(header)) != PL_SUCCESS)
  {
    plLog::Error("Failed to write header.");
    return PL_FAILURE;
  }

  if (inout_stream.WriteBytes(&fileInfoHeader, sizeof(fileInfoHeader)) != PL_SUCCESS)
  {
    plLog::Error("Failed to write fileInfoHeader.");
    return PL_FAILURE;
  }

  if (uiHeaderVersion >= 4)
  {
    plBmpFileInfoHeaderV4 fileInfoHeaderV4;
    memset(&fileInfoHeaderV4, 0, sizeof(fileInfoHeaderV4));

    fileInfoHeaderV4.m_redMask = plImageFormat::GetRedMask(format);
    fileInfoHeaderV4.m_greenMask = plImageFormat::GetGreenMask(format);
    fileInfoHeaderV4.m_blueMask = plImageFormat::GetBlueMask(format);
    fileInfoHeaderV4.m_alphaMask = plImageFormat::GetAlphaMask(format);

    if (inout_stream.WriteBytes(&fileInfoHeaderV4, sizeof(fileInfoHeaderV4)) != PL_SUCCESS)
    {
      plLog::Error("Failed to write fileInfoHeaderV4.");
      return PL_FAILURE;
    }
  }
  else if (bWriteColorMask)
  {
    struct
    {
      plUInt32 m_red;
      plUInt32 m_green;
      plUInt32 m_blue;
    } colorMask;


    colorMask.m_red = plImageFormat::GetRedMask(format);
    colorMask.m_green = plImageFormat::GetGreenMask(format);
    colorMask.m_blue = plImageFormat::GetBlueMask(format);

    if (inout_stream.WriteBytes(&colorMask, sizeof(colorMask)) != PL_SUCCESS)
    {
      plLog::Error("Failed to write colorMask.");
      return PL_FAILURE;
    }
  }

  const plUInt64 uiPaddedRowPitch = ((uiRowPitch - 1) / 4 + 1) * 4;
  // Write rows in reverse order
  for (plInt32 iRow = uiHeight - 1; iRow >= 0; iRow--)
  {
    if (inout_stream.WriteBytes(image.GetPixelPointer<void>(0, 0, 0, 0, iRow, 0), uiRowPitch) != PL_SUCCESS)
    {
      plLog::Error("Failed to write data.");
      return PL_FAILURE;
    }

    plUInt8 zeroes[4] = {0, 0, 0, 0};
    if (inout_stream.WriteBytes(zeroes, uiPaddedRowPitch - uiRowPitch) != PL_SUCCESS)
    {
      plLog::Error("Failed to write data.");
      return PL_FAILURE;
    }
  }

  return PL_SUCCESS;
}

namespace
{
  plUInt32 ExtractBits(const void* pData, plUInt32 uiBitAddress, plUInt32 uiNumBits)
  {
    plUInt32 uiMask = (1U << uiNumBits) - 1;
    plUInt32 uiByteAddress = uiBitAddress / 8;
    plUInt32 uiShiftAmount = 7 - (uiBitAddress % 8 + uiNumBits - 1);

    return (reinterpret_cast<const plUInt8*>(pData)[uiByteAddress] >> uiShiftAmount) & uiMask;
  }

  plResult ReadImageInfo(plStreamReader& inout_stream, plImageHeader& ref_header, plBmpFileHeader& ref_fileHeader, plBmpFileInfoHeader& ref_fileInfoHeader, bool& ref_bIndexed,
    bool& ref_bCompressed, plUInt32& ref_uiBpp, plUInt32& ref_uiDataSize)
  {
    if (inout_stream.ReadBytes(&ref_fileHeader, sizeof(plBmpFileHeader)) != sizeof(plBmpFileHeader))
    {
      plLog::Error("Failed to read header data.");
      return PL_FAILURE;
    }

    // Some very old BMP variants may have different magic numbers, but we don't support them.
    if (ref_fileHeader.m_type != plBmpFileMagic)
    {
      plLog::Error("The file is not a recognized BMP file.");
      return PL_FAILURE;
    }

    // We expect at least header version 3
    plUInt32 uiHeaderVersion = 3;
    if (inout_stream.ReadBytes(&ref_fileInfoHeader, sizeof(plBmpFileInfoHeader)) != sizeof(plBmpFileInfoHeader))
    {
      plLog::Error("Failed to read header data (V3).");
      return PL_FAILURE;
    }

    int remainingHeaderBytes = ref_fileInfoHeader.m_size - sizeof(ref_fileInfoHeader);

    // File header shorter than expected - happens with corrupt files or e.g. with OS/2 BMP files which may have shorter headers
    if (remainingHeaderBytes < 0)
    {
      plLog::Error("The file header was shorter than expected.");
      return PL_FAILURE;
    }

    // Newer files may have a header version 4 (required for transparency)
    plBmpFileInfoHeaderV4 fileInfoHeaderV4;
    if (remainingHeaderBytes >= sizeof(plBmpFileInfoHeaderV4))
    {
      uiHeaderVersion = 4;
      if (inout_stream.ReadBytes(&fileInfoHeaderV4, sizeof(plBmpFileInfoHeaderV4)) != sizeof(plBmpFileInfoHeaderV4))
      {
        plLog::Error("Failed to read header data (V4).");
        return PL_FAILURE;
      }
      remainingHeaderBytes -= sizeof(plBmpFileInfoHeaderV4);
    }

    // Skip rest of header
    if (inout_stream.SkipBytes(remainingHeaderBytes) != remainingHeaderBytes)
    {
      plLog::Error("Failed to skip remaining header data.");
      return PL_FAILURE;
    }

    ref_uiBpp = ref_fileInfoHeader.m_bitCount;

    // Find target format to load the image
    plImageFormat::Enum format = plImageFormat::UNKNOWN;

    switch (ref_fileInfoHeader.m_compression)
    {
        // RGB or indexed data
      case RGB:
        switch (ref_uiBpp)
        {
          case 1:
          case 4:
          case 8:
            ref_bIndexed = true;

            // We always decompress indexed to BGRX, since the palette is specified in this format
            format = plImageFormat::B8G8R8X8_UNORM;
            break;

          case 16:
            format = plImageFormat::B5G5R5X1_UNORM;
            break;

          case 24:
            format = plImageFormat::B8G8R8_UNORM;
            break;

          case 32:
            format = plImageFormat::B8G8R8X8_UNORM;
        }
        break;

        // RGB data, but with the color masks specified in place of the palette
      case BITFIELDS:
        switch (ref_uiBpp)
        {
          case 16:
          case 32:
            // In case of old headers, the color masks appear after the header (and aren't counted as part of it)
            if (uiHeaderVersion < 4)
            {
              // Color masks (w/o alpha channel)
              struct
              {
                plUInt32 m_red;
                plUInt32 m_green;
                plUInt32 m_blue;
              } colorMask;

              if (inout_stream.ReadBytes(&colorMask, sizeof(colorMask)) != sizeof(colorMask))
              {
                return PL_FAILURE;
              }

              format = plImageFormat::FromPixelMask(colorMask.m_red, colorMask.m_green, colorMask.m_blue, 0, ref_uiBpp);
            }
            else
            {
              // For header version four and higher, the color masks are part of the header
              format = plImageFormat::FromPixelMask(
                fileInfoHeaderV4.m_redMask, fileInfoHeaderV4.m_greenMask, fileInfoHeaderV4.m_blueMask, fileInfoHeaderV4.m_alphaMask, ref_uiBpp);
            }

            break;
        }
        break;

      case RLE4:
        if (ref_uiBpp == 4)
        {
          ref_bIndexed = true;
          ref_bCompressed = true;
          format = plImageFormat::B8G8R8X8_UNORM;
        }
        break;

      case RLE8:
        if (ref_uiBpp == 8)
        {
          ref_bIndexed = true;
          ref_bCompressed = true;
          format = plImageFormat::B8G8R8X8_UNORM;
        }
        break;

      default:
        PL_ASSERT_NOT_IMPLEMENTED;
    }

    if (format == plImageFormat::UNKNOWN)
    {
      plLog::Error("Unknown or unsupported BMP encoding.");
      return PL_FAILURE;
    }

    const plUInt32 uiWidth = ref_fileInfoHeader.m_width;

    if (uiWidth > 65536)
    {
      plLog::Error("Image specifies width > 65536. Header corrupted?");
      return PL_FAILURE;
    }

    const plUInt32 uiHeight = ref_fileInfoHeader.m_height;

    if (uiHeight > 65536)
    {
      plLog::Error("Image specifies height > 65536. Header corrupted?");
      return PL_FAILURE;
    }

    ref_uiDataSize = ref_fileInfoHeader.m_sizeImage;

    if (ref_uiDataSize > 1024 * 1024 * 1024)
    {
      plLog::Error("Image specifies data size > 1GiB. Header corrupted?");
      return PL_FAILURE;
    }

    const int uiRowPitchIn = (uiWidth * ref_uiBpp + 31) / 32 * 4;

    if (ref_uiDataSize == 0)
    {
      if (ref_fileInfoHeader.m_compression != RGB)
      {
        plLog::Error("The data size wasn't specified in the header.");
        return PL_FAILURE;
      }
      ref_uiDataSize = uiRowPitchIn * uiHeight;
    }

    // Set image data
    ref_header.SetImageFormat(format);
    ref_header.SetNumMipLevels(1);
    ref_header.SetNumArrayIndices(1);
    ref_header.SetNumFaces(1);

    ref_header.SetWidth(uiWidth);
    ref_header.SetHeight(uiHeight);
    ref_header.SetDepth(1);

    return PL_SUCCESS;
  }

} // namespace

plResult plBmpFileFormat::ReadImageHeader(plStreamReader& inout_stream, plImageHeader& ref_header, plStringView sFileExtension) const
{
  PL_PROFILE_SCOPE("plBmpFileFormat::ReadImage");

  plBmpFileHeader fileHeader;
  plBmpFileInfoHeader fileInfoHeader;
  bool bIndexed = false, bCompressed = false;
  plUInt32 uiBpp = 0;
  plUInt32 uiDataSize = 0;

  return ReadImageInfo(inout_stream, ref_header, fileHeader, fileInfoHeader, bIndexed, bCompressed, uiBpp, uiDataSize);
}

plResult plBmpFileFormat::ReadImage(plStreamReader& inout_stream, plImage& ref_image, plStringView sFileExtension) const
{
  PL_PROFILE_SCOPE("plBmpFileFormat::ReadImage");

  plBmpFileHeader fileHeader;
  plImageHeader header;
  plBmpFileInfoHeader fileInfoHeader;
  bool bIndexed = false, bCompressed = false;
  plUInt32 uiBpp = 0;
  plUInt32 uiDataSize = 0;

  PL_SUCCEED_OR_RETURN(ReadImageInfo(inout_stream, header, fileHeader, fileInfoHeader, bIndexed, bCompressed, uiBpp, uiDataSize));

  ref_image.ResetAndAlloc(header);

  plUInt64 uiRowPitch = ref_image.GetRowPitch(0);

  const int uiRowPitchIn = (header.GetWidth() * uiBpp + 31) / 32 * 4;

  if (bIndexed)
  {
    // If no palette size was specified, the full available palette size will be used
    plUInt32 paletteSize = fileInfoHeader.m_clrUsed;
    if (paletteSize == 0)
    {
      paletteSize = 1U << uiBpp;
    }
    else if (paletteSize > 65536)
    {
      plLog::Error("Palette size > 65536.");
      return PL_FAILURE;
    }

    plDynamicArray<plBmpBgrxQuad> palette;
    palette.SetCountUninitialized(paletteSize);
    if (inout_stream.ReadBytes(&palette[0], paletteSize * sizeof(plBmpBgrxQuad)) != paletteSize * sizeof(plBmpBgrxQuad))
    {
      plLog::Error("Failed to read palette data.");
      return PL_FAILURE;
    }

    if (bCompressed)
    {
      // Compressed data is always in pairs of bytes
      if (uiDataSize % 2 != 0)
      {
        plLog::Error("The data size is not a multiple of 2 bytes in an RLE-compressed file.");
        return PL_FAILURE;
      }

      plDynamicArray<plUInt8> compressedData;
      compressedData.SetCountUninitialized(uiDataSize);

      if (inout_stream.ReadBytes(&compressedData[0], uiDataSize) != uiDataSize)
      {
        plLog::Error("Failed to read data.");
        return PL_FAILURE;
      }

      const plUInt8* pIn = &compressedData[0];
      const plUInt8* pInEnd = pIn + uiDataSize;

      // Current output position
      plUInt32 uiRow = fileInfoHeader.m_height - 1;
      plUInt32 uiCol = 0;

      plBmpBgrxQuad* pLine = ref_image.GetPixelPointer<plBmpBgrxQuad>(0, 0, 0, 0, uiRow, 0);

      // Decode RLE data directly to RGBX
      while (pIn < pInEnd)
      {
        plUInt32 uiByte1 = *pIn++;
        plUInt32 uiByte2 = *pIn++;

        // Relative mode - the first byte specified a number of indices to be repeated, the second one the indices
        if (uiByte1 > 0)
        {
          // Clamp number of repetitions to row width.
          // The spec isn't clear on this point, but some files pad the number of encoded indices for some reason.
          uiByte1 = plMath::Min(uiByte1, fileInfoHeader.m_width - uiCol);

          if (uiBpp == 4)
          {
            // Alternate between two indices.
            for (plUInt32 uiRep = 0; uiRep < uiByte1 / 2; uiRep++)
            {
              pLine[uiCol++] = palette[uiByte2 >> 4];
              pLine[uiCol++] = palette[uiByte2 & 0x0F];
            }

            // Repeat the first index for odd numbers of repetitions.
            if (uiByte1 & 1)
            {
              pLine[uiCol++] = palette[uiByte2 >> 4];
            }
          }
          else /* if (uiBpp == 8) */
          {
            // Repeat a single index.
            for (plUInt32 uiRep = 0; uiRep < uiByte1; uiRep++)
            {
              pLine[uiCol++] = palette[uiByte2];
            }
          }
        }
        else
        {
          // Absolute mode - the first byte specifies a number of indices encoded separately, or is a special marker
          switch (uiByte2)
          {
              // End of line marker
            case 0:
            {

              // Fill up with palette entry 0
              while (uiCol < fileInfoHeader.m_width)
              {
                pLine[uiCol++] = palette[0];
              }

              // Begin next line
              uiCol = 0;
              uiRow--;
              pLine -= fileInfoHeader.m_width;
            }

            break;

              // End of image marker
            case 1:
              // Check that we really reached the end of the image.
              if (uiRow != 0 && uiCol != fileInfoHeader.m_height - 1)
              {
                plLog::Error("Unexpected end of image marker found.");
                return PL_FAILURE;
              }
              break;

            case 2:
              plLog::Error("Found a RLE compression position delta - this is not supported.");
              return PL_FAILURE;

            default:
              // Read uiByte2 number of indices

              // More data than fits into the image or can be read?
              if (uiCol + uiByte2 > fileInfoHeader.m_width || pIn + (uiByte2 + 1) / 2 > pInEnd)
              {
                return PL_FAILURE;
              }

              if (uiBpp == 4)
              {
                for (plUInt32 uiRep = 0; uiRep < uiByte2 / 2; uiRep++)
                {
                  plUInt32 uiIndices = *pIn++;
                  pLine[uiCol++] = palette[uiIndices >> 4];
                  pLine[uiCol++] = palette[uiIndices & 0x0f];
                }

                if (uiByte2 & 1)
                {
                  pLine[uiCol++] = palette[*pIn++ >> 4];
                }

                // Pad to word boundary
                pIn += (uiByte2 / 2 + uiByte2) & 1;
              }
              else /* if (uiBpp == 8) */
              {
                for (plUInt32 uiRep = 0; uiRep < uiByte2; uiRep++)
                {
                  pLine[uiCol++] = palette[*pIn++];
                }

                // Pad to word boundary
                pIn += uiByte2 & 1;
              }
          }
        }
      }
    }
    else
    {
      plDynamicArray<plUInt8> indexedData;
      indexedData.SetCountUninitialized(uiDataSize);
      if (inout_stream.ReadBytes(&indexedData[0], uiDataSize) != uiDataSize)
      {
        plLog::Error("Failed to read data.");
        return PL_FAILURE;
      }

      // Convert to non-indexed
      for (plUInt32 uiRow = 0; uiRow < fileInfoHeader.m_height; uiRow++)
      {
        plUInt8* pIn = &indexedData[uiRowPitchIn * uiRow];

        // Convert flipped vertically
        plBmpBgrxQuad* pOut = ref_image.GetPixelPointer<plBmpBgrxQuad>(0, 0, 0, 0, fileInfoHeader.m_height - uiRow - 1, 0);
        for (plUInt32 uiCol = 0; uiCol < ref_image.GetWidth(0); uiCol++)
        {
          plUInt32 uiIndex = ExtractBits(pIn, uiCol * uiBpp, uiBpp);
          if (uiIndex >= palette.GetCount())
          {
            plLog::Error("Image contains invalid palette indices.");
            return PL_FAILURE;
          }
          pOut[uiCol] = palette[uiIndex];
        }
      }
    }
  }
  else
  {
    // Format must match the number of bits in the file
    if (plImageFormat::GetBitsPerPixel(header.GetImageFormat()) != uiBpp)
    {
      plLog::Error("The number of bits per pixel specified in the file ({0}) does not match the expected value of {1} for the format '{2}'.",
        uiBpp, plImageFormat::GetBitsPerPixel(header.GetImageFormat()), plImageFormat::GetName(header.GetImageFormat()));
      return PL_FAILURE;
    }

    // Skip palette data. Having a palette here doesn't make sense, but is not explicitly disallowed by the standard.
    plUInt32 paletteSize = fileInfoHeader.m_clrUsed * sizeof(plBmpBgrxQuad);
    if (inout_stream.SkipBytes(paletteSize) != paletteSize)
    {
      plLog::Error("Failed to skip palette data.");
      return PL_FAILURE;
    }

    // Read rows in reverse order
    for (plInt32 iRow = fileInfoHeader.m_height - 1; iRow >= 0; iRow--)
    {
      if (inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, 0, iRow, 0), uiRowPitch) != uiRowPitch)
      {
        plLog::Error("Failed to read row data.");
        return PL_FAILURE;
      }
      if (inout_stream.SkipBytes(uiRowPitchIn - uiRowPitch) != uiRowPitchIn - uiRowPitch)
      {
        plLog::Error("Failed to skip row data.");
        return PL_FAILURE;
      }
    }
  }

  return PL_SUCCESS;
}

bool plBmpFileFormat::CanReadFileType(plStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("bmp") || sExtension.IsEqual_NoCase("dib") || sExtension.IsEqual_NoCase("rle");
}

bool plBmpFileFormat::CanWriteFileType(plStringView sExtension) const
{
  return CanReadFileType(sExtension);
}




PL_STATICLINK_FILE(Texture, Texture_Image_Formats_BmpFileFormat);


#include <Texture/TexturePCH.h>

#include <Texture/Image/Formats/TgaFileFormat.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>


plTgaFileFormat g_TgaFormat;

struct TgaImageDescriptor
{
  plUInt8 m_iAlphaBits : 4;
  plUInt8 m_bFlipH : 1;
  plUInt8 m_bFlipV : 1;
  plUInt8 m_Ignored : 2;
};

// see Wikipedia for details:
// http://de.wikipedia.org/wiki/Targa_Image_File
struct TgaHeader
{
  plInt8 m_iImageIDLength;
  plInt8 m_Ignored1;
  plInt8 m_ImageType;
  plInt8 m_Ignored2[9];
  plInt16 m_iImageWidth;
  plInt16 m_iImageHeight;
  plInt8 m_iBitsPerPixel;
  TgaImageDescriptor m_ImageDescriptor;
};

PLASMA_CHECK_AT_COMPILETIME(sizeof(TgaHeader) == 18);


static inline plColorLinearUB GetPixelColor(const plImageView& image, plUInt32 x, plUInt32 y, const plUInt32 uiHeight)
{
  plColorLinearUB c(255, 255, 255, 255);

  const plUInt8* pPixel = image.GetPixelPointer<plUInt8>(0, 0, 0, x, uiHeight - y - 1, 0);

  switch (image.GetImageFormat())
  {
    case plImageFormat::R8G8B8A8_UNORM:
      c.r = pPixel[0];
      c.g = pPixel[1];
      c.b = pPixel[2];
      c.a = pPixel[3];
      break;
    case plImageFormat::B8G8R8A8_UNORM:
      c.a = pPixel[3];
      // fall through
    case plImageFormat::B8G8R8_UNORM:
    case plImageFormat::B8G8R8X8_UNORM:
      c.r = pPixel[2];
      c.g = pPixel[1];
      c.b = pPixel[0];
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  return c;
}


plResult plTgaFileFormat::WriteImage(plStreamWriter& inout_stream, const plImageView& image, plStringView sFileExtension) const
{
  // Technically almost arbitrary formats are supported, but we only use the common ones.
  plImageFormat::Enum compatibleFormats[] = {
    plImageFormat::R8G8B8A8_UNORM,
    plImageFormat::B8G8R8A8_UNORM,
    plImageFormat::B8G8R8X8_UNORM,
    plImageFormat::B8G8R8_UNORM,
  };

  // Find a compatible format closest to the one the image currently has
  plImageFormat::Enum format = plImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == plImageFormat::UNKNOWN)
  {
    plLog::Error("No conversion from format '{0}' to a format suitable for TGA files known.", plImageFormat::GetName(image.GetImageFormat()));
    return PLASMA_FAILURE;
  }

  // Convert if not already in a compatible format
  if (format != image.GetImageFormat())
  {
    plImage convertedImage;
    if (plImageConversion::Convert(image, convertedImage, format) != PLASMA_SUCCESS)
    {
      // This should never happen
      PLASMA_ASSERT_DEV(false, "plImageConversion::Convert failed even though the conversion was to the format returned by FindClosestCompatibleFormat.");
      return PLASMA_FAILURE;
    }

    return WriteImage(inout_stream, convertedImage, sFileExtension);
  }

  const bool bCompress = true;

  // Write the header
  {
    plUInt8 uiHeader[18];
    plMemoryUtils::ZeroFill(uiHeader, 18);

    if (!bCompress)
    {
      // uncompressed TGA
      uiHeader[2] = 2;
    }
    else
    {
      // compressed TGA
      uiHeader[2] = 10;
    }

    uiHeader[13] = static_cast<plUInt8>(image.GetWidth(0) / 256);
    uiHeader[15] = static_cast<plUInt8>(image.GetHeight(0) / 256);
    uiHeader[12] = static_cast<plUInt8>(image.GetWidth(0) % 256);
    uiHeader[14] = static_cast<plUInt8>(image.GetHeight(0) % 256);
    uiHeader[16] = static_cast<plUInt8>(plImageFormat::GetBitsPerPixel(image.GetImageFormat()));

    inout_stream.WriteBytes(uiHeader, 18).IgnoreResult();
  }

  const bool bAlpha = image.GetImageFormat() != plImageFormat::B8G8R8_UNORM;

  const plUInt32 uiWidth = image.GetWidth(0);
  const plUInt32 uiHeight = image.GetHeight(0);

  if (!bCompress)
  {
    // Write image uncompressed

    for (plUInt32 y = 0; y < uiWidth; ++y)
    {
      for (plUInt32 x = 0; x < uiHeight; ++x)
      {
        const plColorLinearUB c = GetPixelColor(image, x, y, uiHeight);

        inout_stream << c.b;
        inout_stream << c.g;
        inout_stream << c.r;

        if (bAlpha)
          inout_stream << c.a;
      }
    }
  }
  else
  {
    // write image RLE compressed

    plInt32 iRLE = 0;

    plColorLinearUB pc = {};
    plStaticArray<plColorLinearUB, 129> unequal;
    plInt32 iEqual = 0;

    for (plUInt32 y = 0; y < uiHeight; ++y)
    {
      for (plUInt32 x = 0; x < uiWidth; ++x)
      {
        const plColorLinearUB c = GetPixelColor(image, x, y, uiHeight);

        if (iRLE == 0) // no comparison possible yet
        {
          pc = c;
          iRLE = 1;
          unequal.PushBack(c);
        }
        else if (iRLE == 1) // has one value gathered for comparison
        {
          if (c == pc)
          {
            iRLE = 2;   // two values were equal
            iEqual = 2; // go into equal-mode
          }
          else
          {
            iRLE = 3; // two values were unequal
            pc = c;   // go into unequal-mode
            unequal.PushBack(c);
          }
        }
        else if (iRLE == 2) // equal values
        {
          if ((c == pc) && (iEqual < 128))
            ++iEqual;
          else
          {
            plUInt8 uiRepeat = static_cast<plUInt8>(iEqual + 127);

            inout_stream << uiRepeat;
            inout_stream << pc.b;
            inout_stream << pc.g;
            inout_stream << pc.r;

            if (bAlpha)
              inout_stream << pc.a;

            pc = c;
            iRLE = 1;
            unequal.Clear();
            unequal.PushBack(c);
          }
        }
        else if (iRLE == 3)
        {
          if ((c != pc) && (unequal.GetCount() < 128))
          {
            unequal.PushBack(c);
            pc = c;
          }
          else
          {
            plUInt8 uiRepeat = (unsigned char)(unequal.GetCount()) - 1;
            inout_stream << uiRepeat;

            for (plUInt32 i = 0; i < unequal.GetCount(); ++i)
            {
              inout_stream << unequal[i].b;
              inout_stream << unequal[i].g;
              inout_stream << unequal[i].r;

              if (bAlpha)
                inout_stream << unequal[i].a;
            }

            pc = c;
            iRLE = 1;
            unequal.Clear();
            unequal.PushBack(c);
          }
        }
      }
    }


    if (iRLE == 1) // has one value gathered for comparison
    {
      plUInt8 uiRepeat = 0;

      inout_stream << uiRepeat;
      inout_stream << pc.b;
      inout_stream << pc.g;
      inout_stream << pc.r;

      if (bAlpha)
        inout_stream << pc.a;
    }
    else if (iRLE == 2) // equal values
    {
      plUInt8 uiRepeat = static_cast<plUInt8>(iEqual + 127);

      inout_stream << uiRepeat;
      inout_stream << pc.b;
      inout_stream << pc.g;
      inout_stream << pc.r;

      if (bAlpha)
        inout_stream << pc.a;
    }
    else if (iRLE == 3)
    {
      plUInt8 uiRepeat = (plUInt8)(unequal.GetCount()) - 1;
      inout_stream << uiRepeat;

      for (plUInt32 i = 0; i < unequal.GetCount(); ++i)
      {
        inout_stream << unequal[i].b;
        inout_stream << unequal[i].g;
        inout_stream << unequal[i].r;

        if (bAlpha)
          inout_stream << unequal[i].a;
      }
    }
  }

  return PLASMA_SUCCESS;
}


static plResult ReadBytesChecked(plStreamReader& inout_stream, void* pDest, plUInt32 uiNumBytes)
{
  if (inout_stream.ReadBytes(pDest, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}

template <typename TYPE>
static plResult ReadBytesChecked(plStreamReader& inout_stream, TYPE& ref_dest)
{
  return ReadBytesChecked(inout_stream, &ref_dest, sizeof(TYPE));
}

static plResult ReadImageHeaderImpl(plStreamReader& inout_stream, plImageHeader& ref_header, plStringView sFileExtension, TgaHeader& ref_tgaHeader)
{
  PLASMA_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_iImageIDLength));
  PLASMA_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_Ignored1));
  PLASMA_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_ImageType));
  PLASMA_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, &ref_tgaHeader.m_Ignored2, 9));
  PLASMA_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_iImageWidth));
  PLASMA_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_iImageHeight));
  PLASMA_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_iBitsPerPixel));
  PLASMA_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_ImageDescriptor));

  // ignore optional data
  if (inout_stream.SkipBytes(ref_tgaHeader.m_iImageIDLength) != ref_tgaHeader.m_iImageIDLength)
    return PLASMA_FAILURE;

  const plUInt32 uiBytesPerPixel = ref_tgaHeader.m_iBitsPerPixel / 8;

  // check whether width, height an BitsPerPixel are valid
  if ((ref_tgaHeader.m_iImageWidth <= 0) || (ref_tgaHeader.m_iImageHeight <= 0) || ((uiBytesPerPixel != 1) && (uiBytesPerPixel != 3) && (uiBytesPerPixel != 4)) || (ref_tgaHeader.m_ImageType != 2 && ref_tgaHeader.m_ImageType != 3 && ref_tgaHeader.m_ImageType != 10 && ref_tgaHeader.m_ImageType != 11))
  {
    plLog::Error("TGA has an invalid header: Width = {0}, Height = {1}, BPP = {2}, ImageType = {3}", ref_tgaHeader.m_iImageWidth, ref_tgaHeader.m_iImageHeight, ref_tgaHeader.m_iBitsPerPixel, ref_tgaHeader.m_ImageType);
    return PLASMA_FAILURE;
  }

  // Set image data

  if (uiBytesPerPixel == 1)
    ref_header.SetImageFormat(plImageFormat::R8_UNORM);
  else if (uiBytesPerPixel == 3)
    ref_header.SetImageFormat(plImageFormat::B8G8R8_UNORM);
  else
    ref_header.SetImageFormat(plImageFormat::B8G8R8A8_UNORM);

  ref_header.SetNumMipLevels(1);
  ref_header.SetNumArrayIndices(1);
  ref_header.SetNumFaces(1);

  ref_header.SetWidth(ref_tgaHeader.m_iImageWidth);
  ref_header.SetHeight(ref_tgaHeader.m_iImageHeight);
  ref_header.SetDepth(1);

  return PLASMA_SUCCESS;
}

plResult plTgaFileFormat::ReadImageHeader(plStreamReader& inout_stream, plImageHeader& ref_header, plStringView sFileExtension) const
{
  PLASMA_PROFILE_SCOPE("plTgaFileFormat::ReadImageHeader");

  TgaHeader tgaHeader;
  return ReadImageHeaderImpl(inout_stream, ref_header, sFileExtension, tgaHeader);
}

plResult plTgaFileFormat::ReadImage(plStreamReader& inout_stream, plImage& ref_image, plStringView sFileExtension) const
{
  PLASMA_PROFILE_SCOPE("plTgaFileFormat::ReadImage");

  plImageHeader imageHeader;
  TgaHeader tgaHeader;
  PLASMA_SUCCEED_OR_RETURN(ReadImageHeaderImpl(inout_stream, imageHeader, sFileExtension, tgaHeader));

  const plUInt32 uiBytesPerPixel = tgaHeader.m_iBitsPerPixel / 8;

  ref_image.ResetAndAlloc(imageHeader);

  if (tgaHeader.m_ImageType == 3)
  {
    // uncompressed greyscale

    const plUInt32 uiBytesPerRow = uiBytesPerPixel * tgaHeader.m_iImageWidth;

    if (tgaHeader.m_ImageDescriptor.m_bFlipH)
    {
      // read each row (gets rid of the row pitch
      for (plInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        for (plInt32 x = tgaHeader.m_iImageWidth - 1; x >= 0; --x)
        {
          inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, x, row, 0), uiBytesPerPixel);
        }
      }
    }
    else
    {
      // read each row (gets rid of the row pitch
      for (plInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, 0, row, 0), uiBytesPerRow);
      }
    }
  }
  else if (tgaHeader.m_ImageType == 2)
  {
    // uncompressed

    const plUInt32 uiBytesPerRow = uiBytesPerPixel * tgaHeader.m_iImageWidth;

    if (tgaHeader.m_ImageDescriptor.m_bFlipH)
    {
      // read each row (gets rid of the row pitch
      for (plInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        for (plInt32 x = tgaHeader.m_iImageWidth - 1; x >= 0; --x)
        {
          inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, x, row, 0), uiBytesPerPixel);
        }
      }
    }
    else
    {
      // read each row (gets rid of the row pitch
      for (plInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, 0, row, 0), uiBytesPerRow);
      }
    }
  }
  else
  {
    // compressed

    plInt32 iCurrentPixel = 0;
    const int iPixelCount = tgaHeader.m_iImageWidth * tgaHeader.m_iImageHeight;

    do
    {
      plUInt8 uiChunkHeader = 0;

      PLASMA_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, uiChunkHeader));

      const plInt32 numToRead = (uiChunkHeader & 127) + 1;

      if (iCurrentPixel + numToRead > iPixelCount)
      {
        plLog::Error("TGA contents are invalid");
        return PLASMA_FAILURE;
      }

      if (uiChunkHeader < 128)
      {
        // If the header is < 128, it means it is the number of RAW color packets minus 1
        // that follow the header
        // add 1 to get number of following color values

        // Read RAW color values
        for (plInt32 i = 0; i < numToRead; ++i)
        {
          const plInt32 x = iCurrentPixel % tgaHeader.m_iImageWidth;
          const plInt32 y = iCurrentPixel / tgaHeader.m_iImageWidth;

          const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
          const auto col = tgaHeader.m_ImageDescriptor.m_bFlipH ? tgaHeader.m_iImageWidth - x - 1 : x;
          inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, col, row, 0), uiBytesPerPixel);

          ++iCurrentPixel;
        }
      }
      else // chunk header > 128 RLE data, next color repeated (chunk header - 127) times
      {
        plUInt8 uiBuffer[4] = {255, 255, 255, 255};

        // read the current color
        inout_stream.ReadBytes(uiBuffer, uiBytesPerPixel);

        // if it is a 24-Bit TGA (3 channels), the fourth channel stays at 255 all the time, since the 4th value in ucBuffer is never overwritten

        // copy the color into the image data as many times as dictated
        for (plInt32 i = 0; i < numToRead; ++i)
        {
          const plInt32 x = iCurrentPixel % tgaHeader.m_iImageWidth;
          const plInt32 y = iCurrentPixel / tgaHeader.m_iImageWidth;

          const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
          const auto col = tgaHeader.m_ImageDescriptor.m_bFlipH ? tgaHeader.m_iImageWidth - x - 1 : x;
          plUInt8* pPixel = ref_image.GetPixelPointer<plUInt8>(0, 0, 0, col, row, 0);

          // BGR
          pPixel[0] = uiBuffer[0];

          if (uiBytesPerPixel > 1)
          {
            pPixel[1] = uiBuffer[1];
            pPixel[2] = uiBuffer[2];

            // Alpha
            if (uiBytesPerPixel == 4)
              pPixel[3] = uiBuffer[3];
          }

          ++iCurrentPixel;
        }
      }
    } while (iCurrentPixel < iPixelCount);
  }

  return PLASMA_SUCCESS;
}

bool plTgaFileFormat::CanReadFileType(plStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("tga");
}

bool plTgaFileFormat::CanWriteFileType(plStringView sExtension) const
{
  return CanReadFileType(sExtension);
}



PLASMA_STATICLINK_FILE(Texture, Texture_Image_Formats_TgaFileFormat);

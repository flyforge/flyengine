#include <Texture/TexturePCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>
#include <Texture/Image/Image.h>

// PL_STATICLINK_FORCE
plDdsFileFormat g_ddsFormat;

struct plDdsPixelFormat
{
  plUInt32 m_uiSize;
  plUInt32 m_uiFlags;
  plUInt32 m_uiFourCC;
  plUInt32 m_uiRGBBitCount;
  plUInt32 m_uiRBitMask;
  plUInt32 m_uiGBitMask;
  plUInt32 m_uiBBitMask;
  plUInt32 m_uiABitMask;
};

struct plDdsHeader
{
  plUInt32 m_uiMagic;
  plUInt32 m_uiSize;
  plUInt32 m_uiFlags;
  plUInt32 m_uiHeight;
  plUInt32 m_uiWidth;
  plUInt32 m_uiPitchOrLinearSize;
  plUInt32 m_uiDepth;
  plUInt32 m_uiMipMapCount;
  plUInt32 m_uiReserved1[11];
  plDdsPixelFormat m_ddspf;
  plUInt32 m_uiCaps;
  plUInt32 m_uiCaps2;
  plUInt32 m_uiCaps3;
  plUInt32 m_uiCaps4;
  plUInt32 m_uiReserved2;
};

struct plDdsResourceDimension
{
  enum Enum
  {
    TEXTURE1D = 2,
    TEXTURE2D = 3,
    TEXTURE3D = 4,
  };
};

struct plDdsResourceMiscFlags
{
  enum Enum
  {
    TEXTURECUBE = 0x4,
  };
};

struct plDdsHeaderDxt10
{
  plUInt32 m_uiDxgiFormat;
  plUInt32 m_uiResourceDimension;
  plUInt32 m_uiMiscFlag;
  plUInt32 m_uiArraySize;
  plUInt32 m_uiMiscFlags2;
};

struct plDdsdFlags
{
  enum Enum
  {
    CAPS = 0x000001,
    HEIGHT = 0x000002,
    WIDTH = 0x000004,
    PITCH = 0x000008,
    PIXELFORMAT = 0x001000,
    MIPMAPCOUNT = 0x020000,
    LINEARSIZE = 0x080000,
    DEPTH = 0x800000,
  };
};

struct plDdpfFlags
{
  enum Enum
  {
    ALPHAPIXELS = 0x00001,
    ALPHA = 0x00002,
    FOURCC = 0x00004,
    RGB = 0x00040,
    YUV = 0x00200,
    LUMINANCE = 0x20000,
  };
};

struct plDdsCaps
{
  enum Enum
  {
    COMPLEX = 0x000008,
    MIPMAP = 0x400000,
    TEXTURE = 0x001000,
  };
};

struct plDdsCaps2
{
  enum Enum
  {
    CUBEMAP = 0x000200,
    CUBEMAP_POSITIVEX = 0x000400,
    CUBEMAP_NEGATIVEX = 0x000800,
    CUBEMAP_POSITIVEY = 0x001000,
    CUBEMAP_NEGATIVEY = 0x002000,
    CUBEMAP_POSITIVPL = 0x004000,
    CUBEMAP_NEGATIVPL = 0x008000,
    VOLUME = 0x200000,
  };
};

static const plUInt32 plDdsMagic = 0x20534444;
static const plUInt32 plDdsDxt10FourCc = 0x30315844;

static plResult ReadImageData(plStreamReader& inout_stream, plImageHeader& ref_imageHeader, plDdsHeader& ref_ddsHeader)
{
  if (inout_stream.ReadBytes(&ref_ddsHeader, sizeof(plDdsHeader)) != sizeof(plDdsHeader))
  {
    plLog::Error("Failed to read file header.");
    return PL_FAILURE;
  }

  if (ref_ddsHeader.m_uiMagic != plDdsMagic)
  {
    plLog::Error("The file is not a recognized DDS file.");
    return PL_FAILURE;
  }

  if (ref_ddsHeader.m_uiSize != 124)
  {
    plLog::Error("The file header size {0} doesn't match the expected size of 124.", ref_ddsHeader.m_uiSize);
    return PL_FAILURE;
  }

  // Required in every .dds file. According to the spec, CAPS and PIXELFORMAT are also required, but D3DX outputs
  // files not conforming to this.
  if ((ref_ddsHeader.m_uiFlags & plDdsdFlags::WIDTH) == 0 || (ref_ddsHeader.m_uiFlags & plDdsdFlags::HEIGHT) == 0)
  {
    plLog::Error("The file header doesn't specify the mandatory WIDTH or HEIGHT flag.");
    return PL_FAILURE;
  }

  if ((ref_ddsHeader.m_uiCaps & plDdsCaps::TEXTURE) == 0)
  {
    plLog::Error("The file header doesn't specify the mandatory TEXTURE flag.");
    return PL_FAILURE;
  }

  ref_imageHeader.SetWidth(ref_ddsHeader.m_uiWidth);
  ref_imageHeader.SetHeight(ref_ddsHeader.m_uiHeight);

  if (ref_ddsHeader.m_ddspf.m_uiSize != 32)
  {
    plLog::Error("The pixel format size {0} doesn't match the expected value of 32.", ref_ddsHeader.m_ddspf.m_uiSize);
    return PL_FAILURE;
  }

  plDdsHeaderDxt10 headerDxt10;

  plImageFormat::Enum format = plImageFormat::UNKNOWN;

  // Data format specified in RGBA masks
  if ((ref_ddsHeader.m_ddspf.m_uiFlags & plDdpfFlags::ALPHAPIXELS) != 0 || (ref_ddsHeader.m_ddspf.m_uiFlags & plDdpfFlags::RGB) != 0 ||
      (ref_ddsHeader.m_ddspf.m_uiFlags & plDdpfFlags::ALPHA) != 0)
  {
    format = plImageFormat::FromPixelMask(ref_ddsHeader.m_ddspf.m_uiRBitMask, ref_ddsHeader.m_ddspf.m_uiGBitMask, ref_ddsHeader.m_ddspf.m_uiBBitMask,
      ref_ddsHeader.m_ddspf.m_uiABitMask, ref_ddsHeader.m_ddspf.m_uiRGBBitCount);

    if (format == plImageFormat::UNKNOWN)
    {
      plLog::Error("The pixel mask specified was not recognized (R: {0}, G: {1}, B: {2}, A: {3}, Bpp: {4}).",
        plArgU(ref_ddsHeader.m_ddspf.m_uiRBitMask, 1, false, 16), plArgU(ref_ddsHeader.m_ddspf.m_uiGBitMask, 1, false, 16),
        plArgU(ref_ddsHeader.m_ddspf.m_uiBBitMask, 1, false, 16), plArgU(ref_ddsHeader.m_ddspf.m_uiABitMask, 1, false, 16),
        ref_ddsHeader.m_ddspf.m_uiRGBBitCount);
      return PL_FAILURE;
    }

    // Verify that the format we found is correct
    if (plImageFormat::GetBitsPerPixel(format) != ref_ddsHeader.m_ddspf.m_uiRGBBitCount)
    {
      plLog::Error("The number of bits per pixel specified in the file ({0}) does not match the expected value of {1} for the format '{2}'.",
        ref_ddsHeader.m_ddspf.m_uiRGBBitCount, plImageFormat::GetBitsPerPixel(format), plImageFormat::GetName(format));
      return PL_FAILURE;
    }
  }
  else if ((ref_ddsHeader.m_ddspf.m_uiFlags & plDdpfFlags::FOURCC) != 0)
  {
    if (ref_ddsHeader.m_ddspf.m_uiFourCC == plDdsDxt10FourCc)
    {
      if (inout_stream.ReadBytes(&headerDxt10, sizeof(plDdsHeaderDxt10)) != sizeof(plDdsHeaderDxt10))
      {
        plLog::Error("Failed to read file header.");
        return PL_FAILURE;
      }

      format = plImageFormatMappings::FromDxgiFormat(headerDxt10.m_uiDxgiFormat);

      if (format == plImageFormat::UNKNOWN)
      {
        plLog::Error("The DXGI format {0} has no equivalent image format.", headerDxt10.m_uiDxgiFormat);
        return PL_FAILURE;
      }
    }
    else
    {
      format = plImageFormatMappings::FromFourCc(ref_ddsHeader.m_ddspf.m_uiFourCC);

      if (format == plImageFormat::UNKNOWN)
      {
        plLog::Error("The FourCC code '{0}{1}{2}{3}' was not recognized.", plArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 0)),
          plArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 8)), plArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 16)),
          plArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 24)));
        return PL_FAILURE;
      }
    }
  }
  else
  {
    plLog::Error("The image format is neither specified as a pixel mask nor as a FourCC code.");
    return PL_FAILURE;
  }

  ref_imageHeader.SetImageFormat(format);

  const bool bHasMipMaps = (ref_ddsHeader.m_uiCaps & plDdsCaps::MIPMAP) != 0;
  const bool bCubeMap = (ref_ddsHeader.m_uiCaps2 & plDdsCaps2::CUBEMAP) != 0;
  const bool bVolume = (ref_ddsHeader.m_uiCaps2 & plDdsCaps2::VOLUME) != 0;


  if (bHasMipMaps)
  {
    ref_imageHeader.SetNumMipLevels(ref_ddsHeader.m_uiMipMapCount);
  }

  // Cubemap and volume texture are mutually exclusive
  if (bVolume && bCubeMap)
  {
    plLog::Error("The header specifies both the VOLUME and CUBEMAP flags.");
    return PL_FAILURE;
  }

  if (bCubeMap)
  {
    ref_imageHeader.SetNumFaces(6);
  }
  else if (bVolume)
  {
    ref_imageHeader.SetDepth(ref_ddsHeader.m_uiDepth);
  }

  return PL_SUCCESS;
}

plResult plDdsFileFormat::ReadImageHeader(plStreamReader& inout_stream, plImageHeader& ref_header, plStringView sFileExtension) const
{
  PL_PROFILE_SCOPE("plDdsFileFormat::ReadImageHeader");

  plDdsHeader ddsHeader;
  return ReadImageData(inout_stream, ref_header, ddsHeader);
}

plResult plDdsFileFormat::ReadImage(plStreamReader& inout_stream, plImage& ref_image, plStringView sFileExtension) const
{
  PL_PROFILE_SCOPE("plDdsFileFormat::ReadImage");

  plImageHeader imageHeader;
  plDdsHeader ddsHeader;
  PL_SUCCEED_OR_RETURN(ReadImageData(inout_stream, imageHeader, ddsHeader));

  ref_image.ResetAndAlloc(imageHeader);

  const bool bPitch = (ddsHeader.m_uiFlags & plDdsdFlags::PITCH) != 0;

  // If pitch is specified, it must match the computed value
  if (bPitch && ref_image.GetRowPitch(0) != ddsHeader.m_uiPitchOrLinearSize)
  {
    plLog::Error("The row pitch specified in the header doesn't match the expected pitch.");
    return PL_FAILURE;
  }

  plUInt64 uiDataSize = ref_image.GetByteBlobPtr().GetCount();

  if (inout_stream.ReadBytes(ref_image.GetByteBlobPtr().GetPtr(), uiDataSize) != uiDataSize)
  {
    plLog::Error("Failed to read image data.");
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

plResult plDdsFileFormat::WriteImage(plStreamWriter& inout_stream, const plImageView& image, plStringView sFileExtension) const
{
  const plImageFormat::Enum format = image.GetImageFormat();
  const plUInt32 uiBpp = plImageFormat::GetBitsPerPixel(format);

  const plUInt32 uiNumFaces = image.GetNumFaces();
  const plUInt32 uiNumMipLevels = image.GetNumMipLevels();
  const plUInt32 uiNumArrayIndices = image.GetNumArrayIndices();

  const plUInt32 uiWidth = image.GetWidth(0);
  const plUInt32 uiHeight = image.GetHeight(0);
  const plUInt32 uiDepth = image.GetDepth(0);

  bool bHasMipMaps = uiNumMipLevels > 1;
  bool bVolume = uiDepth > 1;
  bool bCubeMap = uiNumFaces > 1;
  bool bArray = uiNumArrayIndices > 1;

  bool bDxt10 = false;

  plDdsHeader fileHeader;
  plDdsHeaderDxt10 headerDxt10;

  plMemoryUtils::ZeroFill(&fileHeader, 1);
  plMemoryUtils::ZeroFill(&headerDxt10, 1);

  fileHeader.m_uiMagic = plDdsMagic;
  fileHeader.m_uiSize = 124;
  fileHeader.m_uiWidth = uiWidth;
  fileHeader.m_uiHeight = uiHeight;

  // Required in every .dds file.
  fileHeader.m_uiFlags = plDdsdFlags::WIDTH | plDdsdFlags::HEIGHT | plDdsdFlags::CAPS | plDdsdFlags::PIXELFORMAT;

  if (bHasMipMaps)
  {
    fileHeader.m_uiFlags |= plDdsdFlags::MIPMAPCOUNT;
    fileHeader.m_uiMipMapCount = uiNumMipLevels;
  }

  if (bVolume)
  {
    // Volume and array are incompatible
    if (bArray)
    {
      plLog::Error("The image is both an array and volume texture. This is not supported.");
      return PL_FAILURE;
    }

    fileHeader.m_uiFlags |= plDdsdFlags::DEPTH;
    fileHeader.m_uiDepth = uiDepth;
  }

  switch (plImageFormat::GetType(image.GetImageFormat()))
  {
    case plImageFormatType::LINEAR:
      [[fallthrough]];
    case plImageFormatType::PLANAR:
      fileHeader.m_uiFlags |= plDdsdFlags::PITCH;
      fileHeader.m_uiPitchOrLinearSize = static_cast<plUInt32>(image.GetRowPitch(0));
      break;

    case plImageFormatType::BLOCK_COMPRESSED:
      fileHeader.m_uiFlags |= plDdsdFlags::LINEARSIZE;
      fileHeader.m_uiPitchOrLinearSize = 0; /// \todo sub-image size
      break;

    default:
      plLog::Error("Unknown image format type.");
      return PL_FAILURE;
  }

  fileHeader.m_uiCaps = plDdsCaps::TEXTURE;

  if (bCubeMap)
  {
    if (uiNumFaces != 6)
    {
      plLog::Error("The image is a cubemap, but has {0} faces instead of the expected 6.", uiNumFaces);
      return PL_FAILURE;
    }

    if (bVolume)
    {
      plLog::Error("The image is both a cubemap and volume texture. This is not supported.");
      return PL_FAILURE;
    }

    fileHeader.m_uiCaps |= plDdsCaps::COMPLEX;
    fileHeader.m_uiCaps2 |= plDdsCaps2::CUBEMAP | plDdsCaps2::CUBEMAP_POSITIVEX | plDdsCaps2::CUBEMAP_NEGATIVEX | plDdsCaps2::CUBEMAP_POSITIVEY |
                            plDdsCaps2::CUBEMAP_NEGATIVEY | plDdsCaps2::CUBEMAP_POSITIVPL | plDdsCaps2::CUBEMAP_NEGATIVPL;
  }

  if (bArray)
  {
    fileHeader.m_uiCaps |= plDdsCaps::COMPLEX;

    // Must be written as DXT10
    bDxt10 = true;
  }

  if (bVolume)
  {
    fileHeader.m_uiCaps |= plDdsCaps::COMPLEX;
    fileHeader.m_uiCaps2 |= plDdsCaps2::VOLUME;
  }

  if (bHasMipMaps)
  {
    fileHeader.m_uiCaps |= plDdsCaps::MIPMAP | plDdsCaps::COMPLEX;
  }

  fileHeader.m_ddspf.m_uiSize = 32;

  plUInt32 uiRedMask = plImageFormat::GetRedMask(format);
  plUInt32 uiGreenMask = plImageFormat::GetGreenMask(format);
  plUInt32 uiBlueMask = plImageFormat::GetBlueMask(format);
  plUInt32 uiAlphaMask = plImageFormat::GetAlphaMask(format);

  plUInt32 uiFourCc = plImageFormatMappings::ToFourCc(format);
  plUInt32 uiDxgiFormat = plImageFormatMappings::ToDxgiFormat(format);

  // When not required to use a DXT10 texture, try to write a legacy DDS by specifying FourCC or pixel masks
  if (!bDxt10)
  {
    // The format has a known mask and we would also recognize it as the same when reading back in, since multiple formats may have the same pixel
    // masks
    if ((uiRedMask | uiGreenMask | uiBlueMask | uiAlphaMask) &&
        format == plImageFormat::FromPixelMask(uiRedMask, uiGreenMask, uiBlueMask, uiAlphaMask, uiBpp))
    {
      fileHeader.m_ddspf.m_uiFlags = plDdpfFlags::ALPHAPIXELS | plDdpfFlags::RGB;
      fileHeader.m_ddspf.m_uiRBitMask = uiRedMask;
      fileHeader.m_ddspf.m_uiGBitMask = uiGreenMask;
      fileHeader.m_ddspf.m_uiBBitMask = uiBlueMask;
      fileHeader.m_ddspf.m_uiABitMask = uiAlphaMask;
      fileHeader.m_ddspf.m_uiRGBBitCount = plImageFormat::GetBitsPerPixel(format);
    }
    // The format has a known FourCC
    else if (uiFourCc != 0)
    {
      fileHeader.m_ddspf.m_uiFlags = plDdpfFlags::FOURCC;
      fileHeader.m_ddspf.m_uiFourCC = uiFourCc;
    }
    else
    {
      // Fallback to DXT10 path
      bDxt10 = true;
    }
  }

  if (bDxt10)
  {
    // We must write a DXT10 file, but there is no matching DXGI_FORMAT - we could also try converting, but that is rarely intended when writing .dds
    if (uiDxgiFormat == 0)
    {
      plLog::Error("The image needs to be written as a DXT10 file, but no matching DXGI format was found for '{0}'.", plImageFormat::GetName(format));
      return PL_FAILURE;
    }

    fileHeader.m_ddspf.m_uiFlags = plDdpfFlags::FOURCC;
    fileHeader.m_ddspf.m_uiFourCC = plDdsDxt10FourCc;

    headerDxt10.m_uiDxgiFormat = uiDxgiFormat;

    if (bVolume)
    {
      headerDxt10.m_uiResourceDimension = plDdsResourceDimension::TEXTURE3D;
    }
    else if (uiHeight > 1)
    {
      headerDxt10.m_uiResourceDimension = plDdsResourceDimension::TEXTURE2D;
    }
    else
    {
      headerDxt10.m_uiResourceDimension = plDdsResourceDimension::TEXTURE1D;
    }

    if (bCubeMap)
    {
      headerDxt10.m_uiMiscFlag = plDdsResourceMiscFlags::TEXTURECUBE;
    }

    // NOT multiplied by number of cubemap faces
    headerDxt10.m_uiArraySize = uiNumArrayIndices;

    // Can be used to describe the alpha channel usage, but automatically makes it incompatible with the D3DX libraries if not 0.
    headerDxt10.m_uiMiscFlags2 = 0;
  }

  if (inout_stream.WriteBytes(&fileHeader, sizeof(fileHeader)) != PL_SUCCESS)
  {
    plLog::Error("Failed to write image header.");
    return PL_FAILURE;
  }

  if (bDxt10)
  {
    if (inout_stream.WriteBytes(&headerDxt10, sizeof(headerDxt10)) != PL_SUCCESS)
    {
      plLog::Error("Failed to write image DX10 header.");
      return PL_FAILURE;
    }
  }

  if (inout_stream.WriteBytes(image.GetByteBlobPtr().GetPtr(), image.GetByteBlobPtr().GetCount()) != PL_SUCCESS)
  {
    plLog::Error("Failed to write image data.");
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

bool plDdsFileFormat::CanReadFileType(plStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("dds");
}

bool plDdsFileFormat::CanWriteFileType(plStringView sExtension) const
{
  return CanReadFileType(sExtension);
}




PL_STATICLINK_FILE(Texture, Texture_Image_Formats_DdsFileFormat);


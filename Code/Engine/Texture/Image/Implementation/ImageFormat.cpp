#include <Texture/TexturePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/StaticArray.h>
#include <Texture/Image/ImageFormat.h>

namespace
{
  struct plImageFormatMetaData
  {
    plImageFormatMetaData()
    {
      plMemoryUtils::ZeroFillArray(m_uiBitsPerChannel);
      plMemoryUtils::ZeroFillArray(m_uiChannelMasks);

      m_planeData.SetCount(1);
    }

    const char* m_szName = nullptr;

    struct PlaneData
    {
      plUInt8 m_uiBitsPerBlock{0}; ///< Bits per block for compressed formats; for uncompressed formats (which always have a block size of 1x1x1), this is equal to bits per pixel.
      plUInt8 m_uiBlockWidth{1};
      plUInt8 m_uiBlockHeight{1};
      plUInt8 m_uiBlockDepth{1};
      plImageFormat::Enum m_subFormat{plImageFormat::UNKNOWN}; ///< Subformats when viewing only a subslice of the data.
    };

    plStaticArray<PlaneData, 2> m_planeData;

    plUInt8 m_uiNumChannels{0};

    plUInt8 m_uiBitsPerChannel[plImageFormatChannel::COUNT];
    plUInt32 m_uiChannelMasks[plImageFormatChannel::COUNT];


    bool m_requireFirstLevelBlockAligned{false}; ///< Only for compressed formats: If true, the first level's dimensions must be a multiple of the
                                                 ///< block size; if false, padding can be applied for compressing the first mip level, too.
    bool m_isDepth{false};
    bool m_isStencil{false};

    plImageFormatDataType::Enum m_dataType{plImageFormatDataType::NONE};
    plImageFormatType::Enum m_formatType{plImageFormatType::UNKNOWN};

    plImageFormat::Enum m_asLinear{plImageFormat::UNKNOWN};
    plImageFormat::Enum m_asSrgb{plImageFormat::UNKNOWN};

    plUInt32 getNumBlocksX(plUInt32 uiWidth, plUInt32 uiPlaneIndex) const
    {
      return (uiWidth - 1) / m_planeData[uiPlaneIndex].m_uiBlockWidth + 1;
    }

    plUInt32 getNumBlocksY(plUInt32 uiHeight, plUInt32 uiPlaneIndex) const
    {
      return (uiHeight - 1) / m_planeData[uiPlaneIndex].m_uiBlockHeight + 1;
    }

    plUInt32 getNumBlocksZ(plUInt32 uiDepth, plUInt32 uiPlaneIndex) const
    {
      return (uiDepth - 1) / m_planeData[uiPlaneIndex].m_uiBlockDepth + 1;
    }

    plUInt32 getRowPitch(plUInt32 uiWidth, plUInt32 uiPlaneIndex) const
    {
      return getNumBlocksX(uiWidth, uiPlaneIndex) * m_planeData[uiPlaneIndex].m_uiBitsPerBlock / 8;
    }
  };

  plStaticArray<plImageFormatMetaData, plImageFormat::NUM_FORMATS> s_formatMetaData;

  void InitFormatLinear(plImageFormat::Enum format, const char* szName, plImageFormatDataType::Enum dataType, plUInt8 uiBitsPerPixel, plUInt8 uiBitsR,
    plUInt8 uiBitsG, plUInt8 uiBitsB, plUInt8 uiBitsA, plUInt8 uiNumChannels)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_planeData[0].m_uiBitsPerBlock = uiBitsPerPixel;
    s_formatMetaData[format].m_dataType = dataType;
    s_formatMetaData[format].m_formatType = plImageFormatType::LINEAR;

    s_formatMetaData[format].m_uiNumChannels = uiNumChannels;

    s_formatMetaData[format].m_uiBitsPerChannel[plImageFormatChannel::R] = uiBitsR;
    s_formatMetaData[format].m_uiBitsPerChannel[plImageFormatChannel::G] = uiBitsG;
    s_formatMetaData[format].m_uiBitsPerChannel[plImageFormatChannel::B] = uiBitsB;
    s_formatMetaData[format].m_uiBitsPerChannel[plImageFormatChannel::A] = uiBitsA;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb = format;
  }

#define INIT_FORMAT_LINEAR(format, dataType, uiBitsPerPixel, uiBitsR, uiBitsG, uiBitsB, uiBitsA, uiNumChannels) \
  InitFormatLinear(plImageFormat::format, #format, plImageFormatDataType::dataType, uiBitsPerPixel, uiBitsR, uiBitsG, uiBitsB, uiBitsA, uiNumChannels)

  void InitFormatCompressed(plImageFormat::Enum format, const char* szName, plImageFormatDataType::Enum dataType, plUInt8 uiBitsPerBlock,
    plUInt8 uiBlockWidth, plUInt8 uiBlockHeight, plUInt8 uiBlockDepth, bool bRequireFirstLevelBlockAligned, plUInt8 uiNumChannels)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_planeData[0].m_uiBitsPerBlock = uiBitsPerBlock;
    s_formatMetaData[format].m_planeData[0].m_uiBlockWidth = uiBlockWidth;
    s_formatMetaData[format].m_planeData[0].m_uiBlockHeight = uiBlockHeight;
    s_formatMetaData[format].m_planeData[0].m_uiBlockDepth = uiBlockDepth;
    s_formatMetaData[format].m_dataType = dataType;
    s_formatMetaData[format].m_formatType = plImageFormatType::BLOCK_COMPRESSED;

    s_formatMetaData[format].m_uiNumChannels = uiNumChannels;

    s_formatMetaData[format].m_requireFirstLevelBlockAligned = bRequireFirstLevelBlockAligned;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb = format;
  }

#define INIT_FORMAT_COMPRESSED(                                                                                                                    \
  format, dataType, uiBitsPerBlock, uiBlockWidth, uiBlockHeight, uiBlockDepth, requireFirstLevelBlockAligned, uiNumChannels)                       \
  InitFormatCompressed(plImageFormat::format, #format, plImageFormatDataType::dataType, uiBitsPerBlock, uiBlockWidth, uiBlockHeight, uiBlockDepth, \
    requireFirstLevelBlockAligned, uiNumChannels)

  void InitFormatDepth(plImageFormat::Enum format, const char* szName, plImageFormatDataType::Enum dataType, plUInt8 uiBitsPerPixel, bool bIsStencil,
    plUInt8 uiBitsD, plUInt8 uiBitsS)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_planeData[0].m_uiBitsPerBlock = uiBitsPerPixel;
    s_formatMetaData[format].m_dataType = dataType;
    s_formatMetaData[format].m_formatType = plImageFormatType::LINEAR;

    s_formatMetaData[format].m_isDepth = true;
    s_formatMetaData[format].m_isStencil = bIsStencil;

    s_formatMetaData[format].m_uiNumChannels = bIsStencil ? 2 : 1;

    s_formatMetaData[format].m_uiBitsPerChannel[plImageFormatChannel::D] = uiBitsD;
    s_formatMetaData[format].m_uiBitsPerChannel[plImageFormatChannel::S] = uiBitsS;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb = format;
  }

#define INIT_FORMAT_DEPTH(format, dataType, uiBitsPerPixel, isStencil, uiBitsD, uiBitsS) \
  InitFormatDepth(plImageFormat::format, #format, plImageFormatDataType::dataType, uiBitsPerPixel, isStencil, uiBitsD, uiBitsS);

  void SetupSrgbPair(plImageFormat::Enum linearFormat, plImageFormat::Enum srgbFormat)
  {
    s_formatMetaData[linearFormat].m_asLinear = linearFormat;
    s_formatMetaData[linearFormat].m_asSrgb = srgbFormat;

    s_formatMetaData[srgbFormat].m_asLinear = linearFormat;
    s_formatMetaData[srgbFormat].m_asSrgb = srgbFormat;
  }

} // namespace

static void SetupImageFormatTable()
{
  if (!s_formatMetaData.IsEmpty())
    return;

  s_formatMetaData.SetCount(plImageFormat::NUM_FORMATS);

  s_formatMetaData[plImageFormat::UNKNOWN].m_szName = "UNKNOWN";

  INIT_FORMAT_LINEAR(R32G32B32A32_FLOAT, FLOAT, 128, 32, 32, 32, 32, 4);
  INIT_FORMAT_LINEAR(R32G32B32A32_UINT, UINT, 128, 32, 32, 32, 32, 4);
  INIT_FORMAT_LINEAR(R32G32B32A32_SINT, SINT, 128, 32, 32, 32, 32, 4);

  INIT_FORMAT_LINEAR(R32G32B32_FLOAT, FLOAT, 96, 32, 32, 32, 0, 3);
  INIT_FORMAT_LINEAR(R32G32B32_UINT, UINT, 96, 32, 32, 32, 0, 3);
  INIT_FORMAT_LINEAR(R32G32B32_SINT, SINT, 96, 32, 32, 32, 0, 3);

  INIT_FORMAT_LINEAR(R32G32_FLOAT, FLOAT, 64, 32, 32, 0, 0, 2);
  INIT_FORMAT_LINEAR(R32G32_UINT, UINT, 64, 32, 32, 0, 0, 2);
  INIT_FORMAT_LINEAR(R32G32_SINT, SINT, 64, 32, 32, 0, 0, 2);

  INIT_FORMAT_LINEAR(R32_FLOAT, FLOAT, 32, 32, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R32_UINT, UINT, 32, 32, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R32_SINT, SINT, 32, 32, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R16G16B16A16_FLOAT, FLOAT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_UINT, UINT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_SINT, SINT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_UNORM, UNORM, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_SNORM, SNORM, 64, 16, 16, 16, 16, 4);

  INIT_FORMAT_LINEAR(R16G16B16_UNORM, UNORM, 48, 16, 16, 16, 0, 3);

  INIT_FORMAT_LINEAR(R16G16_FLOAT, FLOAT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_UINT, UINT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_SINT, SINT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_UNORM, UNORM, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_SNORM, SNORM, 32, 16, 16, 0, 0, 2);

  INIT_FORMAT_LINEAR(R16_FLOAT, FLOAT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_UINT, UINT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_SINT, SINT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_UNORM, UNORM, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_SNORM, SNORM, 16, 16, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R8G8B8A8_UINT, UINT, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_SINT, SINT, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_SNORM, SNORM, 32, 8, 8, 8, 8, 4);

  INIT_FORMAT_LINEAR(R8G8B8A8_UNORM, UNORM, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 8, 4);
  SetupSrgbPair(plImageFormat::R8G8B8A8_UNORM, plImageFormat::R8G8B8A8_UNORM_SRGB);

  s_formatMetaData[plImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[plImageFormatChannel::R] = 0x000000FF;
  s_formatMetaData[plImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[plImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[plImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[plImageFormatChannel::B] = 0x00FF0000;
  s_formatMetaData[plImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[plImageFormatChannel::A] = 0xFF000000;

  INIT_FORMAT_LINEAR(R8G8B8_UNORM, UNORM, 24, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(R8G8B8_UNORM_SRGB, UNORM, 24, 8, 8, 8, 0, 3);
  SetupSrgbPair(plImageFormat::R8G8B8_UNORM, plImageFormat::R8G8B8_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B8G8R8A8_UNORM, UNORM, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(B8G8R8A8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 8, 4);
  SetupSrgbPair(plImageFormat::B8G8R8A8_UNORM, plImageFormat::B8G8R8A8_UNORM_SRGB);

  s_formatMetaData[plImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[plImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[plImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[plImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[plImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[plImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[plImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[plImageFormatChannel::A] = 0xFF000000;

  INIT_FORMAT_LINEAR(B8G8R8X8_UNORM, UNORM, 32, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(B8G8R8X8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 0, 3);
  SetupSrgbPair(plImageFormat::B8G8R8X8_UNORM, plImageFormat::B8G8R8X8_UNORM_SRGB);

  s_formatMetaData[plImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[plImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[plImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[plImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[plImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[plImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[plImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[plImageFormatChannel::A] = 0x00000000;

  INIT_FORMAT_LINEAR(B8G8R8_UNORM, UNORM, 24, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(B8G8R8_UNORM_SRGB, UNORM, 24, 8, 8, 8, 0, 3);
  SetupSrgbPair(plImageFormat::B8G8R8_UNORM, plImageFormat::B8G8R8_UNORM_SRGB);

  s_formatMetaData[plImageFormat::B8G8R8_UNORM].m_uiChannelMasks[plImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[plImageFormat::B8G8R8_UNORM].m_uiChannelMasks[plImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[plImageFormat::B8G8R8_UNORM].m_uiChannelMasks[plImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[plImageFormat::B8G8R8_UNORM].m_uiChannelMasks[plImageFormatChannel::A] = 0x00000000;

  INIT_FORMAT_LINEAR(R8G8_UINT, UINT, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_SINT, SINT, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_UNORM, UNORM, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_SNORM, SNORM, 16, 8, 8, 0, 0, 2);

  INIT_FORMAT_LINEAR(R8_UINT, UINT, 8, 8, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R8_SINT, SINT, 8, 8, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R8_SNORM, SNORM, 8, 8, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R8_UNORM, UNORM, 8, 8, 0, 0, 0, 1);
  s_formatMetaData[plImageFormat::R8_UNORM].m_uiChannelMasks[plImageFormatChannel::R] = 0xFF;
  s_formatMetaData[plImageFormat::R8_UNORM].m_uiChannelMasks[plImageFormatChannel::G] = 0x00;
  s_formatMetaData[plImageFormat::R8_UNORM].m_uiChannelMasks[plImageFormatChannel::B] = 0x00;
  s_formatMetaData[plImageFormat::R8_UNORM].m_uiChannelMasks[plImageFormatChannel::A] = 0x00;

  INIT_FORMAT_COMPRESSED(BC1_UNORM, UNORM, 64, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC1_UNORM_SRGB, UNORM, 64, 4, 4, 1, true, 4);
  SetupSrgbPair(plImageFormat::BC1_UNORM, plImageFormat::BC1_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC2_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC2_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(plImageFormat::BC2_UNORM, plImageFormat::BC2_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC3_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC3_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(plImageFormat::BC3_UNORM, plImageFormat::BC3_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC4_UNORM, UNORM, 64, 4, 4, 1, true, 1);
  INIT_FORMAT_COMPRESSED(BC4_SNORM, SNORM, 64, 4, 4, 1, true, 1);

  INIT_FORMAT_COMPRESSED(BC5_UNORM, UNORM, 128, 4, 4, 1, true, 2);
  INIT_FORMAT_COMPRESSED(BC5_SNORM, SNORM, 128, 4, 4, 1, true, 2);

  INIT_FORMAT_COMPRESSED(BC6H_UF16, FLOAT, 128, 4, 4, 1, true, 3);
  INIT_FORMAT_COMPRESSED(BC6H_SF16, FLOAT, 128, 4, 4, 1, true, 3);

  INIT_FORMAT_COMPRESSED(BC7_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC7_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(plImageFormat::BC7_UNORM, plImageFormat::BC7_UNORM_SRGB);



  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM, UNORM, 16, 5, 5, 5, 1, 4);
  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 4);
  SetupSrgbPair(plImageFormat::B5G5R5A1_UNORM, plImageFormat::B5G5R5A1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B4G4R4A4_UNORM, UNORM, 16, 4, 4, 4, 4, 4);
  s_formatMetaData[plImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[plImageFormatChannel::R] = 0x0F00;
  s_formatMetaData[plImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[plImageFormatChannel::G] = 0x00F0;
  s_formatMetaData[plImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[plImageFormatChannel::B] = 0x000F;
  s_formatMetaData[plImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[plImageFormatChannel::A] = 0xF000;
  INIT_FORMAT_LINEAR(B4G4R4A4_UNORM_SRGB, UNORM, 16, 4, 4, 4, 4, 4);
  SetupSrgbPair(plImageFormat::B4G4R4A4_UNORM, plImageFormat::B4G4R4A4_UNORM_SRGB);

  INIT_FORMAT_LINEAR(A4B4G4R4_UNORM, UNORM, 16, 4, 4, 4, 4, 4);
  s_formatMetaData[plImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[plImageFormatChannel::R] = 0xF000;
  s_formatMetaData[plImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[plImageFormatChannel::G] = 0x0F00;
  s_formatMetaData[plImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[plImageFormatChannel::B] = 0x00F0;
  s_formatMetaData[plImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[plImageFormatChannel::A] = 0x000F;
  INIT_FORMAT_LINEAR(A4B4G4R4_UNORM_SRGB, UNORM, 16, 4, 4, 4, 4, 4);
  SetupSrgbPair(plImageFormat::A4B4G4R4_UNORM, plImageFormat::A4B4G4R4_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G6R5_UNORM, UNORM, 16, 5, 6, 5, 0, 3);
  s_formatMetaData[plImageFormat::B5G6R5_UNORM].m_uiChannelMasks[plImageFormatChannel::R] = 0xF800;
  s_formatMetaData[plImageFormat::B5G6R5_UNORM].m_uiChannelMasks[plImageFormatChannel::G] = 0x07E0;
  s_formatMetaData[plImageFormat::B5G6R5_UNORM].m_uiChannelMasks[plImageFormatChannel::B] = 0x001F;
  s_formatMetaData[plImageFormat::B5G6R5_UNORM].m_uiChannelMasks[plImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(B5G6R5_UNORM_SRGB, UNORM, 16, 5, 6, 5, 0, 3);
  SetupSrgbPair(plImageFormat::B5G6R5_UNORM, plImageFormat::B5G6R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[plImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[plImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[plImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[plImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[plImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[plImageFormatChannel::B] = 0x001F;
  s_formatMetaData[plImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[plImageFormatChannel::A] = 0x8000;
  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(plImageFormat::B5G5R5A1_UNORM, plImageFormat::B5G5R5A1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G5R5X1_UNORM, UNORM, 16, 5, 5, 5, 0, 3);
  s_formatMetaData[plImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[plImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[plImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[plImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[plImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[plImageFormatChannel::B] = 0x001F;
  s_formatMetaData[plImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[plImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(B5G5R5X1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 0, 3);
  SetupSrgbPair(plImageFormat::B5G5R5X1_UNORM, plImageFormat::B5G5R5X1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(A1B5G5R5_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[plImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[plImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[plImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[plImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[plImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[plImageFormatChannel::B] = 0x001F;
  s_formatMetaData[plImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[plImageFormatChannel::A] = 0x8000;
  INIT_FORMAT_LINEAR(A1B5G5R5_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(plImageFormat::A1B5G5R5_UNORM, plImageFormat::A1B5G5R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(X1B5G5R5_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[plImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[plImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[plImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[plImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[plImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[plImageFormatChannel::B] = 0x001F;
  s_formatMetaData[plImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[plImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(X1B5G5R5_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(plImageFormat::X1B5G5R5_UNORM, plImageFormat::X1B5G5R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(R11G11B10_FLOAT, FLOAT, 32, 11, 11, 10, 0, 3);
  INIT_FORMAT_LINEAR(R10G10B10A2_UINT, UINT, 32, 10, 10, 10, 2, 4);
  INIT_FORMAT_LINEAR(R10G10B10A2_UNORM, UNORM, 32, 10, 10, 10, 2, 4);

  // msdn.microsoft.com/library/windows/desktop/bb943991(v=vs.85).aspx documents R10G10B10A2 as having an alpha mask of 0
  s_formatMetaData[plImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[plImageFormatChannel::R] = 0x000003FF;
  s_formatMetaData[plImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[plImageFormatChannel::G] = 0x000FFC00;
  s_formatMetaData[plImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[plImageFormatChannel::B] = 0x3FF00000;
  s_formatMetaData[plImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[plImageFormatChannel::A] = 0;

  INIT_FORMAT_DEPTH(D32_FLOAT, DEPTH_STENCIL, 32, false, 32, 0);
  INIT_FORMAT_DEPTH(D32_FLOAT_S8X24_UINT, DEPTH_STENCIL, 64, true, 32, 8);
  INIT_FORMAT_DEPTH(D24_UNORM_S8_UINT, DEPTH_STENCIL, 32, true, 24, 8);
  INIT_FORMAT_DEPTH(D16_UNORM, DEPTH_STENCIL, 16, false, 16, 0);

  INIT_FORMAT_COMPRESSED(ASTC_4x4_UNORM, UNORM, 128, 4, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x4_UNORM, UNORM, 128, 5, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x5_UNORM, UNORM, 128, 5, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x5_UNORM, UNORM, 128, 6, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x6_UNORM, UNORM, 128, 6, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x5_UNORM, UNORM, 128, 8, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x6_UNORM, UNORM, 128, 8, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x5_UNORM, UNORM, 128, 10, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x6_UNORM, UNORM, 128, 10, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x8_UNORM, UNORM, 128, 8, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x8_UNORM, UNORM, 128, 10, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x10_UNORM, UNORM, 128, 10, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x10_UNORM, UNORM, 128, 12, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x12_UNORM, UNORM, 128, 12, 12, 1, false, 4);

  INIT_FORMAT_COMPRESSED(ASTC_4x4_UNORM_SRGB, UNORM, 128, 4, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x4_UNORM_SRGB, UNORM, 128, 5, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x5_UNORM_SRGB, UNORM, 128, 5, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x5_UNORM_SRGB, UNORM, 128, 6, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x6_UNORM_SRGB, UNORM, 128, 6, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x5_UNORM_SRGB, UNORM, 128, 8, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x6_UNORM_SRGB, UNORM, 128, 8, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x5_UNORM_SRGB, UNORM, 128, 10, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x6_UNORM_SRGB, UNORM, 128, 10, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x8_UNORM_SRGB, UNORM, 128, 8, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x8_UNORM_SRGB, UNORM, 128, 10, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x10_UNORM_SRGB, UNORM, 128, 10, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x10_UNORM_SRGB, UNORM, 128, 12, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x12_UNORM_SRGB, UNORM, 128, 12, 12, 1, false, 4);

  SetupSrgbPair(plImageFormat::ASTC_4x4_UNORM, plImageFormat::ASTC_4x4_UNORM_SRGB);
  SetupSrgbPair(plImageFormat::ASTC_5x4_UNORM, plImageFormat::ASTC_5x4_UNORM_SRGB);
  SetupSrgbPair(plImageFormat::ASTC_5x5_UNORM, plImageFormat::ASTC_5x5_UNORM_SRGB);
  SetupSrgbPair(plImageFormat::ASTC_6x5_UNORM, plImageFormat::ASTC_6x5_UNORM_SRGB);
  SetupSrgbPair(plImageFormat::ASTC_6x6_UNORM, plImageFormat::ASTC_6x6_UNORM_SRGB);
  SetupSrgbPair(plImageFormat::ASTC_8x5_UNORM, plImageFormat::ASTC_8x5_UNORM_SRGB);
  SetupSrgbPair(plImageFormat::ASTC_8x6_UNORM, plImageFormat::ASTC_8x6_UNORM_SRGB);
  SetupSrgbPair(plImageFormat::ASTC_10x5_UNORM, plImageFormat::ASTC_10x5_UNORM_SRGB);
  SetupSrgbPair(plImageFormat::ASTC_10x6_UNORM, plImageFormat::ASTC_10x6_UNORM_SRGB);
  SetupSrgbPair(plImageFormat::ASTC_8x8_UNORM, plImageFormat::ASTC_8x8_UNORM_SRGB);
  SetupSrgbPair(plImageFormat::ASTC_10x8_UNORM, plImageFormat::ASTC_10x8_UNORM_SRGB);
  SetupSrgbPair(plImageFormat::ASTC_10x10_UNORM, plImageFormat::ASTC_10x10_UNORM_SRGB);
  SetupSrgbPair(plImageFormat::ASTC_12x10_UNORM, plImageFormat::ASTC_12x10_UNORM_SRGB);
  SetupSrgbPair(plImageFormat::ASTC_12x12_UNORM, plImageFormat::ASTC_12x12_UNORM_SRGB);

  s_formatMetaData[plImageFormat::NV12].m_szName = "NV12";
  s_formatMetaData[plImageFormat::NV12].m_formatType = plImageFormatType::PLANAR;
  s_formatMetaData[plImageFormat::NV12].m_uiNumChannels = 3;

  s_formatMetaData[plImageFormat::NV12].m_planeData.SetCount(2);

  s_formatMetaData[plImageFormat::NV12].m_planeData[0].m_uiBitsPerBlock = 8;
  s_formatMetaData[plImageFormat::NV12].m_planeData[0].m_uiBlockWidth = 1;
  s_formatMetaData[plImageFormat::NV12].m_planeData[0].m_uiBlockHeight = 1;
  s_formatMetaData[plImageFormat::NV12].m_planeData[0].m_uiBlockDepth = 1;
  s_formatMetaData[plImageFormat::NV12].m_planeData[0].m_subFormat = plImageFormat::R8_UNORM;

  s_formatMetaData[plImageFormat::NV12].m_planeData[1].m_uiBitsPerBlock = 16;
  s_formatMetaData[plImageFormat::NV12].m_planeData[1].m_uiBlockWidth = 2;
  s_formatMetaData[plImageFormat::NV12].m_planeData[1].m_uiBlockHeight = 2;
  s_formatMetaData[plImageFormat::NV12].m_planeData[1].m_uiBlockDepth = 1;
  s_formatMetaData[plImageFormat::NV12].m_planeData[1].m_subFormat = plImageFormat::R8G8_UNORM;
}

static const PL_ALWAYS_INLINE plImageFormatMetaData& GetImageFormatMetaData(plImageFormat::Enum format)
{
  if (s_formatMetaData.IsEmpty())
  {
    SetupImageFormatTable();
  }

  return s_formatMetaData[format];
}

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(Image, ImageFormats)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    SetupImageFormatTable();
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

plUInt32 plImageFormat::GetBitsPerPixel(Enum format, plUInt32 uiPlaneIndex)
{
  const plImageFormatMetaData& metaData = GetImageFormatMetaData(format);
  auto pixelsPerBlock = metaData.m_planeData[uiPlaneIndex].m_uiBlockWidth * metaData.m_planeData[uiPlaneIndex].m_uiBlockHeight * metaData.m_planeData[uiPlaneIndex].m_uiBlockDepth;
  return (metaData.m_planeData[uiPlaneIndex].m_uiBitsPerBlock + pixelsPerBlock - 1) / pixelsPerBlock; // Return rounded-up value
}


float plImageFormat::GetExactBitsPerPixel(Enum format, plUInt32 uiPlaneIndex)
{
  const plImageFormatMetaData& metaData = GetImageFormatMetaData(format);
  auto pixelsPerBlock = metaData.m_planeData[uiPlaneIndex].m_uiBlockWidth * metaData.m_planeData[uiPlaneIndex].m_uiBlockHeight * metaData.m_planeData[uiPlaneIndex].m_uiBlockDepth;
  return static_cast<float>(metaData.m_planeData[uiPlaneIndex].m_uiBitsPerBlock) / pixelsPerBlock;
}


plUInt32 plImageFormat::GetBitsPerBlock(Enum format, plUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBitsPerBlock;
}


plUInt32 plImageFormat::GetNumChannels(Enum format)
{
  return GetImageFormatMetaData(format).m_uiNumChannels;
}

plImageFormat::Enum plImageFormat::FromPixelMask(
  plUInt32 uiRedMask, plUInt32 uiGreenMask, plUInt32 uiBlueMask, plUInt32 uiAlphaMask, plUInt32 uiBitsPerPixel)
{
  // Some DDS files in the wild are encoded as this
  if (uiBitsPerPixel == 8 && uiRedMask == 0xff && uiGreenMask == 0xff && uiBlueMask == 0xff)
  {
    return R8_UNORM;
  }

  for (plUInt32 index = 0; index < NUM_FORMATS; index++)
  {
    Enum format = static_cast<Enum>(index);
    if (GetChannelMask(format, plImageFormatChannel::R) == uiRedMask && GetChannelMask(format, plImageFormatChannel::G) == uiGreenMask &&
        GetChannelMask(format, plImageFormatChannel::B) == uiBlueMask && GetChannelMask(format, plImageFormatChannel::A) == uiAlphaMask &&
        GetBitsPerPixel(format) == uiBitsPerPixel && GetDataType(format) == plImageFormatDataType::UNORM && !IsCompressed(format))
    {
      return format;
    }
  }

  return UNKNOWN;
}


plImageFormat::Enum plImageFormat::GetPlaneSubFormat(Enum format, plUInt32 uiPlaneIndex)
{
  const auto& metadata = GetImageFormatMetaData(format);

  if (metadata.m_formatType == plImageFormatType::PLANAR)
  {
    return metadata.m_planeData[uiPlaneIndex].m_subFormat;
  }
  else
  {
    PL_ASSERT_DEV(uiPlaneIndex == 0, "Invalid plane index {0} for format {0}", uiPlaneIndex, plImageFormat::GetName(format));
    return format;
  }
}

bool plImageFormat::IsCompatible(Enum left, Enum right)
{
  if (left == right)
  {
    return true;
  }
  switch (left)
  {
    case plImageFormat::R32G32B32A32_FLOAT:
    case plImageFormat::R32G32B32A32_UINT:
    case plImageFormat::R32G32B32A32_SINT:
      return (right == plImageFormat::R32G32B32A32_FLOAT || right == plImageFormat::R32G32B32A32_UINT || right == plImageFormat::R32G32B32A32_SINT);
    case plImageFormat::R32G32B32_FLOAT:
    case plImageFormat::R32G32B32_UINT:
    case plImageFormat::R32G32B32_SINT:
      return (right == plImageFormat::R32G32B32_FLOAT || right == plImageFormat::R32G32B32_UINT || right == plImageFormat::R32G32B32_SINT);
    case plImageFormat::R32G32_FLOAT:
    case plImageFormat::R32G32_UINT:
    case plImageFormat::R32G32_SINT:
      return (right == plImageFormat::R32G32_FLOAT || right == plImageFormat::R32G32_UINT || right == plImageFormat::R32G32_SINT);
    case plImageFormat::R32_FLOAT:
    case plImageFormat::R32_UINT:
    case plImageFormat::R32_SINT:
      return (right == plImageFormat::R32_FLOAT || right == plImageFormat::R32_UINT || right == plImageFormat::R32_SINT);
    case plImageFormat::R16G16B16A16_FLOAT:
    case plImageFormat::R16G16B16A16_UINT:
    case plImageFormat::R16G16B16A16_SINT:
    case plImageFormat::R16G16B16A16_UNORM:
    case plImageFormat::R16G16B16A16_SNORM:
      return (right == plImageFormat::R16G16B16A16_FLOAT || right == plImageFormat::R16G16B16A16_UINT || right == plImageFormat::R16G16B16A16_SINT ||
              right == plImageFormat::R16G16B16A16_UNORM || right == plImageFormat::R16G16B16A16_SNORM);
    case plImageFormat::R16G16_FLOAT:
    case plImageFormat::R16G16_UINT:
    case plImageFormat::R16G16_SINT:
    case plImageFormat::R16G16_UNORM:
    case plImageFormat::R16G16_SNORM:
      return (right == plImageFormat::R16G16_FLOAT || right == plImageFormat::R16G16_UINT || right == plImageFormat::R16G16_SINT ||
              right == plImageFormat::R16G16_UNORM || right == plImageFormat::R16G16_SNORM);
    case plImageFormat::R8G8B8A8_UINT:
    case plImageFormat::R8G8B8A8_SINT:
    case plImageFormat::R8G8B8A8_UNORM:
    case plImageFormat::R8G8B8A8_SNORM:
    case plImageFormat::R8G8B8A8_UNORM_SRGB:
      return (right == plImageFormat::R8G8B8A8_UINT || right == plImageFormat::R8G8B8A8_SINT || right == plImageFormat::R8G8B8A8_UNORM ||
              right == plImageFormat::R8G8B8A8_SNORM || right == plImageFormat::R8G8B8A8_UNORM_SRGB);
    case plImageFormat::B8G8R8A8_UNORM:
    case plImageFormat::B8G8R8A8_UNORM_SRGB:
      return (right == plImageFormat::B8G8R8A8_UNORM || right == plImageFormat::B8G8R8A8_UNORM_SRGB);
    case plImageFormat::B8G8R8X8_UNORM:
    case plImageFormat::B8G8R8X8_UNORM_SRGB:
      return (right == plImageFormat::B8G8R8X8_UNORM || right == plImageFormat::B8G8R8X8_UNORM_SRGB);
    case plImageFormat::B8G8R8_UNORM:
    case plImageFormat::B8G8R8_UNORM_SRGB:
      return (right == plImageFormat::B8G8R8_UNORM || right == plImageFormat::B8G8R8_UNORM_SRGB);
    case plImageFormat::R8G8_UINT:
    case plImageFormat::R8G8_SINT:
    case plImageFormat::R8G8_UNORM:
    case plImageFormat::R8G8_SNORM:
      return (right == plImageFormat::R8G8_UINT || right == plImageFormat::R8G8_SINT || right == plImageFormat::R8G8_UNORM ||
              right == plImageFormat::R8G8_SNORM);
    case plImageFormat::R8_UINT:
    case plImageFormat::R8_SINT:
    case plImageFormat::R8_UNORM:
    case plImageFormat::R8_SNORM:
      return (
        right == plImageFormat::R8_UINT || right == plImageFormat::R8_SINT || right == plImageFormat::R8_UNORM || right == plImageFormat::R8_SNORM);
    case plImageFormat::BC1_UNORM:
    case plImageFormat::BC1_UNORM_SRGB:
      return (right == plImageFormat::BC1_UNORM || right == plImageFormat::BC1_UNORM_SRGB);
    case plImageFormat::BC2_UNORM:
    case plImageFormat::BC2_UNORM_SRGB:
      return (right == plImageFormat::BC2_UNORM || right == plImageFormat::BC2_UNORM_SRGB);
    case plImageFormat::BC3_UNORM:
    case plImageFormat::BC3_UNORM_SRGB:
      return (right == plImageFormat::BC3_UNORM || right == plImageFormat::BC3_UNORM_SRGB);
    case plImageFormat::BC4_UNORM:
    case plImageFormat::BC4_SNORM:
      return (right == plImageFormat::BC4_UNORM || right == plImageFormat::BC4_SNORM);
    case plImageFormat::BC5_UNORM:
    case plImageFormat::BC5_SNORM:
      return (right == plImageFormat::BC5_UNORM || right == plImageFormat::BC5_SNORM);
    case plImageFormat::BC6H_UF16:
    case plImageFormat::BC6H_SF16:
      return (right == plImageFormat::BC6H_UF16 || right == plImageFormat::BC6H_SF16);
    case plImageFormat::BC7_UNORM:
    case plImageFormat::BC7_UNORM_SRGB:
      return (right == plImageFormat::BC7_UNORM || right == plImageFormat::BC7_UNORM_SRGB);
    case plImageFormat::R10G10B10A2_UINT:
    case plImageFormat::R10G10B10A2_UNORM:
      return (right == plImageFormat::R10G10B10A2_UINT || right == plImageFormat::R10G10B10A2_UNORM);
    case plImageFormat::ASTC_4x4_UNORM:
    case plImageFormat::ASTC_4x4_UNORM_SRGB:
      return (right == plImageFormat::ASTC_4x4_UNORM || right == plImageFormat::ASTC_4x4_UNORM_SRGB);
    case plImageFormat::ASTC_5x4_UNORM:
    case plImageFormat::ASTC_5x4_UNORM_SRGB:
      return (right == plImageFormat::ASTC_5x4_UNORM || right == plImageFormat::ASTC_5x4_UNORM_SRGB);
    case plImageFormat::ASTC_5x5_UNORM:
    case plImageFormat::ASTC_5x5_UNORM_SRGB:
      return (right == plImageFormat::ASTC_5x5_UNORM || right == plImageFormat::ASTC_5x5_UNORM_SRGB);
    case plImageFormat::ASTC_6x5_UNORM:
    case plImageFormat::ASTC_6x5_UNORM_SRGB:
      return (right == plImageFormat::ASTC_6x5_UNORM || right == plImageFormat::ASTC_6x5_UNORM_SRGB);
    case plImageFormat::ASTC_6x6_UNORM:
    case plImageFormat::ASTC_6x6_UNORM_SRGB:
      return (right == plImageFormat::ASTC_6x6_UNORM || right == plImageFormat::ASTC_6x6_UNORM_SRGB);
    case plImageFormat::ASTC_8x5_UNORM:
    case plImageFormat::ASTC_8x5_UNORM_SRGB:
      return (right == plImageFormat::ASTC_8x5_UNORM || right == plImageFormat::ASTC_8x5_UNORM_SRGB);
    case plImageFormat::ASTC_8x6_UNORM:
    case plImageFormat::ASTC_8x6_UNORM_SRGB:
      return (right == plImageFormat::ASTC_8x6_UNORM || right == plImageFormat::ASTC_8x6_UNORM_SRGB);
    case plImageFormat::ASTC_10x5_UNORM:
    case plImageFormat::ASTC_10x5_UNORM_SRGB:
      return (right == plImageFormat::ASTC_10x5_UNORM || right == plImageFormat::ASTC_10x5_UNORM_SRGB);
    case plImageFormat::ASTC_10x6_UNORM:
    case plImageFormat::ASTC_10x6_UNORM_SRGB:
      return (right == plImageFormat::ASTC_10x6_UNORM || right == plImageFormat::ASTC_10x6_UNORM_SRGB);
    case plImageFormat::ASTC_8x8_UNORM:
    case plImageFormat::ASTC_8x8_UNORM_SRGB:
      return (right == plImageFormat::ASTC_8x8_UNORM || right == plImageFormat::ASTC_8x8_UNORM_SRGB);
    case plImageFormat::ASTC_10x8_UNORM:
    case plImageFormat::ASTC_10x8_UNORM_SRGB:
      return (right == plImageFormat::ASTC_10x8_UNORM || right == plImageFormat::ASTC_10x8_UNORM_SRGB);
    case plImageFormat::ASTC_10x10_UNORM:
    case plImageFormat::ASTC_10x10_UNORM_SRGB:
      return (right == plImageFormat::ASTC_10x10_UNORM || right == plImageFormat::ASTC_10x10_UNORM_SRGB);
    case plImageFormat::ASTC_12x10_UNORM:
    case plImageFormat::ASTC_12x10_UNORM_SRGB:
      return (right == plImageFormat::ASTC_12x10_UNORM || right == plImageFormat::ASTC_12x10_UNORM_SRGB);
    case plImageFormat::ASTC_12x12_UNORM:
    case plImageFormat::ASTC_12x12_UNORM_SRGB:
      return (right == plImageFormat::ASTC_12x12_UNORM || right == plImageFormat::ASTC_12x12_UNORM_SRGB);
    default:
      PL_ASSERT_DEV(false, "Encountered unhandled format: {0}", plImageFormat::GetName(left));
      return false;
  }
}


bool plImageFormat::RequiresFirstLevelBlockAlignment(Enum format)
{
  return GetImageFormatMetaData(format).m_requireFirstLevelBlockAligned;
}

const char* plImageFormat::GetName(Enum format)
{
  return GetImageFormatMetaData(format).m_szName;
}

plUInt32 plImageFormat::GetPlaneCount(Enum format)
{
  return GetImageFormatMetaData(format).m_planeData.GetCount();
}

plUInt32 plImageFormat::GetChannelMask(Enum format, plImageFormatChannel::Enum c)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[c];
}

plUInt32 plImageFormat::GetBitsPerChannel(Enum format, plImageFormatChannel::Enum c)
{
  return GetImageFormatMetaData(format).m_uiBitsPerChannel[c];
}

plUInt32 plImageFormat::GetRedMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[plImageFormatChannel::R];
}

plUInt32 plImageFormat::GetGreenMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[plImageFormatChannel::G];
}

plUInt32 plImageFormat::GetBlueMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[plImageFormatChannel::B];
}

plUInt32 plImageFormat::GetAlphaMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[plImageFormatChannel::A];
}

plUInt32 plImageFormat::GetBlockWidth(Enum format, plUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBlockWidth;
}

plUInt32 plImageFormat::GetBlockHeight(Enum format, plUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBlockHeight;
}

plUInt32 plImageFormat::GetBlockDepth(Enum format, plUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBlockDepth;
}

plImageFormatDataType::Enum plImageFormat::GetDataType(Enum format)
{
  return GetImageFormatMetaData(format).m_dataType;
}

bool plImageFormat::IsCompressed(Enum format)
{
  return GetImageFormatMetaData(format).m_formatType == plImageFormatType::BLOCK_COMPRESSED;
}

bool plImageFormat::IsDepth(Enum format)
{
  return GetImageFormatMetaData(format).m_isDepth;
}

bool plImageFormat::IsSrgb(Enum format)
{
  return GetImageFormatMetaData(format).m_asLinear != format;
}

bool plImageFormat::IsStencil(Enum format)
{
  return GetImageFormatMetaData(format).m_isStencil;
}

plImageFormat::Enum plImageFormat::AsSrgb(Enum format)
{
  return GetImageFormatMetaData(format).m_asSrgb;
}

plImageFormat::Enum plImageFormat::AsLinear(Enum format)
{
  return GetImageFormatMetaData(format).m_asLinear;
}

plUInt32 plImageFormat::GetNumBlocksX(Enum format, plUInt32 uiWidth, plUInt32 uiPlaneIndex)
{
  return (uiWidth - 1) / GetBlockWidth(format, uiPlaneIndex) + 1;
}

plUInt32 plImageFormat::GetNumBlocksY(Enum format, plUInt32 uiHeight, plUInt32 uiPlaneIndex)
{
  return (uiHeight - 1) / GetBlockHeight(format, uiPlaneIndex) + 1;
}

plUInt32 plImageFormat::GetNumBlocksZ(Enum format, plUInt32 uiDepth, plUInt32 uiPlaneIndex)
{
  return (uiDepth - 1) / GetBlockDepth(format, uiPlaneIndex) + 1;
}

plUInt64 plImageFormat::GetRowPitch(Enum format, plUInt32 uiWidth, plUInt32 uiPlaneIndex)
{
  return static_cast<plUInt64>(GetNumBlocksX(format, uiWidth, uiPlaneIndex)) * GetBitsPerBlock(format, uiPlaneIndex) / 8;
}

plUInt64 plImageFormat::GetDepthPitch(Enum format, plUInt32 uiWidth, plUInt32 uiHeight, plUInt32 uiPlaneIndex)
{
  return static_cast<plUInt64>(GetNumBlocksY(format, uiHeight, uiPlaneIndex)) * static_cast<plUInt64>(GetRowPitch(format, uiWidth, uiPlaneIndex));
}

plImageFormatType::Enum plImageFormat::GetType(Enum format)
{
  return GetImageFormatMetaData(format).m_formatType;
}

PL_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageFormat);

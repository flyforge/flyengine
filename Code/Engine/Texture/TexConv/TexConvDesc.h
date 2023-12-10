#pragma once

#include <Texture/TexConv/TexConvEnums.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageEnums.h>

struct plTexConvChannelMapping
{
  plInt8 m_iInputImageIndex = -1;
  plTexConvChannelValue::Enum m_ChannelValue;
};

/// Describes from which input file to read which channel and then write it to the R, G, B, or A channel of the
/// output file. The four elements of the array represent the four channels of the output image.
struct plTexConvSliceChannelMapping
{
  plTexConvChannelMapping m_Channel[4] = {
    plTexConvChannelMapping{-1, plTexConvChannelValue::Red},
    plTexConvChannelMapping{-1, plTexConvChannelValue::Green},
    plTexConvChannelMapping{-1, plTexConvChannelValue::Blue},
    plTexConvChannelMapping{-1, plTexConvChannelValue::Alpha},
  };
};

class PLASMA_TEXTURE_DLL plTexConvDesc
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plTexConvDesc);

public:
  plTexConvDesc() = default;

  plHybridArray<plString, 4> m_InputFiles;
  plDynamicArray<plImage> m_InputImages;

  plHybridArray<plTexConvSliceChannelMapping, 6> m_ChannelMappings;

  // output type / platform
  plEnum<plTexConvOutputType> m_OutputType;
  plEnum<plTexConvTargetPlatform> m_TargetPlatform; // TODO: implement android

  // low resolution output
  plUInt32 m_uiLowResMipmaps = 0;

  // thumbnail output
  plUInt32 m_uiThumbnailOutputResolution = 0;

  // Format / Compression
  plEnum<plTexConvUsage> m_Usage;
  plEnum<plTexConvCompressionMode> m_CompressionMode;

  // resolution clamp and downscale
  plUInt32 m_uiMinResolution = 16;
  plUInt32 m_uiMaxResolution = 1024 * 8;
  plUInt32 m_uiDownscaleSteps = 0;

  // Mipmaps / filtering
  plEnum<plTexConvMipmapMode> m_MipmapMode;
  plEnum<plTextureFilterSetting> m_FilterMode; // only used when writing to pl specific formats
  plEnum<plImageAddressMode> m_AddressModeU;
  plEnum<plImageAddressMode> m_AddressModeV;
  plEnum<plImageAddressMode> m_AddressModeW;
  bool m_bPreserveMipmapCoverage = false;
  float m_fMipmapAlphaThreshold = 0.5f;

  // Misc options
  plUInt8 m_uiDilateColor = 0;
  bool m_bFlipHorizontal = false;
  bool m_bPremultiplyAlpha = false;
  float m_fHdrExposureBias = 0.0f;
  float m_fMaxValue = 64000.f;

  // pl specific
  plUInt64 m_uiAssetHash = 0;
  plUInt16 m_uiAssetVersion = 0;

  // Texture Atlas
  plString m_sTextureAtlasDescFile;

  // Bump map filter
  plEnum<plTexConvBumpMapFilter> m_BumpMapFilter;
};

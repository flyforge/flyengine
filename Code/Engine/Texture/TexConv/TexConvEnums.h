#pragma once

#include <Texture/TextureDLL.h>

#include <Foundation/Reflection/Reflection.h>

struct plTexConvOutputType
{
  enum Enum
  {
    None,
    Texture2D,
    Volume,
    Cubemap,
    Atlas,

    Default = Texture2D
  };

  using StorageType = plUInt8;
};

struct plTexConvCompressionMode
{
  enum Enum
  {
    // note: order of enum values matters
    None = 0,   // uncompressed
    Medium = 1, // compressed with high quality, if possible
    High = 2,   // strongest compression, if possible

    Default = Medium,
  };

  using StorageType = plUInt8;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_TEXTURE_DLL, plTexConvCompressionMode);

struct plTexConvUsage
{
  enum Enum
  {
    Auto, ///< Target format will be detected from heuristics (filename, content)

    // Exact format will be decided together with plTexConvCompressionMode

    Color,
    Linear,
    Hdr,

    NormalMap,
    NormalMap_Inverted,

    BumpMap,

    Default = Auto
  };

  using StorageType = plUInt8;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_TEXTURE_DLL, plTexConvUsage);

struct plTexConvMipmapMode
{
  enum Enum
  {
    None, ///< Mipmap generation is disabled, output will have no mipmaps
    Linear,
    Kaiser,

    Default = Kaiser
  };

  using StorageType = plUInt8;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_TEXTURE_DLL, plTexConvMipmapMode);

struct plTexConvTargetPlatform
{
  enum Enum
  {
    PC,
    Android,

    Default = PC
  };

  using StorageType = plUInt8;
};

/// \brief Defines which channel of another texture to read to get a value
struct plTexConvChannelValue
{
  enum Enum
  {
    Red,   ///< read the RED channel
    Green, ///< read the GREEN channel
    Blue,  ///< read the BLUE channel
    Alpha, ///< read the ALPHA channel

    Black, ///< don't read any channel, just take the constant value 0
    White, ///< don't read any channel, just take the constant value 0xFF / 1.0f
  };
};

/// \brief Defines which filter kernel is used to approximate the x/y bump map gradients
struct plTexConvBumpMapFilter
{
  enum Enum
  {
    Finite, ///< Simple finite differences in a 4-Neighborhood
    Sobel,  ///< Sobel kernel (8-Neighborhood)
    Scharr, ///< Scharr kernel (8-Neighborhood)

    Default = Finite
  };

  using StorageType = plUInt8;
};

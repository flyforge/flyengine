#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Types.h>
#include <Texture/TextureDLL.h>

struct PL_TEXTURE_DLL plImageAddressMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Repeat,
    Clamp,
    ClampBorder,
    Mirror,

    ENUM_COUNT,

    Default = Repeat
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_TEXTURE_DLL, plImageAddressMode);

//////////////////////////////////////////////////////////////////////////
// plTextureFilterSetting
//////////////////////////////////////////////////////////////////////////

struct PL_TEXTURE_DLL plTextureFilterSetting
{
  using StorageType = plUInt8;

  enum Enum
  {
    FixedNearest,
    FixedBilinear,
    FixedTrilinear,
    FixedAnisotropic2x,
    FixedAnisotropic4x,
    FixedAnisotropic8x,
    FixedAnisotropic16x,

    LowestQuality,
    LowQuality,
    DefaultQuality,
    HighQuality,
    HighestQuality,

    Default = DefaultQuality
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_TEXTURE_DLL, plTextureFilterSetting);

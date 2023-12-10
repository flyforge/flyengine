#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageEnums.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plImageAddressMode, 1)
  PLASMA_ENUM_CONSTANT(plImageAddressMode::Repeat),
  PLASMA_ENUM_CONSTANT(plImageAddressMode::Clamp),
  PLASMA_ENUM_CONSTANT(plImageAddressMode::ClampBorder),
  PLASMA_ENUM_CONSTANT(plImageAddressMode::Mirror),
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plTextureFilterSetting, 1)
  PLASMA_ENUM_CONSTANT(plTextureFilterSetting::FixedNearest),
  PLASMA_ENUM_CONSTANT(plTextureFilterSetting::FixedBilinear),
  PLASMA_ENUM_CONSTANT(plTextureFilterSetting::FixedTrilinear),
  PLASMA_ENUM_CONSTANT(plTextureFilterSetting::FixedAnisotropic2x),
  PLASMA_ENUM_CONSTANT(plTextureFilterSetting::FixedAnisotropic4x),
  PLASMA_ENUM_CONSTANT(plTextureFilterSetting::FixedAnisotropic8x),
  PLASMA_ENUM_CONSTANT(plTextureFilterSetting::FixedAnisotropic16x),
  PLASMA_ENUM_CONSTANT(plTextureFilterSetting::LowestQuality),
  PLASMA_ENUM_CONSTANT(plTextureFilterSetting::LowQuality),
  PLASMA_ENUM_CONSTANT(plTextureFilterSetting::DefaultQuality),
  PLASMA_ENUM_CONSTANT(plTextureFilterSetting::HighQuality),
  PLASMA_ENUM_CONSTANT(plTextureFilterSetting::HighestQuality),
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on


PLASMA_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageEnums);

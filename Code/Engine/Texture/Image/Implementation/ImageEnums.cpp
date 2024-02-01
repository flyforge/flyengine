#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageEnums.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plImageAddressMode, 1)
  PL_ENUM_CONSTANT(plImageAddressMode::Repeat),
  PL_ENUM_CONSTANT(plImageAddressMode::Clamp),
  PL_ENUM_CONSTANT(plImageAddressMode::ClampBorder),
  PL_ENUM_CONSTANT(plImageAddressMode::Mirror),
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plTextureFilterSetting, 1)
  PL_ENUM_CONSTANT(plTextureFilterSetting::FixedNearest),
  PL_ENUM_CONSTANT(plTextureFilterSetting::FixedBilinear),
  PL_ENUM_CONSTANT(plTextureFilterSetting::FixedTrilinear),
  PL_ENUM_CONSTANT(plTextureFilterSetting::FixedAnisotropic2x),
  PL_ENUM_CONSTANT(plTextureFilterSetting::FixedAnisotropic4x),
  PL_ENUM_CONSTANT(plTextureFilterSetting::FixedAnisotropic8x),
  PL_ENUM_CONSTANT(plTextureFilterSetting::FixedAnisotropic16x),
  PL_ENUM_CONSTANT(plTextureFilterSetting::LowestQuality),
  PL_ENUM_CONSTANT(plTextureFilterSetting::LowQuality),
  PL_ENUM_CONSTANT(plTextureFilterSetting::DefaultQuality),
  PL_ENUM_CONSTANT(plTextureFilterSetting::HighQuality),
  PL_ENUM_CONSTANT(plTextureFilterSetting::HighestQuality),
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on


PL_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageEnums);

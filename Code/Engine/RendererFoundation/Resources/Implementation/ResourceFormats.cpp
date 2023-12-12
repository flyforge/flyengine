#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ResourceFormats.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plGALResourceFormat, 1)
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAFloat),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAUInt),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAInt),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBFloat),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBUInt),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBInt),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::B5G6R5UNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BGRAUByteNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BGRAUByteNormalizedsRGB),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAHalf),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAUShort),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAUShortNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAShort),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAShortNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGFloat),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGUInt),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGInt),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGB10A2UInt),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGB10A2UIntNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RG11B10Float),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAUByteNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAUByteNormalizedsRGB),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAUByte),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAByteNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGBAByte),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGHalf),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGUShort),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGUShortNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGShort),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGShortNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGUByte),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGUByteNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGByte),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RGByteNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::DFloat),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RFloat),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RUInt),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RInt),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RHalf),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RUShort),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RUShortNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RShort),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RShortNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RUByte),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RUByteNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RByte),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::RByteNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::AUByteNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::D16),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::D24S8),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC1),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC1sRGB),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC2),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC2sRGB),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC3),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC3sRGB),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC4UNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC4Normalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC5UNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC5Normalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC6UFloat),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC6Float),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC7UNormalized),
  PLASMA_ENUM_CONSTANT(plGALResourceFormat::BC7UNormalizedsRGB)
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
const plUInt8 plGALResourceFormat::s_BitsPerElement[plGALResourceFormat::ENUM_COUNT] =
{
  0, // Invalid

  128, // RGBAFloat, XYZWFloat
  128, // RGBAUInt
  128, // RGBAInt

  96, // RGBFloat, XYZFloat, UVWFloat
  96, // RGBUInt
  96, // RGBInt

  16, // B5G6R5UNormalized

  32, // BGRAUByteNormalized
  32, // BGRAUByteNormalizedsRGB

  64, // RGBAHalf, XYZWHalf
  64, // RGBAUShort
  64, // RGBAUShortNormalized
  64, // RGBAShort
  64, // RGBAShortNormalized

  64, // RGFloat, XYFloat, UVFloat
  64, // RGUInt
  64, // RGInt

  32, // RGB10A2UInt
  32, // RGB10A2UIntNormalized
  32, // RG11B10Float

  32, // RGBAUByteNormalized
  32, // RGBAUByteNormalizedsRGB
  32, // RGBAUByte
  32, // RGBAByteNormalized
  32, // RGBAByte

  32, // RGHalf, XYHalf, UVHalf
  32, // RGUShort
  32, // RGUShortNormalized
  32, // RGShort
  32, // RGShortNormalized

  16, // RGUByte
  16, // RGUByteNormalized
  16, // RGByte
  16, // RGByteNormalized

  32, // DFloat
  32, // RFloat
  32, // RUInt
  32, // RInt

  16, // RHalf
  16, // RUShort
  16, // RUShortNormalized
  16, // RShort
  16, // RShortNormalized

  8, // RUByte
  8, // RUByteNormalized
  8, // RByte
  8, // RByteNormalized
  8, // AUByteNormalized

  16, // D16
  32, // D24S8

  // For compressed formats see: http://msdn.microsoft.com/en-us/library/windows/desktop/hh308955%28v=vs.85%29.aspx

  4, // BC1
  4, // BC1sRGB

  8, // BC2
  8, // BC2sRGB
  8, // BC3
  8, // BC3sRGB

  4, // BC4UNormalized
  4, // BC4Normalized

  8, // BC5UNormalized
  8, // BC5Normalized
  8, // BC6UFloat
  8, // BC6Float
  8, // BC7UNormalized
  8  // BC7UNormalizedsRGB
};

const plUInt8 plGALResourceFormat::s_ChannelCount[plGALResourceFormat::ENUM_COUNT] =
{
  0, // Invalid

  4, // RGBAFloat, XYZWFloat
  4, // RGBAUInt
  4, // RGBAInt

  3, // RGBFloat, XYZFloat, UVWFloat
  3, // RGBUInt
  3, // RGBInt

  3, // B5G6R5UNormalized

  4, // BGRAUByteNormalized
  4, // BGRAUByteNormalizedsRGB

  4, // RGBAHalf, XYZWHalf
  4, // RGBAUShort
  4, // RGBAUShortNormalized
  4, // RGBAShort
  4, // RGBAShortNormalized

  2, // RGFloat, XYFloat, UVFloat
  2, // RGUInt
  2, // RGInt

  4, // RGB10A2UInt
  4, // RGB10A2UIntNormalized
  3, // RG11B10Float

  4, // RGBAUByteNormalized
  4, // RGBAUByteNormalizedsRGB
  4, // RGBAUByte
  4, // RGBAByteNormalized
  4, // RGBAByte

  2, // RGHalf, XYHalf, UVHalf
  2, // RGUShort
  2, // RGUShortNormalized
  2, // RGShort
  2, // RGShortNormalized

  2, // RGUByte
  2, // RGUByteNormalized
  2, // RGByte
  2, // RGByteNormalized

  1, // DFloat
  1, // RFloat
  1, // RUInt
  1, // RInt

  1, // RHalf
  1, // RUShort
  1, // RUShortNormalized
  1, // RShort
  1, // RShortNormalized

  1, // RUByte
  1, // RUByteNormalized
  1, // RByte
  1, // RByteNormalized
  1, // AUByteNormalized

  1, // D16
  2, // D24S8

  // For compressed formats see: http://msdn.microsoft.com/en-us/library/windows/desktop/hh308955%28v=vs.85%29.aspx

  4, // BC1
  4, // BC1sRGB
  4, // BC2
  4, // BC2sRGB
  4, // BC3
  4, // BC3sRGB
  1, // BC4UNormalized
  1, // BC4Normalized
  2, // BC5UNormalized
  2, // BC5Normalized
  3, // BC6UFloat
  3, // BC6Float
  4, // BC7UNormalized
  4  // BC7UNormalizedsRGB
};
// clang-format off


PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_ResourceFormats);


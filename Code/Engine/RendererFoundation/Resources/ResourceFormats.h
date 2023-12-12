
#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct PLASMA_RENDERERFOUNDATION_DLL plGALResourceFormat
{
  using StorageType = plUInt8;

  enum Enum : plUInt8
  {
    Invalid = 0,

    RGBAFloat,
    XYZWFloat = RGBAFloat,
    RGBAUInt,
    RGBAInt,

    RGBFloat,
    XYZFloat = RGBFloat,
    UVWFloat = RGBFloat,
    RGBUInt,
    RGBInt,

    B5G6R5UNormalized,
    BGRAUByteNormalized,
    BGRAUByteNormalizedsRGB,

    RGBAHalf,
    XYZWHalf = RGBAHalf,
    RGBAUShort,
    RGBAUShortNormalized,
    RGBAShort,
    RGBAShortNormalized,

    RGFloat,
    XYFloat = RGFloat,
    UVFloat = RGFloat,
    RGUInt,
    RGInt,

    RGB10A2UInt,
    RGB10A2UIntNormalized,
    RG11B10Float,

    RGBAUByteNormalized,
    RGBAUByteNormalizedsRGB,
    RGBAUByte,
    RGBAByteNormalized,
    RGBAByte,

    RGHalf,
    XYHalf = RGHalf,
    UVHalf = RGHalf,
    RGUShort,
    RGUShortNormalized,
    RGShort,
    RGShortNormalized,
    RGUByte,
    RGUByteNormalized,
    RGByte,
    RGByteNormalized,

    DFloat,

    RFloat,
    RUInt,
    RInt,
    RHalf,
    RUShort,
    RUShortNormalized,
    RShort,
    RShortNormalized,
    RUByte,
    RUByteNormalized,
    RByte,
    RByteNormalized,

    AUByteNormalized,

    D16,
    D24S8,

    BC1,
    BC1sRGB,
    BC2,
    BC2sRGB,
    BC3,
    BC3sRGB,
    BC4UNormalized,
    BC4Normalized,
    BC5UNormalized,
    BC5Normalized,
    BC6UFloat,
    BC6Float,
    BC7UNormalized,
    BC7UNormalizedsRGB,

    ENUM_COUNT,

    Default = RGBAUByteNormalizedsRGB
  };


  // General format Meta-Informations:

  /// \brief The size in bits per element (usually pixels, except for mesh stream elements) of a single element of the given resource format.
  static plUInt32 GetBitsPerElement(plGALResourceFormat::Enum format);

  /// \brief The number of color channels this format contains.
  static plUInt8 GetChannelCount(plGALResourceFormat::Enum format);

  /// \todo A combination of propertyflags, something like srgb, normalized, ...
  // Would be very useful for some GL stuff and Testing.

  /// \brief Returns whether the given resource format is a depth format
  static bool IsDepthFormat(plGALResourceFormat::Enum format);
  static bool IsStencilFormat(plGALResourceFormat::Enum format);

  static bool IsSrgb(plGALResourceFormat::Enum format);

private:
  static const plUInt8 s_BitsPerElement[plGALResourceFormat::ENUM_COUNT];

  static const plUInt8 s_ChannelCount[plGALResourceFormat::ENUM_COUNT];
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERFOUNDATION_DLL, plGALResourceFormat);


template <typename NativeFormatType, NativeFormatType InvalidFormat>
class plGALFormatLookupEntry
{
public:
  inline plGALFormatLookupEntry();

  inline plGALFormatLookupEntry(NativeFormatType storage);

  inline plGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RT(NativeFormatType renderTargetType);

  inline plGALFormatLookupEntry<NativeFormatType, InvalidFormat>& D(NativeFormatType depthOnlyType);

  inline plGALFormatLookupEntry<NativeFormatType, InvalidFormat>& S(NativeFormatType stencilOnlyType);

  inline plGALFormatLookupEntry<NativeFormatType, InvalidFormat>& DS(NativeFormatType depthStencilType);

  inline plGALFormatLookupEntry<NativeFormatType, InvalidFormat>& VA(NativeFormatType vertexAttributeType);

  inline plGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RV(NativeFormatType resourceViewType);

  NativeFormatType m_eStorage;
  NativeFormatType m_eRenderTarget;
  NativeFormatType m_eDepthOnlyType;
  NativeFormatType m_eStencilOnlyType;
  NativeFormatType m_eDepthStencilType;
  NativeFormatType m_eVertexAttributeType;
  NativeFormatType m_eResourceViewType;
};

// Reusable table class to store lookup information (from plGALResourceFormat to the various formats for texture/buffer storage, views)
template <typename FormatClass>
class plGALFormatLookupTable
{
public:
  plGALFormatLookupTable();

  PLASMA_ALWAYS_INLINE const FormatClass& GetFormatInfo(plGALResourceFormat::Enum format) const;

  PLASMA_ALWAYS_INLINE void SetFormatInfo(plGALResourceFormat::Enum format, const FormatClass& newFormatInfo);

private:
  FormatClass m_Formats[plGALResourceFormat::ENUM_COUNT];
};

#include <RendererFoundation/Resources/Implementation/ResourceFormats_inl.h>


// static
PLASMA_ALWAYS_INLINE plUInt32 plGALResourceFormat::GetBitsPerElement(plGALResourceFormat::Enum format)
{
  return s_BitsPerElement[format];
}

// static
PLASMA_ALWAYS_INLINE plUInt8 plGALResourceFormat::GetChannelCount(plGALResourceFormat::Enum format)
{
  return s_ChannelCount[format];
}

// static
PLASMA_FORCE_INLINE bool plGALResourceFormat::IsDepthFormat(plGALResourceFormat::Enum format)
{
  return format == DFloat || format == D16 || format == D24S8;
}

// static
PLASMA_FORCE_INLINE bool plGALResourceFormat::IsStencilFormat(Enum format)
{
  return format == D24S8;
}

// static
PLASMA_FORCE_INLINE bool plGALResourceFormat::IsSrgb(plGALResourceFormat::Enum format)
{
  return format == BGRAUByteNormalizedsRGB || format == RGBAUByteNormalizedsRGB || format == BC1sRGB || format == BC2sRGB || format == BC3sRGB ||
         format == BC7UNormalizedsRGB;
}


template <typename NativeFormatType, NativeFormatType InvalidFormat>
plGALFormatLookupEntry<NativeFormatType, InvalidFormat>::plGALFormatLookupEntry()
  : m_eStorage(InvalidFormat)
  , m_eRenderTarget(InvalidFormat)
  , m_eDepthOnlyType(InvalidFormat)
  , m_eStencilOnlyType(InvalidFormat)
  , m_eDepthStencilType(InvalidFormat)
  , m_eVertexAttributeType(InvalidFormat)
  , m_eResourceViewType(InvalidFormat)
{
}


template <typename NativeFormatType, NativeFormatType InvalidFormat>
plGALFormatLookupEntry<NativeFormatType, InvalidFormat>::plGALFormatLookupEntry(NativeFormatType storage)
  : m_eStorage(storage)
  , m_eRenderTarget(InvalidFormat)
  , m_eDepthOnlyType(InvalidFormat)
  , m_eStencilOnlyType(InvalidFormat)
  , m_eDepthStencilType(InvalidFormat)
  , m_eVertexAttributeType(InvalidFormat)
  , m_eResourceViewType(InvalidFormat)
{
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
plGALFormatLookupEntry<NativeFormatType, InvalidFormat>& plGALFormatLookupEntry<NativeFormatType, InvalidFormat>::RT(
  NativeFormatType renderTargetType)
{
  m_eRenderTarget = renderTargetType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
plGALFormatLookupEntry<NativeFormatType, InvalidFormat>& plGALFormatLookupEntry<NativeFormatType, InvalidFormat>::D(NativeFormatType depthOnlyType)
{
  m_eDepthOnlyType = depthOnlyType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
plGALFormatLookupEntry<NativeFormatType, InvalidFormat>& plGALFormatLookupEntry<NativeFormatType, InvalidFormat>::S(NativeFormatType stencilOnlyType)
{
  m_eStencilOnlyType = stencilOnlyType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
plGALFormatLookupEntry<NativeFormatType, InvalidFormat>& plGALFormatLookupEntry<NativeFormatType, InvalidFormat>::DS(
  NativeFormatType depthStencilType)
{
  m_eDepthStencilType = depthStencilType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
plGALFormatLookupEntry<NativeFormatType, InvalidFormat>& plGALFormatLookupEntry<NativeFormatType, InvalidFormat>::VA(
  NativeFormatType vertexAttributeType)
{
  m_eVertexAttributeType = vertexAttributeType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
plGALFormatLookupEntry<NativeFormatType, InvalidFormat>& plGALFormatLookupEntry<NativeFormatType, InvalidFormat>::RV(
  NativeFormatType resourceViewType)
{
  m_eResourceViewType = resourceViewType;
  return *this;
}


template <typename FormatClass>
plGALFormatLookupTable<FormatClass>::plGALFormatLookupTable()
{
  for (plUInt32 i = 0; i < plGALResourceFormat::ENUM_COUNT; i++)
  {
    m_Formats[i] = FormatClass();
  }
}

template <typename FormatClass>
const FormatClass& plGALFormatLookupTable<FormatClass>::GetFormatInfo(plGALResourceFormat::Enum format) const
{
  return m_Formats[format];
}

template <typename FormatClass>
void plGALFormatLookupTable<FormatClass>::SetFormatInfo(plGALResourceFormat::Enum format, const FormatClass& newFormatInfo)
{
  m_Formats[format] = newFormatInfo;
}

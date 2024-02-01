#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>

// Ensure that we can retrieve the base data type with this simple bit operation
static_assert(((int)plProcessingStream::DataType::Half3 & ~3) == (int)plProcessingStream::DataType::Half);
static_assert(((int)plProcessingStream::DataType::Float4 & ~3) == (int)plProcessingStream::DataType::Float);
static_assert(((int)plProcessingStream::DataType::Byte2 & ~3) == (int)plProcessingStream::DataType::Byte);
static_assert(((int)plProcessingStream::DataType::Short3 & ~3) == (int)plProcessingStream::DataType::Short);
static_assert(((int)plProcessingStream::DataType::Int4 & ~3) == (int)plProcessingStream::DataType::Int);

#if PL_ENABLED(PL_PLATFORM_64BIT)
static_assert(sizeof(plProcessingStream) == 32);
#endif

plProcessingStream::plProcessingStream() = default;

plProcessingStream::plProcessingStream(const plHashedString& sName, DataType type, plUInt16 uiStride, plUInt16 uiAlignment)
  : m_uiAlignment(uiAlignment)
  , m_uiTypeSize(GetDataTypeSize(type))
  , m_uiStride(uiStride)
  , m_Type(type)
  , m_sName(sName)
{
}

plProcessingStream::plProcessingStream(const plHashedString& sName, plArrayPtr<plUInt8> data, DataType type, plUInt16 uiStride)
  : m_pData(data.GetPtr())
  , m_uiDataSize(data.GetCount())
  , m_uiTypeSize(GetDataTypeSize(type))
  , m_uiStride(uiStride)
  , m_Type(type)
  , m_bExternalMemory(true)
  , m_sName(sName)
{
}

plProcessingStream::plProcessingStream(const plHashedString& sName, plArrayPtr<plUInt8> data, DataType type)
  : m_pData(data.GetPtr())
  , m_uiDataSize(data.GetCount())
  , m_uiTypeSize(GetDataTypeSize(type))
  , m_uiStride(m_uiTypeSize)
  , m_Type(type)
  , m_bExternalMemory(true)
  , m_sName(sName)
{
}

plProcessingStream::~plProcessingStream()
{
  FreeData();
}

void plProcessingStream::SetSize(plUInt64 uiNumElements)
{
  plUInt64 uiNewDataSize = uiNumElements * m_uiTypeSize;
  if (m_uiDataSize == uiNewDataSize)
    return;

  FreeData();

  if (uiNewDataSize == 0)
  {
    return;
  }

  /// \todo Allow to reuse memory from a pool ?
  if (m_uiAlignment > 0)
  {
    m_pData = plFoundation::GetAlignedAllocator()->Allocate(static_cast<size_t>(uiNewDataSize), static_cast<size_t>(m_uiAlignment));
  }
  else
  {
    m_pData = plFoundation::GetDefaultAllocator()->Allocate(static_cast<size_t>(uiNewDataSize), 0);
  }

  PL_ASSERT_DEV(m_pData != nullptr, "Allocating {0} elements of {1} bytes each, with {2} bytes alignment, failed", uiNumElements, ((plUInt32)GetDataTypeSize(m_Type)), m_uiAlignment);
  m_uiDataSize = uiNewDataSize;
}

void plProcessingStream::FreeData()
{
  if (m_pData != nullptr && m_bExternalMemory == false)
  {
    if (m_uiAlignment > 0)
    {
      plFoundation::GetAlignedAllocator()->Deallocate(m_pData);
    }
    else
    {
      plFoundation::GetDefaultAllocator()->Deallocate(m_pData);
    }
  }

  m_pData = nullptr;
  m_uiDataSize = 0;
}

static plUInt16 s_TypeSize[] = {
  2, // Half,
  4, // Half2,
  6, // Half3,
  8, // Half4,

  4,  // Float,
  8,  // Float2,
  12, // Float3,
  16, // Float4,

  1, // Byte,
  2, // Byte2,
  3, // Byte3,
  4, // Byte4,

  2, // Short,
  4, // Short2,
  6, // Short3,
  8, // Short4,

  4,  // Int,
  8,  // Int2,
  12, // Int3,
  16, // Int4,
};
static_assert(PL_ARRAY_SIZE(s_TypeSize) == (size_t)plProcessingStream::DataType::Count);

// static
plUInt16 plProcessingStream::GetDataTypeSize(DataType type)
{
  return s_TypeSize[(plUInt32)type];
}

static plStringView s_TypeName[] = {
  "Half"_plsv,  // Half,
  "Half2"_plsv, // Half2,
  "Half3"_plsv, // Half3,
  "Half4"_plsv, // Half4,

  "Float"_plsv,  // Float,
  "Float2"_plsv, // Float2,
  "Float3"_plsv, // Float3,
  "Float4"_plsv, // Float4,

  "Byte"_plsv,  // Byte,
  "Byte2"_plsv, // Byte2,
  "Byte3"_plsv, // Byte3,
  "Byte4"_plsv, // Byte4,

  "Short"_plsv,  // Short,
  "Short2"_plsv, // Short2,
  "Short3"_plsv, // Short3,
  "Short4"_plsv, // Short4,

  "Int"_plsv,  // Int,
  "Int2"_plsv, // Int2,
  "Int3"_plsv, // Int3,
  "Int4"_plsv, // Int4,
};
static_assert(PL_ARRAY_SIZE(s_TypeName) == (size_t)plProcessingStream::DataType::Count);

// static
plStringView plProcessingStream::GetDataTypeName(DataType type)
{
  return s_TypeName[(plUInt32)type];
}



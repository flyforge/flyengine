
#include <Foundation/IO/Stream.h>

template <typename T>
PLASMA_ALWAYS_INLINE plResult plDeduplicationReadContext::ReadObjectInplace(plStreamReader& inout_stream, T& inout_obj)
{
  return ReadObject(inout_stream, inout_obj, nullptr);
}

template <typename T>
plResult plDeduplicationReadContext::ReadObject(plStreamReader& inout_stream, T& obj, plAllocatorBase* pAllocator)
{
  bool bIsRealObject;
  inout_stream >> bIsRealObject;

  PLASMA_ASSERT_DEV(bIsRealObject, "Reading an object inplace only works for the first occurrence");

  PLASMA_SUCCEED_OR_RETURN(plStreamReaderUtil::Deserialize<T>(inout_stream, obj));

  m_Objects.PushBack(&obj);

  return PLASMA_SUCCESS;
}

template <typename T>
plResult plDeduplicationReadContext::ReadObject(plStreamReader& inout_stream, T*& ref_pObject, plAllocatorBase* pAllocator)
{
  bool bIsRealObject;
  inout_stream >> bIsRealObject;

  if (bIsRealObject)
  {
    ref_pObject = PLASMA_NEW(pAllocator, T);
    PLASMA_SUCCEED_OR_RETURN(plStreamReaderUtil::Deserialize<T>(inout_stream, *ref_pObject));

    m_Objects.PushBack(ref_pObject);
  }
  else
  {
    plUInt32 uiIndex;
    inout_stream >> uiIndex;

    if (uiIndex < m_Objects.GetCount())
    {
      ref_pObject = static_cast<T*>(m_Objects[uiIndex]);
    }
    else if (uiIndex == plInvalidIndex)
    {
      ref_pObject = nullptr;
    }
    else
    {
      return PLASMA_FAILURE;
    }
  }

  return PLASMA_SUCCESS;
}

template <typename T>
plResult plDeduplicationReadContext::ReadObject(plStreamReader& inout_stream, plSharedPtr<T>& ref_pObject, plAllocatorBase* pAllocator)
{
  T* ptr = nullptr;
  if (ReadObject(inout_stream, ptr, pAllocator).Succeeded())
  {
    ref_pObject = plSharedPtr<T>(ptr, pAllocator);
    return PLASMA_SUCCESS;
  }
  return PLASMA_FAILURE;
}

template <typename T>
plResult plDeduplicationReadContext::ReadObject(plStreamReader& inout_stream, plUniquePtr<T>& ref_pObject, plAllocatorBase* pAllocator)
{
  T* ptr = nullptr;
  if (ReadObject(inout_stream, ptr, pAllocator).Succeeded())
  {
    ref_pObject = std::move(plUniquePtr<T>(ptr, pAllocator));
    return PLASMA_SUCCESS;
  }
  return PLASMA_FAILURE;
}

template <typename ArrayType, typename ValueType>
plResult plDeduplicationReadContext::ReadArray(plStreamReader& inout_stream, plArrayBase<ValueType, ArrayType>& ref_array, plAllocatorBase* pAllocator)
{
  plUInt64 uiCount = 0;
  PLASMA_SUCCEED_OR_RETURN(inout_stream.ReadQWordValue(&uiCount));

  PLASMA_ASSERT_DEV(uiCount < std::numeric_limits<plUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  ref_array.Clear();

  if (uiCount > 0)
  {
    static_cast<ArrayType&>(ref_array).Reserve(static_cast<plUInt32>(uiCount));

    for (plUInt32 i = 0; i < static_cast<plUInt32>(uiCount); ++i)
    {
      PLASMA_SUCCEED_OR_RETURN(ReadObject(inout_stream, ref_array.ExpandAndGetRef(), pAllocator));
    }
  }

  return PLASMA_SUCCESS;
}

template <typename KeyType, typename Comparer>
plResult plDeduplicationReadContext::ReadSet(plStreamReader& inout_stream, plSetBase<KeyType, Comparer>& ref_set, plAllocatorBase* pAllocator)
{
  plUInt64 uiCount = 0;
  PLASMA_SUCCEED_OR_RETURN(inout_stream.ReadQWordValue(&uiCount));

  PLASMA_ASSERT_DEV(uiCount < std::numeric_limits<plUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  ref_set.Clear();

  for (plUInt32 i = 0; i < static_cast<plUInt32>(uiCount); ++i)
  {
    KeyType key;
    PLASMA_SUCCEED_OR_RETURN(ReadObject(inout_stream, key, pAllocator));

    ref_set.Insert(std::move(key));
  }

  return PLASMA_SUCCESS;
}

namespace plInternal
{
  // Internal helper to prevent the compiler from trying to find a de-serialization method for pointer types or other types which don't have
  // one.
  struct DeserializeHelper
  {
    template <typename T>
    static auto Deserialize(plStreamReader& inout_stream, T& ref_obj, int) -> decltype(plStreamReaderUtil::Deserialize(inout_stream, ref_obj))
    {
      return plStreamReaderUtil::Deserialize(inout_stream, ref_obj);
    }

    template <typename T>
    static plResult Deserialize(plStreamReader& inout_stream, T& ref_obj, float)
    {
      PLASMA_REPORT_FAILURE("No deserialize method available");
      return PLASMA_FAILURE;
    }
  };
} // namespace plInternal

template <typename KeyType, typename ValueType, typename Comparer>
plResult plDeduplicationReadContext::ReadMap(plStreamReader& inout_stream, plMapBase<KeyType, ValueType, Comparer>& ref_map, ReadMapMode mode, plAllocatorBase* pKeyAllocator, plAllocatorBase* pValueAllocator)
{
  plUInt64 uiCount = 0;
  PLASMA_SUCCEED_OR_RETURN(inout_stream.ReadQWordValue(&uiCount));

  PLASMA_ASSERT_DEV(uiCount < std::numeric_limits<plUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  ref_map.Clear();

  if (mode == ReadMapMode::DedupKey)
  {
    for (plUInt32 i = 0; i < static_cast<plUInt32>(uiCount); ++i)
    {
      KeyType key;
      ValueType value;
      PLASMA_SUCCEED_OR_RETURN(ReadObject(inout_stream, key, pKeyAllocator));
      PLASMA_SUCCEED_OR_RETURN(plInternal::DeserializeHelper::Deserialize<ValueType>(inout_stream, value, 0));

      ref_map.Insert(std::move(key), std::move(value));
    }
  }
  else if (mode == ReadMapMode::DedupValue)
  {
    for (plUInt32 i = 0; i < static_cast<plUInt32>(uiCount); ++i)
    {
      KeyType key;
      ValueType value;
      PLASMA_SUCCEED_OR_RETURN(plInternal::DeserializeHelper::Deserialize<KeyType>(inout_stream, key, 0));
      PLASMA_SUCCEED_OR_RETURN(ReadObject(inout_stream, value, pValueAllocator));

      ref_map.Insert(std::move(key), std::move(value));
    }
  }
  else
  {
    for (plUInt32 i = 0; i < static_cast<plUInt32>(uiCount); ++i)
    {
      KeyType key;
      ValueType value;
      PLASMA_SUCCEED_OR_RETURN(ReadObject(inout_stream, key, pKeyAllocator));
      PLASMA_SUCCEED_OR_RETURN(ReadObject(inout_stream, value, pValueAllocator));

      ref_map.Insert(std::move(key), std::move(value));
    }
  }

  return PLASMA_SUCCESS;
}

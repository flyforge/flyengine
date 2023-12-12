#pragma once

#if PLASMA_ENABLED(PLASMA_PLATFORM_BIG_ENDIAN)

template <typename T>
plResult plStreamReader::ReadWordValue(T* pWordValue)
{
  PLASMA_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(plUInt16));

  plUInt16 uiTemp;

  const plUInt32 uiRead = ReadBytes(reinterpret_cast<plUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<plUInt16*>(pWordValue) = plEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? PLASMA_SUCCESS : PLASMA_FAILURE;
}

template <typename T>
plResult plStreamReader::ReadDWordValue(T* pDWordValue)
{
  PLASMA_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(plUInt32));

  plUInt32 uiTemp;

  const plUInt32 uiRead = ReadBytes(reinterpret_cast<plUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<plUInt32*>(pDWordValue) = plEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? PLASMA_SUCCESS : PLASMA_FAILURE;
}

template <typename T>
plResult plStreamReader::ReadQWordValue(T* pQWordValue)
{
  PLASMA_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(plUInt64));

  plUInt64 uiTemp;

  const plUInt32 uiRead = ReadBytes(reinterpret_cast<plUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<plUInt64*>(pQWordValue) = plEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? PLASMA_SUCCESS : PLASMA_FAILURE;
}



template <typename T>
plResult plStreamWriter::WriteWordValue(const T* pWordValue)
{
  PLASMA_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(plUInt16));

  plUInt16 uiTemp = *reinterpret_cast<const plUInt16*>(pWordValue);
  uiTemp = plEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<plUInt8*>(&uiTemp), sizeof(T));
}

template <typename T>
plResult plStreamWriter::WriteDWordValue(const T* pDWordValue)
{
  PLASMA_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(plUInt32));

  plUInt32 uiTemp = *reinterpret_cast<const plUInt16*>(pDWordValue);
  uiTemp = plEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<plUInt8*>(&uiTemp), sizeof(T));
}

template <typename T>
plResult plStreamWriter::WriteQWordValue(const T* pQWordValue)
{
  PLASMA_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(plUInt64));

  plUInt64 uiTemp = *reinterpret_cast<const plUInt64*>(pQWordValue);
  uiTemp = plEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<plUInt8*>(&uiTemp), sizeof(T));
}

#else

template <typename T>
plResult plStreamReader::ReadWordValue(T* pWordValue)
{
  PLASMA_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(plUInt16));

  if (ReadBytes(reinterpret_cast<plUInt8*>(pWordValue), sizeof(T)) != sizeof(T))
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}

template <typename T>
plResult plStreamReader::ReadDWordValue(T* pDWordValue)
{
  PLASMA_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(plUInt32));

  if (ReadBytes(reinterpret_cast<plUInt8*>(pDWordValue), sizeof(T)) != sizeof(T))
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}

template <typename T>
plResult plStreamReader::ReadQWordValue(T* pQWordValue)
{
  PLASMA_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(plUInt64));

  if (ReadBytes(reinterpret_cast<plUInt8*>(pQWordValue), sizeof(T)) != sizeof(T))
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}

template <typename T>
plResult plStreamWriter::WriteWordValue(const T* pWordValue)
{
  PLASMA_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(plUInt16));

  return WriteBytes(reinterpret_cast<const plUInt8*>(pWordValue), sizeof(T));
}

template <typename T>
plResult plStreamWriter::WriteDWordValue(const T* pDWordValue)
{
  PLASMA_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(plUInt32));

  return WriteBytes(reinterpret_cast<const plUInt8*>(pDWordValue), sizeof(T));
}

template <typename T>
plResult plStreamWriter::WriteQWordValue(const T* pQWordValue)
{
  PLASMA_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(plUInt64));

  return WriteBytes(reinterpret_cast<const plUInt8*>(pQWordValue), sizeof(T));
}

#endif

plTypeVersion plStreamReader::ReadVersion(plTypeVersion expectedMaxVersion)
{
  plTypeVersion v = 0;
  ReadWordValue(&v).IgnoreResult();

  PLASMA_ASSERT_ALWAYS(v <= expectedMaxVersion, "Read version ({0}) is larger than expected max version ({1}).", v, expectedMaxVersion);
  PLASMA_ASSERT_ALWAYS(v > 0, "Invalid version.");

  return v;
}

void plStreamWriter::WriteVersion(plTypeVersion version)
{
  PLASMA_ASSERT_ALWAYS(version > 0, "Version cannot be zero.");

  WriteWordValue(&version).IgnoreResult();
}


namespace plStreamWriterUtil
{
  // single element serialization

  template <class T>
  PLASMA_ALWAYS_INLINE auto SerializeImpl(plStreamWriter& inout_stream, const T& obj, int) -> decltype(inout_stream << obj, plResult(PLASMA_SUCCESS))
  {
    inout_stream << obj;

    return PLASMA_SUCCESS;
  }

  template <class T>
  PLASMA_ALWAYS_INLINE auto SerializeImpl(plStreamWriter& inout_stream, const T& obj, long) -> decltype(obj.Serialize(inout_stream).IgnoreResult(), plResult(PLASMA_SUCCESS))
  {
    return plToResult(obj.Serialize(inout_stream));
  }

  template <class T>
  PLASMA_ALWAYS_INLINE auto SerializeImpl(plStreamWriter& inout_stream, const T& obj, float) -> decltype(obj.serialize(inout_stream).IgnoreResult(), plResult(PLASMA_SUCCESS))
  {
    return plToResult(obj.serialize(inout_stream));
  }

  template <class T>
  PLASMA_ALWAYS_INLINE auto Serialize(plStreamWriter& inout_stream, const T& obj) -> decltype(SerializeImpl(inout_stream, obj, 0).IgnoreResult(), plResult(PLASMA_SUCCESS))
  {
    return SerializeImpl(inout_stream, obj, 0);
  }

  // serialization of array

#if PLASMA_DISABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  template <class T>
  PLASMA_ALWAYS_INLINE auto SerializeArrayImpl(plStreamWriter& inout_stream, const T* pArray, plUInt64 uiCount, int) -> decltype(SerializeArray(inout_stream, pArray, uiCount), plResult(PLASMA_SUCCESS))
  {
    return SerializeArray(inout_stream, pArray, uiCount);
  }
#endif

  template <class T>
  plResult SerializeArrayImpl(plStreamWriter& inout_stream, const T* pArray, plUInt64 uiCount, long)
  {
    for (plUInt64 i = 0; i < uiCount; ++i)
    {
      PLASMA_SUCCEED_OR_RETURN(plStreamWriterUtil::Serialize<T>(inout_stream, pArray[i]));
    }

    return PLASMA_SUCCESS;
  }

  template <class T>
  PLASMA_ALWAYS_INLINE plResult SerializeArray(plStreamWriter& inout_stream, const T* pArray, plUInt64 uiCount)
  {
    return SerializeArrayImpl(inout_stream, pArray, uiCount, 0);
  }
} // namespace plStreamWriterUtil

template <typename ArrayType, typename ValueType>
plResult plStreamWriter::WriteArray(const plArrayBase<ValueType, ArrayType>& array)
{
  const plUInt64 uiCount = array.GetCount();
  PLASMA_SUCCEED_OR_RETURN(WriteQWordValue(&uiCount));

  return plStreamWriterUtil::SerializeArray<ValueType>(*this, array.GetData(), array.GetCount());
}

template <typename ValueType, plUInt16 uiSize>
plResult plStreamWriter::WriteArray(const plSmallArrayBase<ValueType, uiSize>& array)
{
  const plUInt32 uiCount = array.GetCount();
  PLASMA_SUCCEED_OR_RETURN(WriteDWordValue(&uiCount));

  return plStreamWriterUtil::SerializeArray<ValueType>(*this, array.GetData(), array.GetCount());
}

template <typename ValueType, plUInt32 uiSize>
plResult plStreamWriter::WriteArray(const ValueType (&array)[uiSize])
{
  const plUInt64 uiWriteSize = uiSize;
  PLASMA_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  return plStreamWriterUtil::SerializeArray<ValueType>(*this, array, uiSize);
}

template <typename KeyType, typename Comparer>
plResult plStreamWriter::WriteSet(const plSetBase<KeyType, Comparer>& set)
{
  const plUInt64 uiWriteSize = set.GetCount();
  PLASMA_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (const auto& item : set)
  {
    PLASMA_SUCCEED_OR_RETURN(plStreamWriterUtil::Serialize<KeyType>(*this, item));
  }

  return PLASMA_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Comparer>
plResult plStreamWriter::WriteMap(const plMapBase<KeyType, ValueType, Comparer>& map)
{
  const plUInt64 uiWriteSize = map.GetCount();
  PLASMA_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (auto It = map.GetIterator(); It.IsValid(); ++It)
  {
    PLASMA_SUCCEED_OR_RETURN(plStreamWriterUtil::Serialize<KeyType>(*this, It.Key()));
    PLASMA_SUCCEED_OR_RETURN(plStreamWriterUtil::Serialize<ValueType>(*this, It.Value()));
  }

  return PLASMA_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Hasher>
plResult plStreamWriter::WriteHashTable(const plHashTableBase<KeyType, ValueType, Hasher>& hashTable)
{
  const plUInt64 uiWriteSize = hashTable.GetCount();
  PLASMA_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (auto It = hashTable.GetIterator(); It.IsValid(); ++It)
  {
    PLASMA_SUCCEED_OR_RETURN(plStreamWriterUtil::Serialize<KeyType>(*this, It.Key()));
    PLASMA_SUCCEED_OR_RETURN(plStreamWriterUtil::Serialize<ValueType>(*this, It.Value()));
  }

  return PLASMA_SUCCESS;
}

namespace plStreamReaderUtil
{
  template <class T>
  PLASMA_ALWAYS_INLINE auto DeserializeImpl(plStreamReader& inout_stream, T& ref_obj, int) -> decltype(inout_stream >> ref_obj, plResult(PLASMA_SUCCESS))
  {
    inout_stream >> ref_obj;

    return PLASMA_SUCCESS;
  }

  template <class T>
  PLASMA_ALWAYS_INLINE auto DeserializeImpl(plStreamReader& inout_stream, T& inout_obj, long) -> decltype(inout_obj.Deserialize(inout_stream).IgnoreResult(), plResult(PLASMA_SUCCESS))
  {
    return plToResult(inout_obj.Deserialize(inout_stream));
  }

  template <class T>
  PLASMA_ALWAYS_INLINE auto DeserializeImpl(plStreamReader& inout_stream, T& inout_obj, float) -> decltype(inout_obj.deserialize(inout_stream).IgnoreResult(), plResult(PLASMA_SUCCESS))
  {
    return plToResult(inout_obj.deserialize(inout_stream));
  }

  template <class T>
  PLASMA_ALWAYS_INLINE auto Deserialize(plStreamReader& inout_stream, T& inout_obj) -> decltype(DeserializeImpl(inout_stream, inout_obj, 0).IgnoreResult(), plResult(PLASMA_SUCCESS))
  {
    return DeserializeImpl(inout_stream, inout_obj, 0);
  }

  // serialization of array

#if PLASMA_DISABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  template <class T>
  PLASMA_ALWAYS_INLINE auto DeserializeArrayImpl(plStreamReader& inout_stream, T* pArray, plUInt64 uiCount, int) -> decltype(DeserializeArray(inout_stream, pArray, uiCount), plResult(PLASMA_SUCCESS))
  {
    return DeserializeArray(inout_stream, pArray, uiCount);
  }
#endif

  template <class T>
  plResult DeserializeArrayImpl(plStreamReader& inout_stream, T* pArray, plUInt64 uiCount, long)
  {
    for (plUInt64 i = 0; i < uiCount; ++i)
    {
      PLASMA_SUCCEED_OR_RETURN(plStreamReaderUtil::Deserialize<T>(inout_stream, pArray[i]));
    }

    return PLASMA_SUCCESS;
  }

  template <class T>
  PLASMA_ALWAYS_INLINE plResult DeserializeArray(plStreamReader& inout_stream, T* pArray, plUInt64 uiCount)
  {
    return DeserializeArrayImpl(inout_stream, pArray, uiCount, 0);
  }

} // namespace plStreamReaderUtil

template <typename ArrayType, typename ValueType>
plResult plStreamReader::ReadArray(plArrayBase<ValueType, ArrayType>& inout_array)
{
  plUInt64 uiCount = 0;
  PLASMA_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < plMath::MaxValue<plUInt32>())
  {
    inout_array.Clear();

    if (uiCount > 0)
    {
      static_cast<ArrayType&>(inout_array).SetCount(static_cast<plUInt32>(uiCount));

      PLASMA_SUCCEED_OR_RETURN(plStreamReaderUtil::DeserializeArray<ValueType>(*this, inout_array.GetData(), uiCount));
    }

    return PLASMA_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return PLASMA_FAILURE;
  }
}

template <typename ValueType, plUInt16 uiSize, typename AllocatorWrapper>
plResult plStreamReader::ReadArray(plSmallArray<ValueType, uiSize, AllocatorWrapper>& ref_array)
{
  plUInt32 uiCount = 0;
  PLASMA_SUCCEED_OR_RETURN(ReadDWordValue(&uiCount));

  if (uiCount < plMath::MaxValue<plUInt16>())
  {
    ref_array.Clear();

    if (uiCount > 0)
    {
      ref_array.SetCount(static_cast<plUInt16>(uiCount));

      PLASMA_SUCCEED_OR_RETURN(plStreamReaderUtil::DeserializeArray<ValueType>(*this, ref_array.GetData(), uiCount));
    }

    return PLASMA_SUCCESS;
  }
  else
  {
    // Small array uses 16 bit for counts internally. Value from file is too large.
    return PLASMA_FAILURE;
  }
}

template <typename ValueType, plUInt32 uiSize>
plResult plStreamReader::ReadArray(ValueType (&array)[uiSize])
{
  plUInt64 uiCount = 0;
  PLASMA_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (static_cast<plUInt32>(uiCount) != uiSize)
    return PLASMA_FAILURE;

  if (uiCount < plMath::MaxValue<plUInt32>())
  {
    PLASMA_SUCCEED_OR_RETURN(plStreamReaderUtil::DeserializeArray<ValueType>(*this, array, uiCount));

    return PLASMA_SUCCESS;
  }

  // Containers currently use 32 bit for counts internally. Value from file is too large.
  return PLASMA_FAILURE;
}

template <typename KeyType, typename Comparer>
plResult plStreamReader::ReadSet(plSetBase<KeyType, Comparer>& inout_set)
{
  plUInt64 uiCount = 0;
  PLASMA_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < plMath::MaxValue<plUInt32>())
  {
    inout_set.Clear();

    for (plUInt32 i = 0; i < static_cast<plUInt32>(uiCount); ++i)
    {
      KeyType Item;
      PLASMA_SUCCEED_OR_RETURN(plStreamReaderUtil::Deserialize(*this, Item));

      inout_set.Insert(std::move(Item));
    }

    return PLASMA_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return PLASMA_FAILURE;
  }
}

template <typename KeyType, typename ValueType, typename Comparer>
plResult plStreamReader::ReadMap(plMapBase<KeyType, ValueType, Comparer>& inout_map)
{
  plUInt64 uiCount = 0;
  PLASMA_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < plMath::MaxValue<plUInt32>())
  {
    inout_map.Clear();

    for (plUInt32 i = 0; i < static_cast<plUInt32>(uiCount); ++i)
    {
      KeyType Key;
      ValueType Value;
      PLASMA_SUCCEED_OR_RETURN(plStreamReaderUtil::Deserialize(*this, Key));
      PLASMA_SUCCEED_OR_RETURN(plStreamReaderUtil::Deserialize(*this, Value));

      inout_map.Insert(std::move(Key), std::move(Value));
    }

    return PLASMA_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return PLASMA_FAILURE;
  }
}

template <typename KeyType, typename ValueType, typename Hasher>
plResult plStreamReader::ReadHashTable(plHashTableBase<KeyType, ValueType, Hasher>& inout_hashTable)
{
  plUInt64 uiCount = 0;
  PLASMA_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < plMath::MaxValue<plUInt32>())
  {
    inout_hashTable.Clear();
    inout_hashTable.Reserve(static_cast<plUInt32>(uiCount));

    for (plUInt32 i = 0; i < static_cast<plUInt32>(uiCount); ++i)
    {
      KeyType Key;
      ValueType Value;
      PLASMA_SUCCEED_OR_RETURN(plStreamReaderUtil::Deserialize(*this, Key));
      PLASMA_SUCCEED_OR_RETURN(plStreamReaderUtil::Deserialize(*this, Value));

      inout_hashTable.Insert(std::move(Key), std::move(Value));
    }

    return PLASMA_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return PLASMA_FAILURE;
  }
}


#include <Foundation/IO/Stream.h>

namespace plInternal
{
  // This internal helper is needed to differentiate between reference and pointer which is not possible with regular function overloading
  // in this case.
  template <typename T>
  struct WriteObjectHelper
  {
    static const T* GetAddress(const T& obj) { return &obj; }
  };

  template <typename T>
  struct WriteObjectHelper<T*>
  {
    static const T* GetAddress(const T* pObj) { return pObj; }
  };
} // namespace plInternal

template <typename T>
PLASMA_ALWAYS_INLINE plResult plDeduplicationWriteContext::WriteObject(plStreamWriter& inout_stream, const T& obj)
{
  return WriteObjectInternal(inout_stream, plInternal::WriteObjectHelper<T>::GetAddress(obj));
}

template <typename T>
PLASMA_ALWAYS_INLINE plResult plDeduplicationWriteContext::WriteObject(plStreamWriter& inout_stream, const plSharedPtr<T>& pObject)
{
  return WriteObjectInternal(inout_stream, pObject.Borrow());
}

template <typename T>
PLASMA_ALWAYS_INLINE plResult plDeduplicationWriteContext::WriteObject(plStreamWriter& inout_stream, const plUniquePtr<T>& pObject)
{
  return WriteObjectInternal(inout_stream, pObject.Borrow());
}

template <typename ArrayType, typename ValueType>
plResult plDeduplicationWriteContext::WriteArray(plStreamWriter& inout_stream, const plArrayBase<ValueType, ArrayType>& array)
{
  const plUInt64 uiCount = array.GetCount();
  PLASMA_SUCCEED_OR_RETURN(inout_stream.WriteQWordValue(&uiCount));

  for (plUInt32 i = 0; i < static_cast<plUInt32>(uiCount); ++i)
  {
    PLASMA_SUCCEED_OR_RETURN(WriteObject(inout_stream, array[i]));
  }

  return PLASMA_SUCCESS;
}

template <typename KeyType, typename Comparer>
plResult plDeduplicationWriteContext::WriteSet(plStreamWriter& inout_stream, const plSetBase<KeyType, Comparer>& set)
{
  const plUInt64 uiWriteSize = set.GetCount();
  PLASMA_SUCCEED_OR_RETURN(inout_stream.WriteQWordValue(&uiWriteSize));

  for (const auto& item : set)
  {
    PLASMA_SUCCEED_OR_RETURN(WriteObject(inout_stream, item));
  }

  return PLASMA_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Comparer>
plResult plDeduplicationWriteContext::WriteMap(plStreamWriter& inout_stream, const plMapBase<KeyType, ValueType, Comparer>& map, WriteMapMode mode)
{
  const plUInt64 uiWriteSize = map.GetCount();
  PLASMA_SUCCEED_OR_RETURN(inout_stream.WriteQWordValue(&uiWriteSize));

  if (mode == WriteMapMode::DedupKey)
  {
    for (auto It = map.GetIterator(); It.IsValid(); ++It)
    {
      PLASMA_SUCCEED_OR_RETURN(WriteObject(inout_stream, It.Key()));
      PLASMA_SUCCEED_OR_RETURN(plStreamWriterUtil::Serialize<ValueType>(inout_stream, It.Value()));
    }
  }
  else if (mode == WriteMapMode::DedupValue)
  {
    for (auto It = map.GetIterator(); It.IsValid(); ++It)
    {
      PLASMA_SUCCEED_OR_RETURN(plStreamWriterUtil::Serialize<KeyType>(inout_stream, It.Key()));
      PLASMA_SUCCEED_OR_RETURN(WriteObject(inout_stream, It.Value()));
    }
  }
  else
  {
    for (auto It = map.GetIterator(); It.IsValid(); ++It)
    {
      PLASMA_SUCCEED_OR_RETURN(WriteObject(inout_stream, It.Key()));
      PLASMA_SUCCEED_OR_RETURN(WriteObject(inout_stream, It.Value()));
    }
  }

  return PLASMA_SUCCESS;
}

template <typename T>
plResult plDeduplicationWriteContext::WriteObjectInternal(plStreamWriter& stream, const T* pObject)
{
  plUInt32 uiIndex = plInvalidIndex;

  if (pObject)
  {
    bool bIsRealObject = !m_Objects.TryGetValue(pObject, uiIndex);
    stream << bIsRealObject;

    if (bIsRealObject)
    {
      uiIndex = m_Objects.GetCount();
      m_Objects.Insert(pObject, uiIndex);

      return plStreamWriterUtil::Serialize<T>(stream, *pObject);
    }
    else
    {
      stream << uiIndex;
    }
  }
  else
  {
    stream << false;
    stream << plInvalidIndex;
  }

  return PLASMA_SUCCESS;
}

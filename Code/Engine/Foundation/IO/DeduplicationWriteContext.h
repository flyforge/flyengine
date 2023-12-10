
#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

class plStreamWriter;

/// \brief Serialization Context that de-duplicates objects when writing to a stream. Duplicated objects are identified by their address and
/// only the first occurrence is written to the stream while all subsequence occurrences are just written as an index.
class PLASMA_FOUNDATION_DLL plDeduplicationWriteContext : public plSerializationContext<plDeduplicationWriteContext>
{
  PLASMA_DECLARE_SERIALIZATION_CONTEXT(plDeduplicationWriteContext);

public:
  plDeduplicationWriteContext();
  ~plDeduplicationWriteContext();

  /// \brief Writes a single object to the stream. Can be either a reference or a pointer to the object.
  template <typename T>
  plResult WriteObject(plStreamWriter& inout_stream, const T& obj); // [tested]

  /// \brief Writes a single object to the stream.
  template <typename T>
  plResult WriteObject(plStreamWriter& inout_stream, const plSharedPtr<T>& pObject); // [tested]

  /// \brief Writes a single object to the stream.
  template <typename T>
  plResult WriteObject(plStreamWriter& inout_stream, const plUniquePtr<T>& pObject); // [tested]

  /// \brief Writes an array of de-duplicated objects.
  template <typename ArrayType, typename ValueType>
  plResult WriteArray(plStreamWriter& inout_stream, const plArrayBase<ValueType, ArrayType>& array); // [tested]

  /// \brief Writes a set of de-duplicated objects.
  template <typename KeyType, typename Comparer>
  plResult WriteSet(plStreamWriter& inout_stream, const plSetBase<KeyType, Comparer>& set); // [tested]

  enum class WriteMapMode
  {
    DedupKey,
    DedupValue,
    DedupBoth
  };

  /// \brief Writes a map. Mode controls whether key or value or both should de-duplicated.
  template <typename KeyType, typename ValueType, typename Comparer>
  plResult WriteMap(plStreamWriter& inout_stream, const plMapBase<KeyType, ValueType, Comparer>& map, WriteMapMode mode); // [tested]

private:
  template <typename T>
  plResult WriteObjectInternal(plStreamWriter& stream, const T* pObject);

  plHashTable<const void*, plUInt32> m_Objects;
};

#include <Foundation/IO/Implementation/DeduplicationWriteContext_inl.h>

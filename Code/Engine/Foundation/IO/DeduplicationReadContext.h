
#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

class plStreamReader;

/// \brief Serialization Context that reads de-duplicated objects from a stream and restores the pointers.
class PLASMA_FOUNDATION_DLL plDeduplicationReadContext : public plSerializationContext<plDeduplicationReadContext>
{
  PLASMA_DECLARE_SERIALIZATION_CONTEXT(plDeduplicationReadContext);

public:
  plDeduplicationReadContext();
  ~plDeduplicationReadContext();

  /// \brief Reads a single object inplace.
  template <typename T>
  plResult ReadObjectInplace(plStreamReader& inout_stream, T& ref_obj); // [tested]

  /// \brief Reads a single object and sets the pointer to it. The given allocator is used to create the object if it doesn't exist yet.
  template <typename T>
  plResult ReadObject(plStreamReader& inout_stream, T*& ref_pObject,
    plAllocatorBase* pAllocator = plFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads a single object and sets the shared pointer to it. The given allocator is used to create the object if it doesn't exist
  /// yet.
  template <typename T>
  plResult ReadObject(plStreamReader& inout_stream, plSharedPtr<T>& ref_pObject,
    plAllocatorBase* pAllocator = plFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads a single object and sets the unique pointer to it. The given allocator is used to create the object if it doesn't exist
  /// yet.
  template <typename T>
  plResult ReadObject(plStreamReader& inout_stream, plUniquePtr<T>& ref_pObject,
    plAllocatorBase* pAllocator = plFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads an array of de-duplicated objects.
  template <typename ArrayType, typename ValueType>
  plResult ReadArray(plStreamReader& inout_stream, plArrayBase<ValueType, ArrayType>& ref_array,
    plAllocatorBase* pAllocator = plFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads a set of de-duplicated objects.
  template <typename KeyType, typename Comparer>
  plResult ReadSet(plStreamReader& inout_stream, plSetBase<KeyType, Comparer>& ref_set,
    plAllocatorBase* pAllocator = plFoundation::GetDefaultAllocator()); // [tested]

  enum class ReadMapMode
  {
    DedupKey,
    DedupValue,
    DedupBoth
  };

  /// \brief Reads a map. Mode controls whether key or value or both should de-duplicated.
  template <typename KeyType, typename ValueType, typename Comparer>
  plResult ReadMap(plStreamReader& inout_stream, plMapBase<KeyType, ValueType, Comparer>& ref_map, ReadMapMode mode,
    plAllocatorBase* pKeyAllocator = plFoundation::GetDefaultAllocator(),
    plAllocatorBase* pValueAllocator = plFoundation::GetDefaultAllocator()); // [tested]

private:
  template <typename T>
  plResult ReadObject(plStreamReader& stream, T& obj, plAllocatorBase* pAllocator); // [tested]

  plDynamicArray<void*> m_Objects;
};

#include <Foundation/IO/Implementation/DeduplicationReadContext_inl.h>

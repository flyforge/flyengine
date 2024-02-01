#pragma once

#include <Foundation/Containers/ArrayBase.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/PointerWithFlags.h>

/// \brief Implementation of a dynamically growing array.
///
/// Best-case performance for the PushBack operation is O(1) if the plDynamicArray doesn't need to be expanded.
/// In the worst case, PushBack is O(n).
/// Look-up is guaranteed to always be O(1).
template <typename T>
class plDynamicArrayBase : public plArrayBase<T, plDynamicArrayBase<T>>
{
protected:
  /// \brief Creates an empty array. Does not allocate any data yet.
  explicit plDynamicArrayBase(plAllocator* pAllocator); // [tested]

  plDynamicArrayBase(T* pInplaceStorage, plUInt32 uiCapacity, plAllocator* pAllocator); // [tested]

  /// \brief Creates a copy of the given array.
  plDynamicArrayBase(const plDynamicArrayBase<T>& other, plAllocator* pAllocator); // [tested]

  /// \brief Moves the given array into this one.
  plDynamicArrayBase(plDynamicArrayBase<T>&& other, plAllocator* pAllocator); // [tested]

  /// \brief Creates a copy of the given array.
  plDynamicArrayBase(const plArrayPtr<const T>& other, plAllocator* pAllocator); // [tested]

  /// \brief Destructor.
  ~plDynamicArrayBase(); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator=(const plDynamicArrayBase<T>& rhs); // [tested]

  /// \brief Moves the data from some other contiguous array into this one.
  void operator=(plDynamicArrayBase<T>&& rhs) noexcept; // [tested]

  T* GetElementsPtr();
  const T* GetElementsPtr() const;

  friend class plArrayBase<T, plDynamicArrayBase<T>>;

public:
  /// \brief Expands the array so it can at least store the given capacity.
  void Reserve(plUInt32 uiCapacity); // [tested]

  /// \brief Tries to compact the array to avoid wasting memory. The resulting capacity is at least 'GetCount' (no elements get removed). Will
  /// deallocate all data, if the array is empty.
  void Compact(); // [tested]

  /// \brief Returns the allocator that is used by this instance.
  plAllocator* GetAllocator() const { return const_cast<plAllocator*>(m_pAllocator.GetPtr()); }

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  plUInt64 GetHeapMemoryUsage() const; // [tested]

  /// \brief swaps the contents of this array with another one
  void Swap(plDynamicArrayBase<T>& other); // [tested]

private:
  enum Storage
  {
    Owned = 0,
    External = 1
  };

  plPointerWithFlags<plAllocator, 1> m_pAllocator;

  enum
  {
    CAPACITY_ALIGNMENT = 16
  };

  void SetCapacity(plUInt32 uiCapacity);
};

/// \brief \see plDynamicArrayBase
template <typename T, typename AllocatorWrapper = plDefaultAllocatorWrapper>
class plDynamicArray : public plDynamicArrayBase<T>
{
public:
  PL_DECLARE_MEM_RELOCATABLE_TYPE();


  plDynamicArray();
  explicit plDynamicArray(plAllocator* pAllocator);

  plDynamicArray(const plDynamicArray<T, AllocatorWrapper>& other);
  plDynamicArray(const plDynamicArrayBase<T>& other);
  explicit plDynamicArray(const plArrayPtr<const T>& other);

  plDynamicArray(plDynamicArray<T, AllocatorWrapper>&& other);
  plDynamicArray(plDynamicArrayBase<T>&& other);

  void operator=(const plDynamicArray<T, AllocatorWrapper>& rhs);
  void operator=(const plDynamicArrayBase<T>& rhs);
  void operator=(const plArrayPtr<const T>& rhs);

  void operator=(plDynamicArray<T, AllocatorWrapper>&& rhs) noexcept;
  void operator=(plDynamicArrayBase<T>&& rhs) noexcept;

protected:
  plDynamicArray(T* pInplaceStorage, plUInt32 uiCapacity, plAllocator* pAllocator)
    : plDynamicArrayBase<T>(pInplaceStorage, uiCapacity, pAllocator)
  {
  }
};

/// Overload of plMakeArrayPtr for const dynamic arrays of pointer pointing to const type.
template <typename T, typename AllocatorWrapper>
plArrayPtr<const T* const> plMakeArrayPtr(const plDynamicArray<T*, AllocatorWrapper>& dynArray);

/// Overload of plMakeArrayPtr for const dynamic arrays.
template <typename T, typename AllocatorWrapper>
plArrayPtr<const T> plMakeArrayPtr(const plDynamicArray<T, AllocatorWrapper>& dynArray);

/// Overload of plMakeArrayPtr for dynamic arrays.
template <typename T, typename AllocatorWrapper>
plArrayPtr<T> plMakeArrayPtr(plDynamicArray<T, AllocatorWrapper>& in_dynArray);


PL_CHECK_AT_COMPILETIME_MSG(plGetTypeClass<plDynamicArray<int>>::value == 2, "dynamic array is not memory relocatable");

#include <Foundation/Containers/Implementation/DynamicArray_inl.h>

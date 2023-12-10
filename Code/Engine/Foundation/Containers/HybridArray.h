#pragma once

#include <Foundation/Containers/DynamicArray.h>

/// \brief A hybrid array uses in-place storage to handle the first few elements without any allocation. It dynamically resizes when more elements are needed.
///
/// It is often more efficient to use a hybrid array, rather than a dynamic array, when the number of needed elements is typically low or when the array is used only temporarily. In this case costly allocations can often be prevented entirely.
/// However, if the number of elements is unpredictable or usually very large, prefer a dynamic array, to avoid wasting (stack) memory for a hybrid array that is rarely large enough to be used.
/// The plHybridArray is derived from plDynamicArray and can therefore be passed to functions that expect an plDynamicArray, even for output.
template <typename T, plUInt32 Size, typename AllocatorWrapper = plDefaultAllocatorWrapper>
class plHybridArray : public plDynamicArray<T, AllocatorWrapper>
{
public:
  /// \brief Creates an empty array. Does not allocate any data yet.
  plHybridArray(); // [tested]

  /// \brief Creates an empty array. Does not allocate any data yet.
  explicit plHybridArray(plAllocatorBase* pAllocator); // [tested]

  /// \brief Creates a copy of the given array.
  plHybridArray(const plHybridArray<T, Size, AllocatorWrapper>& other); // [tested]

  /// \brief Creates a copy of the given array.
  explicit plHybridArray(const plArrayPtr<const T>& other); // [tested]

  /// \brief Moves the given array.
  plHybridArray(plHybridArray<T, Size, AllocatorWrapper>&& other); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator=(const plHybridArray<T, Size, AllocatorWrapper>& rhs); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator=(const plArrayPtr<const T>& rhs); // [tested]

  /// \brief Moves the data from some other contiguous array into this one.
  void operator=(plHybridArray<T, Size, AllocatorWrapper>&& rhs) noexcept; // [tested]

protected:
  /// \brief The fixed size array.
  struct alignas(PLASMA_ALIGNMENT_OF(T))
  {
    plUInt8 m_StaticData[Size * sizeof(T)];
  };

  PLASMA_ALWAYS_INLINE T* GetStaticArray() { return reinterpret_cast<T*>(m_StaticData); }

  PLASMA_ALWAYS_INLINE const T* GetStaticArray() const { return reinterpret_cast<const T*>(m_StaticData); }
};

#include <Foundation/Containers/Implementation/HybridArray_inl.h>

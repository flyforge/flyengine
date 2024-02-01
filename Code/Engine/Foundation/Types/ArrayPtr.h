#pragma once

#include <Foundation/Memory/MemoryUtils.h>

#include <Foundation/Containers/Implementation/ArrayIterator.h>

// This #include is quite vital, do not remove it!
#include <Foundation/Strings/FormatString.h>

#include <Foundation/Math/Math.h>

/// \brief Value used by containers for indices to indicate an invalid index.
#ifndef plInvalidIndex
#  define plInvalidIndex 0xFFFFFFFF
#endif

namespace plArrayPtrDetail
{
  template <typename U>
  struct ByteTypeHelper
  {
    using type = plUInt8;
  };

  template <typename U>
  struct ByteTypeHelper<const U>
  {
    using type = const plUInt8;
  };
} // namespace plArrayPtrDetail

/// \brief This class encapsulates an array and it's size. It is recommended to use this class instead of plain C arrays.
///
/// No data is deallocated at destruction, the plArrayPtr only allows for easier access.
template <typename T>
class plArrayPtr
{
  template <typename U>
  friend class plArrayPtr;

public:
  PL_DECLARE_POD_TYPE();

  static_assert(!std::is_same_v<T, void>, "plArrayPtr<void> is not allowed (anymore)");
  static_assert(!std::is_same_v<T, const void>, "plArrayPtr<void> is not allowed (anymore)");

  using ByteType = typename plArrayPtrDetail::ByteTypeHelper<T>::type;
  using ValueType = T;
  using PointerType = T*;

  /// \brief Initializes the plArrayPtr to be empty.
  PL_ALWAYS_INLINE plArrayPtr() // [tested]
    : m_pPtr(nullptr)
    , m_uiCount(0u)
  {
  }

  /// \brief Copies the pointer and size of /a other. Does not allocate any data.
  PL_ALWAYS_INLINE plArrayPtr(const plArrayPtr<T>& other) // [tested]
  {
    m_pPtr = other.m_pPtr;
    m_uiCount = other.m_uiCount;
  }

  /// \brief Initializes the plArrayPtr with the given pointer and number of elements. No memory is allocated or copied.
  inline plArrayPtr(T* pPtr, plUInt32 uiCount) // [tested]
    : m_pPtr(pPtr)
    , m_uiCount(uiCount)
  {
    // If any of the arguments is invalid, we invalidate ourself.
    if (m_pPtr == nullptr || m_uiCount == 0)
    {
      m_pPtr = nullptr;
      m_uiCount = 0;
    }
  }

  /// \brief Initializes the plArrayPtr to encapsulate the given array.
  template <size_t N>
  PL_ALWAYS_INLINE plArrayPtr(T (&staticArray)[N]) // [tested]
    : m_pPtr(staticArray)
    , m_uiCount(static_cast<plUInt32>(N))
  {
  }

  /// \brief Initializes the plArrayPtr to be a copy of \a other. No memory is allocated or copied.
  template <typename U>
  PL_ALWAYS_INLINE plArrayPtr(const plArrayPtr<U>& other) // [tested]
    : m_pPtr(other.m_pPtr)
    , m_uiCount(other.m_uiCount)
  {
  }

  /// \brief Convert to const version.
  operator plArrayPtr<const T>() const { return plArrayPtr<const T>(static_cast<const T*>(GetPtr()), GetCount()); } // [tested]

  /// \brief Copies the pointer and size of /a other. Does not allocate any data.
  PL_ALWAYS_INLINE void operator=(const plArrayPtr<T>& other) // [tested]
  {
    m_pPtr = other.m_pPtr;
    m_uiCount = other.m_uiCount;
  }

  /// \brief Clears the array
  PL_ALWAYS_INLINE void Clear()
  {
    m_pPtr = nullptr;
    m_uiCount = 0;
  }

  PL_ALWAYS_INLINE void operator=(std::nullptr_t) // [tested]
  {
    m_pPtr = nullptr;
    m_uiCount = 0;
  }

  /// \brief Returns the pointer to the array.
  PL_ALWAYS_INLINE PointerType GetPtr() const // [tested]
  {
    return m_pPtr;
  }

  /// \brief Returns the pointer to the array.
  PL_ALWAYS_INLINE PointerType GetPtr() // [tested]
  {
    return m_pPtr;
  }

  /// \brief Returns the pointer behind the last element of the array
  PL_ALWAYS_INLINE PointerType GetEndPtr() { return m_pPtr + m_uiCount; }

  /// \brief Returns the pointer behind the last element of the array
  PL_ALWAYS_INLINE PointerType GetEndPtr() const { return m_pPtr + m_uiCount; }

  /// \brief Returns whether the array is empty.
  PL_ALWAYS_INLINE bool IsEmpty() const // [tested]
  {
    return GetCount() == 0;
  }

  /// \brief Returns the number of elements in the array.
  PL_ALWAYS_INLINE plUInt32 GetCount() const // [tested]
  {
    return m_uiCount;
  }

  /// \brief Creates a sub-array from this array.
  PL_FORCE_INLINE plArrayPtr<T> GetSubArray(plUInt32 uiStart, plUInt32 uiCount) const // [tested]
  {
    // the first check is necessary to also detect errors when uiStart+uiCount would overflow
    PL_ASSERT_DEV(uiStart <= GetCount() && uiStart + uiCount <= GetCount(), "uiStart+uiCount ({0}) has to be smaller or equal than the count ({1}).",
      uiStart + uiCount, GetCount());
    return plArrayPtr<T>(GetPtr() + uiStart, uiCount);
  }

  /// \brief Creates a sub-array from this array.
  /// \note \code ap.GetSubArray(i) \endcode is equivalent to \code ap.GetSubArray(i, ap.GetCount() - i) \endcode.
  PL_FORCE_INLINE plArrayPtr<T> GetSubArray(plUInt32 uiStart) const // [tested]
  {
    PL_ASSERT_DEV(uiStart <= GetCount(), "uiStart ({0}) has to be smaller or equal than the count ({1}).", uiStart, GetCount());
    return plArrayPtr<T>(GetPtr() + uiStart, GetCount() - uiStart);
  }

  /// \brief Reinterprets this array as a byte array.
  PL_ALWAYS_INLINE plArrayPtr<const ByteType> ToByteArray() const
  {
    return plArrayPtr<const ByteType>(reinterpret_cast<const ByteType*>(GetPtr()), GetCount() * sizeof(T));
  }

  /// \brief Reinterprets this array as a byte array.
  PL_ALWAYS_INLINE plArrayPtr<ByteType> ToByteArray() { return plArrayPtr<ByteType>(reinterpret_cast<ByteType*>(GetPtr()), GetCount() * sizeof(T)); }


  /// \brief Cast an ArrayPtr to an ArrayPtr to a different, but same size, type
  template <typename U>
  PL_ALWAYS_INLINE plArrayPtr<U> Cast()
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return plArrayPtr<U>(reinterpret_cast<U*>(GetPtr()), GetCount());
  }

  /// \brief Cast an ArrayPtr to an ArrayPtr to a different, but same size, type
  template <typename U>
  PL_ALWAYS_INLINE plArrayPtr<const U> Cast() const
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return plArrayPtr<const U>(reinterpret_cast<const U*>(GetPtr()), GetCount());
  }

  /// \brief Index access.
  PL_FORCE_INLINE const ValueType& operator[](plUInt32 uiIndex) const // [tested]
  {
    PL_ASSERT_DEBUG(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<const ValueType*>(GetPtr() + uiIndex);
  }

  /// \brief Index access.
  PL_FORCE_INLINE ValueType& operator[](plUInt32 uiIndex) // [tested]
  {
    PL_ASSERT_DEBUG(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<ValueType*>(GetPtr() + uiIndex);
  }

  /// \brief Compares the two arrays for equality.
  template <typename = typename std::enable_if<std::is_const<T>::value == false>>
  inline bool operator==(const plArrayPtr<const T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return false;

    if (GetPtr() == other.GetPtr())
      return true;

    return plMemoryUtils::IsEqual(static_cast<const ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }

#if PL_DISABLED(PL_USE_CPP20_OPERATORS)
  template <typename = typename std::enable_if<std::is_const<T>::value == false>>
  inline bool operator!=(const plArrayPtr<const T>& other) const // [tested]
  {
    return !(*this == other);
  }
#endif

  /// \brief Compares the two arrays for equality.
  inline bool operator==(const plArrayPtr<T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return false;

    if (GetPtr() == other.GetPtr())
      return true;

    return plMemoryUtils::IsEqual(static_cast<const ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }
  PL_ADD_DEFAULT_OPERATOR_NOTEQUAL(const plArrayPtr<T>&);

  /// \brief Compares the two arrays for less.
  inline bool operator<(const plArrayPtr<const T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return GetCount() < other.GetCount();

    for (plUInt32 i = 0; i < GetCount(); ++i)
    {
      if (GetPtr()[i] < other.GetPtr()[i])
        return true;

      if (other.GetPtr()[i] < GetPtr()[i])
        return false;
    }

    return false;
  }

  /// \brief Copies the data from \a other into this array. The arrays must have the exact same size.
  inline void CopyFrom(const plArrayPtr<const T>& other) // [tested]
  {
    PL_ASSERT_DEV(GetCount() == other.GetCount(), "Count for copy does not match. Target has {0} elements, source {1} elements", GetCount(), other.GetCount());

    plMemoryUtils::Copy(static_cast<ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }

  PL_ALWAYS_INLINE void Swap(plArrayPtr<T>& other)
  {
    ::plMath::Swap(m_pPtr, other.m_pPtr);
    ::plMath::Swap(m_uiCount, other.m_uiCount);
  }

  /// \brief Checks whether the given value can be found in the array. O(n) complexity.
  PL_ALWAYS_INLINE bool Contains(const T& value) const // [tested]
  {
    return IndexOf(value) != plInvalidIndex;
  }

  /// \brief Searches for the first occurrence of the given value and returns its index or plInvalidIndex if not found.
  inline plUInt32 IndexOf(const T& value, plUInt32 uiStartIndex = 0) const // [tested]
  {
    for (plUInt32 i = uiStartIndex; i < m_uiCount; ++i)
    {
      if (plMemoryUtils::IsEqual(m_pPtr + i, &value))
        return i;
    }

    return plInvalidIndex;
  }

  /// \brief Searches for the last occurrence of the given value and returns its index or plInvalidIndex if not found.
  inline plUInt32 LastIndexOf(const T& value, plUInt32 uiStartIndex = plInvalidIndex) const // [tested]
  {
    for (plUInt32 i = ::plMath::Min(uiStartIndex, m_uiCount); i-- > 0;)
    {
      if (plMemoryUtils::IsEqual(m_pPtr + i, &value))
        return i;
    }
    return plInvalidIndex;
  }

  using const_iterator = const T*;
  using const_reverse_iterator = const_reverse_pointer_iterator<T>;
  using iterator = T*;
  using reverse_iterator = reverse_pointer_iterator<T>;

private:
  PointerType m_pPtr;
  plUInt32 m_uiCount;
};

//////////////////////////////////////////////////////////////////////////

using plByteArrayPtr = plArrayPtr<plUInt8>;
using plConstByteArrayPtr = plArrayPtr<const plUInt8>;

//////////////////////////////////////////////////////////////////////////

/// \brief Helper function to create plArrayPtr from a pointer of some type and a count.
template <typename T>
PL_ALWAYS_INLINE plArrayPtr<T> plMakeArrayPtr(T* pPtr, plUInt32 uiCount)
{
  return plArrayPtr<T>(pPtr, uiCount);
}

/// \brief Helper function to create plArrayPtr from a static array the a size known at compile-time.
template <typename T, plUInt32 N>
PL_ALWAYS_INLINE plArrayPtr<T> plMakeArrayPtr(T (&staticArray)[N])
{
  return plArrayPtr<T>(staticArray);
}

/// \brief Helper function to create plConstByteArrayPtr from a pointer of some type and a count.
template <typename T>
PL_ALWAYS_INLINE plConstByteArrayPtr plMakeByteArrayPtr(const T* pPtr, plUInt32 uiCount)
{
  return plConstByteArrayPtr(static_cast<const plUInt8*>(pPtr), uiCount * sizeof(T));
}

/// \brief Helper function to create plByteArrayPtr from a pointer of some type and a count.
template <typename T>
PL_ALWAYS_INLINE plByteArrayPtr plMakeByteArrayPtr(T* pPtr, plUInt32 uiCount)
{
  return plByteArrayPtr(reinterpret_cast<plUInt8*>(pPtr), uiCount * sizeof(T));
}

/// \brief Helper function to create plByteArrayPtr from a void pointer and a count.
PL_ALWAYS_INLINE plByteArrayPtr plMakeByteArrayPtr(void* pPtr, plUInt32 uiBytes)
{
  return plByteArrayPtr(reinterpret_cast<plUInt8*>(pPtr), uiBytes);
}

/// \brief Helper function to create plConstByteArrayPtr from a const void pointer and a count.
PL_ALWAYS_INLINE plConstByteArrayPtr plMakeByteArrayPtr(const void* pPtr, plUInt32 uiBytes)
{
  return plConstByteArrayPtr(static_cast<const plUInt8*>(pPtr), uiBytes);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
typename plArrayPtr<T>::iterator begin(plArrayPtr<T>& ref_container)
{
  return ref_container.GetPtr();
}

template <typename T>
typename plArrayPtr<T>::const_iterator begin(const plArrayPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename plArrayPtr<T>::const_iterator cbegin(const plArrayPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename plArrayPtr<T>::reverse_iterator rbegin(plArrayPtr<T>& ref_container)
{
  return typename plArrayPtr<T>::reverse_iterator(ref_container.GetPtr() + ref_container.GetCount() - 1);
}

template <typename T>
typename plArrayPtr<T>::const_reverse_iterator rbegin(const plArrayPtr<T>& container)
{
  return typename plArrayPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename plArrayPtr<T>::const_reverse_iterator crbegin(const plArrayPtr<T>& container)
{
  return typename plArrayPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename plArrayPtr<T>::iterator end(plArrayPtr<T>& ref_container)
{
  return ref_container.GetPtr() + ref_container.GetCount();
}

template <typename T>
typename plArrayPtr<T>::const_iterator end(const plArrayPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename plArrayPtr<T>::const_iterator cend(const plArrayPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename plArrayPtr<T>::reverse_iterator rend(plArrayPtr<T>& ref_container)
{
  return typename plArrayPtr<T>::reverse_iterator(ref_container.GetPtr() - 1);
}

template <typename T>
typename plArrayPtr<T>::const_reverse_iterator rend(const plArrayPtr<T>& container)
{
  return typename plArrayPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

template <typename T>
typename plArrayPtr<T>::const_reverse_iterator crend(const plArrayPtr<T>& container)
{
  return typename plArrayPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

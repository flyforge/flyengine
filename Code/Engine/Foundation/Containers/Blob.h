
#pragma once

#include <Foundation/Basics.h>

/// \brief This class encapsulates a blob's storage and it's size. It is recommended to use this class instead of directly working on the void* of the
/// blob.
///
/// No data is deallocated at destruction, the plBlobPtr only allows for easier access.
template <typename T>
class plBlobPtr
{
public:
  PL_DECLARE_POD_TYPE();

  static_assert(!std::is_same_v<T, void>, "plBlobPtr<void> is not allowed (anymore)");
  static_assert(!std::is_same_v<T, const void>, "plBlobPtr<void> is not allowed (anymore)");

  using ByteType = typename plArrayPtrDetail::ByteTypeHelper<T>::type;
  using ValueType = T;
  using PointerType = T*;

  /// \brief Initializes the plBlobPtr to be empty.
  plBlobPtr() = default;

  /// \brief Initializes the plBlobPtr with the given pointer and number of elements. No memory is allocated or copied.
  template <typename U>
  inline plBlobPtr(U* pPtr, plUInt64 uiCount)
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

  /// \brief Initializes the plBlobPtr to encapsulate the given array.
  template <size_t N>
  PL_ALWAYS_INLINE plBlobPtr(ValueType (&staticArray)[N])
    : m_pPtr(staticArray)
    , m_uiCount(static_cast<plUInt64>(N))
  {
  }

  /// \brief Initializes the plBlobPtr to be a copy of \a other. No memory is allocated or copied.
  PL_ALWAYS_INLINE plBlobPtr(const plBlobPtr<T>& other)
    : m_pPtr(other.m_pPtr)
    , m_uiCount(other.m_uiCount)
  {
  }

  /// \brief Convert to const version.
  operator plBlobPtr<const T>() const { return plBlobPtr<const T>(static_cast<const T*>(GetPtr()), GetCount()); }

  /// \brief Copies the pointer and size of /a other. Does not allocate any data.
  PL_ALWAYS_INLINE void operator=(const plBlobPtr<T>& other)
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

  PL_ALWAYS_INLINE void operator=(std::nullptr_t)
  {
    m_pPtr = nullptr;
    m_uiCount = 0;
  }

  /// \brief Returns the pointer to the array.
  PL_ALWAYS_INLINE PointerType GetPtr() const { return m_pPtr; }

  /// \brief Returns the pointer to the array.
  PL_ALWAYS_INLINE PointerType GetPtr() { return m_pPtr; }

  /// \brief Returns the pointer behind the last element of the array
  PL_ALWAYS_INLINE PointerType GetEndPtr() { return m_pPtr + m_uiCount; }

  /// \brief Returns the pointer behind the last element of the array
  PL_ALWAYS_INLINE PointerType GetEndPtr() const { return m_pPtr + m_uiCount; }

  /// \brief Returns whether the array is empty.
  PL_ALWAYS_INLINE bool IsEmpty() const { return GetCount() == 0; }

  /// \brief Returns the number of elements in the array.
  PL_ALWAYS_INLINE plUInt64 GetCount() const { return m_uiCount; }

  /// \brief Creates a sub-array from this array.
  PL_FORCE_INLINE plBlobPtr<T> GetSubArray(plUInt64 uiStart, plUInt64 uiCount) const // [tested]
  {
    PL_ASSERT_DEV(
      uiStart + uiCount <= GetCount(), "uiStart+uiCount ({0}) has to be smaller or equal than the count ({1}).", uiStart + uiCount, GetCount());
    return plBlobPtr<T>(GetPtr() + uiStart, uiCount);
  }

  /// \brief Creates a sub-array from this array.
  /// \note \code ap.GetSubArray(i) \endcode is equivalent to \code ap.GetSubArray(i, ap.GetCount() - i) \endcode.
  PL_FORCE_INLINE plBlobPtr<T> GetSubArray(plUInt64 uiStart) const // [tested]
  {
    PL_ASSERT_DEV(uiStart <= GetCount(), "uiStart ({0}) has to be smaller or equal than the count ({1}).", uiStart, GetCount());
    return plBlobPtr<T>(GetPtr() + uiStart, GetCount() - uiStart);
  }

  /// \brief Reinterprets this array as a byte array.
  PL_ALWAYS_INLINE plBlobPtr<const ByteType> ToByteBlob() const
  {
    return plBlobPtr<const ByteType>(reinterpret_cast<const ByteType*>(GetPtr()), GetCount() * sizeof(T));
  }

  /// \brief Reinterprets this array as a byte array.
  PL_ALWAYS_INLINE plBlobPtr<ByteType> ToByteBlob() { return plBlobPtr<ByteType>(reinterpret_cast<ByteType*>(GetPtr()), GetCount() * sizeof(T)); }

  /// \brief Cast an BlobPtr to an BlobPtr to a different, but same size, type
  template <typename U>
  PL_ALWAYS_INLINE plBlobPtr<U> Cast()
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return plBlobPtr<U>(reinterpret_cast<U*>(GetPtr()), GetCount());
  }

  /// \brief Cast an BlobPtr to an BlobPtr to a different, but same size, type
  template <typename U>
  PL_ALWAYS_INLINE plBlobPtr<const U> Cast() const
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return plBlobPtr<const U>(reinterpret_cast<const U*>(GetPtr()), GetCount());
  }

  /// \brief Index access.
  PL_FORCE_INLINE const ValueType& operator[](plUInt64 uiIndex) const // [tested]
  {
    PL_ASSERT_DEBUG(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<const ValueType*>(GetPtr() + uiIndex);
  }

  /// \brief Index access.
  PL_FORCE_INLINE ValueType& operator[](plUInt64 uiIndex) // [tested]
  {
    PL_ASSERT_DEBUG(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<ValueType*>(GetPtr() + uiIndex);
  }

  /// \brief Compares the two arrays for equality.
  inline bool operator==(const plBlobPtr<const T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return false;

    if (GetPtr() == other.GetPtr())
      return true;

    return plMemoryUtils::IsEqual(static_cast<const ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), static_cast<size_t>(GetCount()));
  }

  /// \brief Compares the two arrays for inequality.
  PL_ALWAYS_INLINE bool operator!=(const plBlobPtr<const T>& other) const // [tested]
  {
    return !(*this == other);
  }

  /// \brief Copies the data from \a other into this array. The arrays must have the exact same size.
  inline void CopyFrom(const plBlobPtr<const T>& other) // [tested]
  {
    PL_ASSERT_DEV(GetCount() == other.GetCount(), "Count for copy does not match. Target has {0} elements, source {1} elements", GetCount(), other.GetCount());

    plMemoryUtils::Copy(static_cast<ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), static_cast<size_t>(GetCount()));
  }

  PL_ALWAYS_INLINE void Swap(plBlobPtr<T>& other)
  {
    plMath::Swap(m_pPtr, other.m_pPtr);
    plMath::Swap(m_uiCount, other.m_uiCount);
  }

  using const_iterator = const T*;
  using const_reverse_iterator = const_reverse_pointer_iterator<T>;
  using iterator = T*;
  using reverse_iterator = reverse_pointer_iterator<T>;

private:
  PointerType m_pPtr = nullptr;
  plUInt64 m_uiCount = 0u;
};

//////////////////////////////////////////////////////////////////////////

using plByteBlobPtr = plBlobPtr<plUInt8>;
using plConstByteBlobPtr = plBlobPtr<const plUInt8>;

//////////////////////////////////////////////////////////////////////////

/// \brief Helper function to create plBlobPtr from a pointer of some type and a count.
template <typename T>
PL_ALWAYS_INLINE plBlobPtr<T> plMakeBlobPtr(T* pPtr, plUInt64 uiCount)
{
  return plBlobPtr<T>(pPtr, uiCount);
}

/// \brief Helper function to create plBlobPtr from a static array the a size known at compile-time.
template <typename T, plUInt64 N>
PL_ALWAYS_INLINE plBlobPtr<T> plMakeBlobPtr(T (&staticArray)[N])
{
  return plBlobPtr<T>(staticArray);
}

/// \brief Helper function to create plConstByteBlobPtr from a pointer of some type and a count.
template <typename T>
PL_ALWAYS_INLINE plConstByteBlobPtr plMakeByteBlobPtr(const T* pPtr, plUInt32 uiCount)
{
  return plConstByteBlobPtr(static_cast<const plUInt8*>(pPtr), uiCount * sizeof(T));
}

/// \brief Helper function to create plByteBlobPtr from a pointer of some type and a count.
template <typename T>
PL_ALWAYS_INLINE plByteBlobPtr plMakeByteBlobPtr(T* pPtr, plUInt32 uiCount)
{
  return plByteBlobPtr(reinterpret_cast<plUInt8*>(pPtr), uiCount * sizeof(T));
}

/// \brief Helper function to create plByteBlobPtr from a void pointer and a count.
PL_ALWAYS_INLINE plByteBlobPtr plMakeByteBlobPtr(void* pPtr, plUInt32 uiBytes)
{
  return plByteBlobPtr(reinterpret_cast<plUInt8*>(pPtr), uiBytes);
}

/// \brief Helper function to create plConstByteBlobPtr from a const void pointer and a count.
PL_ALWAYS_INLINE plConstByteBlobPtr plMakeByteBlobPtr(const void* pPtr, plUInt32 uiBytes)
{
  return plConstByteBlobPtr(static_cast<const plUInt8*>(pPtr), uiBytes);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
typename plBlobPtr<T>::iterator begin(plBlobPtr<T>& in_container)
{
  return in_container.GetPtr();
}

template <typename T>
typename plBlobPtr<T>::const_iterator begin(const plBlobPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename plBlobPtr<T>::const_iterator cbegin(const plBlobPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename plBlobPtr<T>::reverse_iterator rbegin(plBlobPtr<T>& in_container)
{
  return typename plBlobPtr<T>::reverse_iterator(in_container.GetPtr() + in_container.GetCount() - 1);
}

template <typename T>
typename plBlobPtr<T>::const_reverse_iterator rbegin(const plBlobPtr<T>& container)
{
  return typename plBlobPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename plBlobPtr<T>::const_reverse_iterator crbegin(const plBlobPtr<T>& container)
{
  return typename plBlobPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename plBlobPtr<T>::iterator end(plBlobPtr<T>& in_container)
{
  return in_container.GetPtr() + in_container.GetCount();
}

template <typename T>
typename plBlobPtr<T>::const_iterator end(const plBlobPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename plBlobPtr<T>::const_iterator cend(const plBlobPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename plBlobPtr<T>::reverse_iterator rend(plBlobPtr<T>& in_container)
{
  return typename plBlobPtr<T>::reverse_iterator(in_container.GetPtr() - 1);
}

template <typename T>
typename plBlobPtr<T>::const_reverse_iterator rend(const plBlobPtr<T>& container)
{
  return typename plBlobPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

template <typename T>
typename plBlobPtr<T>::const_reverse_iterator crend(const plBlobPtr<T>& container)
{
  return typename plBlobPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

/// \brief plBlob allows to store simple binary data larger than 4GB.
/// This storage class is used by plImage to allow processing of large textures for example.
/// In the current implementation the start of the allocated memory is guaranteed to be 64 byte aligned.
class PL_FOUNDATION_DLL plBlob
{
public:
  PL_DECLARE_MEM_RELOCATABLE_TYPE();

  /// \brief Default constructor. Does not allocate any memory.
  plBlob();

  /// \brief Move constructor. Moves the storage pointer from the other blob to this blob.
  plBlob(plBlob&& other);

  /// \brief Move assignment. Moves the storage pointer from the other blob to this blob.
  void operator=(plBlob&& rhs);

  /// \brief Default destructor. Will call Clear() to deallocate the memory.
  ~plBlob();

  /// \brief Sets the blob to the content of pSource.
  /// This will allocate the necessary memory if needed and then copy uiSize bytes from pSource.
  void SetFrom(const void* pSource, plUInt64 uiSize);

  /// \brief Deallocates the memory allocated by this instance.
  void Clear();

  /// \brief Allocates uiCount bytes for storage in this object. The bytes will have undefined content.
  void SetCountUninitialized(plUInt64 uiCount);

  /// \brief Convenience method to clear the content of the blob to all 0 bytes.
  void ZeroFill();

  /// \brief Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  template <typename T>
  plBlobPtr<T> GetBlobPtr()
  {
    return plBlobPtr<T>(static_cast<T*>(m_pStorage), m_uiSize);
  }

  /// \brief Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  template <typename T>
  plBlobPtr<const T> GetBlobPtr() const
  {
    return plBlobPtr<const T>(static_cast<T*>(m_pStorage), m_uiSize);
  }

  /// \brief Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  plByteBlobPtr GetByteBlobPtr() { return plByteBlobPtr(reinterpret_cast<plUInt8*>(m_pStorage), m_uiSize); }

  /// \brief Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  plConstByteBlobPtr GetByteBlobPtr() const { return plConstByteBlobPtr(reinterpret_cast<const plUInt8*>(m_pStorage), m_uiSize); }

private:
  void* m_pStorage = nullptr;
  plUInt64 m_uiSize = 0;
};

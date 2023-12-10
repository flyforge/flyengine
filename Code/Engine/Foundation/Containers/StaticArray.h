#pragma once

#include <Foundation/Containers/ArrayBase.h>

/// \brief Wraps a C-style array, which has a fixed size at compile-time, with a more convenient interface.
///
/// plStaticArray can be used to create a fixed size array, either on the stack or as a class member.
/// Additionally it allows to use that array as a 'cache', i.e. not all its elements need to be constructed.
/// As such it can be used whenever a fixed size array is sufficient, but a more powerful interface is desired,
/// and when the number of elements in an array is dynamic at run-time, but always capped at a fixed limit.
template <typename T, plUInt32 Capacity>
class plStaticArray : public plArrayBase<T, plStaticArray<T, Capacity>>
{
public:
  // Only if the stored type is either POD or relocatable the hybrid array itself is also relocatable.
  PLASMA_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T);

  /// \brief Creates an empty array.
  plStaticArray(); // [tested]

  /// \brief Creates a copy of the given array.
  plStaticArray(const plStaticArray<T, Capacity>& rhs); // [tested]

  /// \brief Creates a copy of the given array.
  template <plUInt32 OtherCapacity>
  plStaticArray(const plStaticArray<T, OtherCapacity>& rhs); // [tested]

  /// \brief Creates a copy of the given array.
  explicit plStaticArray(const plArrayPtr<const T>& rhs); // [tested]

  /// \brief Destroys all objects.
  ~plStaticArray(); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator=(const plStaticArray<T, Capacity>& rhs); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  template <plUInt32 OtherCapacity>
  void operator=(const plStaticArray<T, OtherCapacity>& rhs); // [tested]

  /// \brief Copies the data from some other contiguous array into this one.
  void operator=(const plArrayPtr<const T>& rhs); // [tested]

  /// \brief For the static array Reserve is a no-op. However the function checks if the requested capacity is below or equal to the static capacity.
  void Reserve(plUInt32 uiCapacity);

protected:
  T* GetElementsPtr();
  const T* GetElementsPtr() const;
  friend class plArrayBase<T, plStaticArray<T, Capacity>>;

private:
  T* GetStaticArray();
  const T* GetStaticArray() const;

  /// \brief The fixed size array.
  struct alignas(PLASMA_ALIGNMENT_OF(T))
  {
    plUInt8 m_Data[Capacity * sizeof(T)];
  };

  friend class plArrayBase<T, plStaticArray<T, Capacity>>;
};

// TODO PLASMA_CHECK_AT_COMPILETIME_MSG with a ',' in the expression does not work
// PLASMA_CHECK_AT_COMPILETIME_MSG(plGetTypeClass< plStaticArray<int, 4> >::value == 2, "static array is not memory relocatable");

#include <Foundation/Containers/Implementation/StaticArray_inl.h>

#pragma once

#include <Foundation/Basics.h>

/// \brief This class provides functions to do atomic operations.
///
/// Atomic operations are generally faster than mutexes, and should therefore be preferred whenever possible.
/// However only the operations in themselves are atomic, once you execute several of them in sequence,
/// the sequence will not be atomic.
/// Also atomic operations are a lot slower than non-atomic operations, thus you should not use them in code that
/// does not need to be thread-safe.
/// plAtomicInteger is built on top of plAtomicUtils and provides a more convenient interface to use atomic
/// integer instructions.
struct PL_FOUNDATION_DLL plAtomicUtils
{
  /// \brief Returns src as an atomic operation and returns its value.
  static plInt32 Read(const plInt32& iSrc); // [tested]

  /// \brief Returns src as an atomic operation and returns its value.
  static plInt64 Read(const plInt64& iSrc); // [tested]

  /// \brief Increments dest as an atomic operation and returns the new value.
  static plInt32 Increment(plInt32& ref_iDest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the new value.
  static plInt64 Increment(plInt64& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the new value.
  static plInt32 Decrement(plInt32& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the new value.
  static plInt64 Decrement(plInt64& ref_iDest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the old value.
  static plInt32 PostIncrement(plInt32& ref_iDest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the old value.
  static plInt64 PostIncrement(plInt64& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the old value.
  static plInt32 PostDecrement(plInt32& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the old value.
  static plInt64 PostDecrement(plInt64& ref_iDest); // [tested]

  /// \brief Adds value to dest as an atomic operation.
  static void Add(plInt32& ref_iDest, plInt32 value); // [tested]

  /// \brief Adds value to dest as an atomic operation.
  static void Add(plInt64& ref_iDest, plInt64 value); // [tested]

  /// \brief Performs an atomic bitwise AND on dest using value.
  static void And(plInt32& ref_iDest, plInt32 value); // [tested]

  /// \brief Performs an atomic bitwise AND on dest using value.
  static void And(plInt64& ref_iDest, plInt64 value); // [tested]

  /// \brief Performs an atomic bitwise OR on dest using value.
  static void Or(plInt32& ref_iDest, plInt32 value); // [tested]

  /// \brief Performs an atomic bitwise OR on dest using value.
  static void Or(plInt64& ref_iDest, plInt64 value); // [tested]

  /// \brief Performs an atomic bitwise XOR on dest using value.
  static void Xor(plInt32& ref_iDest, plInt32 value); // [tested]

  /// \brief Performs an atomic bitwise XOR on dest using value.
  static void Xor(plInt64& ref_iDest, plInt64 value); // [tested]

  /// \brief Performs an atomic min operation on dest using value.
  static void Min(plInt32& ref_iDest, plInt32 value); // [tested]

  /// \brief Performs an atomic min operation on dest using value.
  static void Min(plInt64& ref_iDest, plInt64 value); // [tested]

  /// \brief Performs an atomic max operation on dest using value.
  static void Max(plInt32& ref_iDest, plInt32 value); // [tested]

  /// \brief Performs an atomic max operation on dest using value.
  static void Max(plInt64& ref_iDest, plInt64 value); // [tested]

  /// \brief Sets dest to value as an atomic operation and returns the original value of dest.
  static plInt32 Set(plInt32& ref_iDest, plInt32 value); // [tested]

  /// \brief Sets dest to value as an atomic operation and returns the original value of dest.
  static plInt64 Set(plInt64& ref_iDest, plInt64 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(plInt32& ref_iDest, plInt32 iExpected, plInt32 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(plInt64& ref_iDest, plInt64 iExpected, plInt64 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(void** pDest, void* pExpected, void* value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value*. Otherwise *dest* will not be modified. Always returns the value
  /// of *dest* before the modification.
  static plInt32 CompareAndSwap(plInt32& ref_iDest, plInt32 iExpected, plInt32 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value*. Otherwise *dest* will not be modified. Always returns the value
  /// of *dest* before the modification.
  static plInt64 CompareAndSwap(plInt64& ref_iDest, plInt64 iExpected, plInt64 value); // [tested]
};

// Include inline file
#if PL_ENABLED(PL_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/AtomicUtils_win.h>
#elif PL_ENABLED(PL_PLATFORM_OSX) || PL_ENABLED(PL_PLATFORM_LINUX) || PL_ENABLED(PL_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/AtomicUtils_posix.h>
#else
#  error "Atomics are not implemented on current platform"
#endif

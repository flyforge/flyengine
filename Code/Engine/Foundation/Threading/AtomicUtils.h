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
struct PLASMA_FOUNDATION_DLL plAtomicUtils
{
  /// \brief Returns src as an atomic operation and returns its value.
  static plInt32 Read(volatile const plInt32& iSrc); // [tested]

  /// \brief Returns src as an atomic operation and returns its value.
  static plInt64 Read(volatile const plInt64& iSrc); // [tested]

  /// \brief Increments dest as an atomic operation and returns the new value.
  static plInt32 Increment(volatile plInt32& ref_iDest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the new value.
  static plInt64 Increment(volatile plInt64& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the new value.
  static plInt32 Decrement(volatile plInt32& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the new value.
  static plInt64 Decrement(volatile plInt64& ref_iDest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the old value.
  static plInt32 PostIncrement(volatile plInt32& ref_iDest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the old value.
  static plInt64 PostIncrement(volatile plInt64& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the old value.
  static plInt32 PostDecrement(volatile plInt32& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the old value.
  static plInt64 PostDecrement(volatile plInt64& ref_iDest); // [tested]

  /// \brief Adds value to dest as an atomic operation.
  static void Add(volatile plInt32& ref_iDest, plInt32 value); // [tested]

  /// \brief Adds value to dest as an atomic operation.
  static void Add(volatile plInt64& ref_iDest, plInt64 value); // [tested]

  /// \brief Performs an atomic bitwise AND on dest using value.
  static void And(volatile plInt32& ref_iDest, plInt32 value); // [tested]

  /// \brief Performs an atomic bitwise AND on dest using value.
  static void And(volatile plInt64& ref_iDest, plInt64 value); // [tested]

  /// \brief Performs an atomic bitwise OR on dest using value.
  static void Or(volatile plInt32& ref_iDest, plInt32 value); // [tested]

  /// \brief Performs an atomic bitwise OR on dest using value.
  static void Or(volatile plInt64& ref_iDest, plInt64 value); // [tested]

  /// \brief Performs an atomic bitwise XOR on dest using value.
  static void Xor(volatile plInt32& ref_iDest, plInt32 value); // [tested]

  /// \brief Performs an atomic bitwise XOR on dest using value.
  static void Xor(volatile plInt64& ref_iDest, plInt64 value); // [tested]

  /// \brief Performs an atomic min operation on dest using value.
  static void Min(volatile plInt32& ref_iDest, plInt32 value); // [tested]

  /// \brief Performs an atomic min operation on dest using value.
  static void Min(volatile plInt64& ref_iDest, plInt64 value); // [tested]

  /// \brief Performs an atomic max operation on dest using value.
  static void Max(volatile plInt32& ref_iDest, plInt32 value); // [tested]

  /// \brief Performs an atomic max operation on dest using value.
  static void Max(volatile plInt64& ref_iDest, plInt64 value); // [tested]

  /// \brief Sets dest to value as an atomic operation and returns the original value of dest.
  static plInt32 Set(volatile plInt32& ref_iDest, plInt32 value); // [tested]

  /// \brief Sets dest to value as an atomic operation and returns the original value of dest.
  static plInt64 Set(volatile plInt64& ref_iDest, plInt64 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(volatile plInt32& ref_iDest, plInt32 iExpected, plInt32 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(volatile plInt64& ref_iDest, plInt64 iExpected, plInt64 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(void** volatile pDest, void* pExpected, void* value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value*. Otherwise *dest* will not be modified. Always returns the value
  /// of *dest* before the modification.
  static plInt32 CompareAndSwap(volatile plInt32& ref_iDest, plInt32 iExpected, plInt32 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value*. Otherwise *dest* will not be modified. Always returns the value
  /// of *dest* before the modification.
  static plInt64 CompareAndSwap(volatile plInt64& ref_iDest, plInt64 iExpected, plInt64 value); // [tested]
};

// Include inline file
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/AtomicUtils_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/AtomicUtils_posix.h>
#else
#  error "Atomics are not implemented on current platform"
#endif

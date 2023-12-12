#ifdef PLASMA_ATOMICUTLS_POSIX_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define PLASMA_ATOMICUTLS_POSIX_INL_H_INCLUDED


#include <Foundation/Math/Math.h>

PLASMA_ALWAYS_INLINE plInt32 plAtomicUtils::Read(volatile const plInt32& src)
{
  return __sync_fetch_and_or(const_cast<volatile plInt32*>(&src), 0);
}

PLASMA_ALWAYS_INLINE plInt64 plAtomicUtils::Read(volatile const plInt64& src)
{
  return __sync_fetch_and_or_8(const_cast<volatile plInt64*>(&src), 0);
}

PLASMA_ALWAYS_INLINE plInt32 plAtomicUtils::Increment(volatile plInt32& dest)
{
  return __sync_add_and_fetch(&dest, 1);
}

PLASMA_ALWAYS_INLINE plInt64 plAtomicUtils::Increment(volatile plInt64& dest)
{
  return __sync_add_and_fetch_8(&dest, 1);
}


PLASMA_ALWAYS_INLINE plInt32 plAtomicUtils::Decrement(volatile plInt32& dest)
{
  return __sync_sub_and_fetch(&dest, 1);
}

PLASMA_ALWAYS_INLINE plInt64 plAtomicUtils::Decrement(volatile plInt64& dest)
{
  return __sync_sub_and_fetch_8(&dest, 1);
}

PLASMA_ALWAYS_INLINE plInt32 plAtomicUtils::PostIncrement(volatile plInt32& dest)
{
  return __sync_fetch_and_add(&dest, 1);
}

PLASMA_ALWAYS_INLINE plInt64 plAtomicUtils::PostIncrement(volatile plInt64& dest)
{
  return __sync_fetch_and_add_8(&dest, 1);
}


PLASMA_ALWAYS_INLINE plInt32 plAtomicUtils::PostDecrement(volatile plInt32& dest)
{
  return __sync_fetch_and_sub(&dest, 1);
}

PLASMA_ALWAYS_INLINE plInt64 plAtomicUtils::PostDecrement(volatile plInt64& dest)
{
  return __sync_fetch_and_sub_8(&dest, 1);
}

PLASMA_ALWAYS_INLINE void plAtomicUtils::Add(volatile plInt32& dest, plInt32 value)
{
  __sync_fetch_and_add(&dest, value);
}

PLASMA_ALWAYS_INLINE void plAtomicUtils::Add(volatile plInt64& dest, plInt64 value)
{
  __sync_fetch_and_add_8(&dest, value);
}


PLASMA_ALWAYS_INLINE void plAtomicUtils::And(volatile plInt32& dest, plInt32 value)
{
  __sync_fetch_and_and(&dest, value);
}

PLASMA_ALWAYS_INLINE void plAtomicUtils::And(volatile plInt64& dest, plInt64 value)
{
  __sync_fetch_and_and_8(&dest, value);
}


PLASMA_ALWAYS_INLINE void plAtomicUtils::Or(volatile plInt32& dest, plInt32 value)
{
  __sync_fetch_and_or(&dest, value);
}

PLASMA_ALWAYS_INLINE void plAtomicUtils::Or(volatile plInt64& dest, plInt64 value)
{
  __sync_fetch_and_or_8(&dest, value);
}


PLASMA_ALWAYS_INLINE void plAtomicUtils::Xor(volatile plInt32& dest, plInt32 value)
{
  __sync_fetch_and_xor(&dest, value);
}

PLASMA_ALWAYS_INLINE void plAtomicUtils::Xor(volatile plInt64& dest, plInt64 value)
{
  __sync_fetch_and_xor_8(&dest, value);
}


PLASMA_FORCE_INLINE void plAtomicUtils::Min(volatile plInt32& dest, plInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    plInt32 iOldValue = dest;
    plInt32 iNewValue = plMath::Min(iOldValue, value);

    if (__sync_bool_compare_and_swap(&dest, iOldValue, iNewValue))
      break;
  }
}

PLASMA_FORCE_INLINE void plAtomicUtils::Min(volatile plInt64& dest, plInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    plInt64 iOldValue = dest;
    plInt64 iNewValue = plMath::Min(iOldValue, value);

    if (__sync_bool_compare_and_swap_8(&dest, iOldValue, iNewValue))
      break;
  }
}


PLASMA_FORCE_INLINE void plAtomicUtils::Max(volatile plInt32& dest, plInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    plInt32 iOldValue = dest;
    plInt32 iNewValue = plMath::Max(iOldValue, value);

    if (__sync_bool_compare_and_swap(&dest, iOldValue, iNewValue))
      break;
  }
}

PLASMA_FORCE_INLINE void plAtomicUtils::Max(volatile plInt64& dest, plInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    plInt64 iOldValue = dest;
    plInt64 iNewValue = plMath::Max(iOldValue, value);

    if (__sync_bool_compare_and_swap_8(&dest, iOldValue, iNewValue))
      break;
  }
}


PLASMA_ALWAYS_INLINE plInt32 plAtomicUtils::Set(volatile plInt32& dest, plInt32 value)
{
  return __sync_lock_test_and_set(&dest, value);
}

PLASMA_ALWAYS_INLINE plInt64 plAtomicUtils::Set(volatile plInt64& dest, plInt64 value)
{
  return __sync_lock_test_and_set_8(&dest, value);
}


PLASMA_ALWAYS_INLINE bool plAtomicUtils::TestAndSet(volatile plInt32& dest, plInt32 expected, plInt32 value)
{
  return __sync_bool_compare_and_swap(&dest, expected, value);
}

PLASMA_ALWAYS_INLINE bool plAtomicUtils::TestAndSet(volatile plInt64& dest, plInt64 expected, plInt64 value)
{
  return __sync_bool_compare_and_swap_8(&dest, expected, value);
}

PLASMA_ALWAYS_INLINE bool plAtomicUtils::TestAndSet(void** volatile dest, void* expected, void* value)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_64BIT)
  plUInt64* puiTemp = reinterpret_cast<plUInt64*>(dest);
  return __sync_bool_compare_and_swap(puiTemp, reinterpret_cast<plUInt64>(expected), reinterpret_cast<plUInt64>(value));
#else
  plUInt32* puiTemp = reinterpret_cast<plUInt32*>(dest);
  return __sync_bool_compare_and_swap(puiTemp, reinterpret_cast<plUInt32>(expected), reinterpret_cast<plUInt32>(value));
#endif
}

PLASMA_ALWAYS_INLINE plInt32 plAtomicUtils::CompareAndSwap(volatile plInt32& dest, plInt32 expected, plInt32 value)
{
  return __sync_val_compare_and_swap(&dest, expected, value);
}

PLASMA_ALWAYS_INLINE plInt64 plAtomicUtils::CompareAndSwap(volatile plInt64& dest, plInt64 expected, plInt64 value)
{
  return __sync_val_compare_and_swap_8(&dest, expected, value);
}

#ifdef PL_ATOMICUTLS_POSIX_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define PL_ATOMICUTLS_POSIX_INL_H_INCLUDED


#include <Foundation/Math/Math.h>

PL_ALWAYS_INLINE plInt32 plAtomicUtils::Read(const plInt32& src)
{
  return __sync_fetch_and_or(const_cast<plInt32*>(&src), 0);
}

PL_ALWAYS_INLINE plInt64 plAtomicUtils::Read(const plInt64& src)
{
  return __sync_fetch_and_or_8(const_cast<plInt64*>(&src), 0);
}

PL_ALWAYS_INLINE plInt32 plAtomicUtils::Increment(plInt32& dest)
{
  return __sync_add_and_fetch(&dest, 1);
}

PL_ALWAYS_INLINE plInt64 plAtomicUtils::Increment(plInt64& dest)
{
  return __sync_add_and_fetch_8(&dest, 1);
}


PL_ALWAYS_INLINE plInt32 plAtomicUtils::Decrement(plInt32& dest)
{
  return __sync_sub_and_fetch(&dest, 1);
}

PL_ALWAYS_INLINE plInt64 plAtomicUtils::Decrement(plInt64& dest)
{
  return __sync_sub_and_fetch_8(&dest, 1);
}

PL_ALWAYS_INLINE plInt32 plAtomicUtils::PostIncrement(plInt32& dest)
{
  return __sync_fetch_and_add(&dest, 1);
}

PL_ALWAYS_INLINE plInt64 plAtomicUtils::PostIncrement(plInt64& dest)
{
  return __sync_fetch_and_add_8(&dest, 1);
}


PL_ALWAYS_INLINE plInt32 plAtomicUtils::PostDecrement(plInt32& dest)
{
  return __sync_fetch_and_sub(&dest, 1);
}

PL_ALWAYS_INLINE plInt64 plAtomicUtils::PostDecrement(plInt64& dest)
{
  return __sync_fetch_and_sub_8(&dest, 1);
}

PL_ALWAYS_INLINE void plAtomicUtils::Add(plInt32& dest, plInt32 value)
{
  __sync_fetch_and_add(&dest, value);
}

PL_ALWAYS_INLINE void plAtomicUtils::Add(plInt64& dest, plInt64 value)
{
  __sync_fetch_and_add_8(&dest, value);
}


PL_ALWAYS_INLINE void plAtomicUtils::And(plInt32& dest, plInt32 value)
{
  __sync_fetch_and_and(&dest, value);
}

PL_ALWAYS_INLINE void plAtomicUtils::And(plInt64& dest, plInt64 value)
{
  __sync_fetch_and_and_8(&dest, value);
}


PL_ALWAYS_INLINE void plAtomicUtils::Or(plInt32& dest, plInt32 value)
{
  __sync_fetch_and_or(&dest, value);
}

PL_ALWAYS_INLINE void plAtomicUtils::Or(plInt64& dest, plInt64 value)
{
  __sync_fetch_and_or_8(&dest, value);
}


PL_ALWAYS_INLINE void plAtomicUtils::Xor(plInt32& dest, plInt32 value)
{
  __sync_fetch_and_xor(&dest, value);
}

PL_ALWAYS_INLINE void plAtomicUtils::Xor(plInt64& dest, plInt64 value)
{
  __sync_fetch_and_xor_8(&dest, value);
}


PL_FORCE_INLINE void plAtomicUtils::Min(plInt32& dest, plInt32 value)
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

PL_FORCE_INLINE void plAtomicUtils::Min(plInt64& dest, plInt64 value)
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


PL_FORCE_INLINE void plAtomicUtils::Max(plInt32& dest, plInt32 value)
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

PL_FORCE_INLINE void plAtomicUtils::Max(plInt64& dest, plInt64 value)
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


PL_ALWAYS_INLINE plInt32 plAtomicUtils::Set(plInt32& dest, plInt32 value)
{
  return __sync_lock_test_and_set(&dest, value);
}

PL_ALWAYS_INLINE plInt64 plAtomicUtils::Set(plInt64& dest, plInt64 value)
{
  return __sync_lock_test_and_set_8(&dest, value);
}


PL_ALWAYS_INLINE bool plAtomicUtils::TestAndSet(plInt32& dest, plInt32 expected, plInt32 value)
{
  return __sync_bool_compare_and_swap(&dest, expected, value);
}

PL_ALWAYS_INLINE bool plAtomicUtils::TestAndSet(plInt64& dest, plInt64 expected, plInt64 value)
{
  return __sync_bool_compare_and_swap_8(&dest, expected, value);
}

PL_ALWAYS_INLINE bool plAtomicUtils::TestAndSet(void** dest, void* expected, void* value)
{
#if PL_ENABLED(PL_PLATFORM_64BIT)
  plUInt64* puiTemp = reinterpret_cast<plUInt64*>(dest);
  return __sync_bool_compare_and_swap(puiTemp, reinterpret_cast<plUInt64>(expected), reinterpret_cast<plUInt64>(value));
#else
  plUInt32* puiTemp = reinterpret_cast<plUInt32*>(dest);
  return __sync_bool_compare_and_swap(puiTemp, reinterpret_cast<plUInt32>(expected), reinterpret_cast<plUInt32>(value));
#endif
}

PL_ALWAYS_INLINE plInt32 plAtomicUtils::CompareAndSwap(plInt32& dest, plInt32 expected, plInt32 value)
{
  return __sync_val_compare_and_swap(&dest, expected, value);
}

PL_ALWAYS_INLINE plInt64 plAtomicUtils::CompareAndSwap(plInt64& dest, plInt64 expected, plInt64 value)
{
  return __sync_val_compare_and_swap_8(&dest, expected, value);
}

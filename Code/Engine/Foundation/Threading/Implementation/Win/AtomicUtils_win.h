#ifdef PLASMA_ATOMICUTLS_WIN_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define PLASMA_ATOMICUTLS_WIN_INL_H_INCLUDED

#include <intrin.h>

PLASMA_ALWAYS_INLINE plInt32 plAtomicUtils::Read(volatile const plInt32& iSrc)
{
  return _InterlockedOr((volatile long*)(&iSrc), 0);
}

PLASMA_ALWAYS_INLINE plInt64 plAtomicUtils::Read(volatile const plInt64& iSrc)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = iSrc;
  } while (_InterlockedCompareExchange64(const_cast<volatile plInt64*>(&iSrc), old, old) != old);
  return old;
#else
  return _InterlockedOr64(const_cast<volatile plInt64*>(&iSrc), 0);
#endif
}

PLASMA_ALWAYS_INLINE plInt32 plAtomicUtils::Increment(volatile plInt32& ref_iDest)
{
  return _InterlockedIncrement(reinterpret_cast<volatile long*>(&ref_iDest));
}

PLASMA_ALWAYS_INLINE plInt64 plAtomicUtils::Increment(volatile plInt64& ref_iDest)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old + 1, old) != old);
  return old + 1;
#else
  return _InterlockedIncrement64(&ref_iDest);
#endif
}

PLASMA_ALWAYS_INLINE plInt32 plAtomicUtils::Decrement(volatile plInt32& ref_iDest)
{
  return _InterlockedDecrement(reinterpret_cast<volatile long*>(&ref_iDest));
}

PLASMA_ALWAYS_INLINE plInt64 plAtomicUtils::Decrement(volatile plInt64& ref_iDest)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old - 1, old) != old);
  return old - 1;
#else
  return _InterlockedDecrement64(&ref_iDest);
#endif
}

PLASMA_ALWAYS_INLINE plInt32 plAtomicUtils::PostIncrement(volatile plInt32& ref_iDest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_iDest), 1);
}

PLASMA_ALWAYS_INLINE plInt64 plAtomicUtils::PostIncrement(volatile plInt64& ref_iDest)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old + 1, old) != old);
  return old;
#else
  return _InterlockedExchangeAdd64(&ref_iDest, 1);
#endif
}

PLASMA_ALWAYS_INLINE plInt32 plAtomicUtils::PostDecrement(volatile plInt32& ref_iDest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_iDest), -1);
}

PLASMA_ALWAYS_INLINE plInt64 plAtomicUtils::PostDecrement(volatile plInt64& ref_iDest)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old - 1, old) != old);
  return old;
#else
  return _InterlockedExchangeAdd64(&ref_iDest, -1);
#endif
}

PLASMA_ALWAYS_INLINE void plAtomicUtils::Add(volatile plInt32& ref_iDest, plInt32 value)
{
  _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

PLASMA_ALWAYS_INLINE void plAtomicUtils::Add(volatile plInt64& ref_iDest, plInt64 value)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old + value, old) != old);
#else
  _InterlockedExchangeAdd64(&ref_iDest, value);
#endif
}


PLASMA_ALWAYS_INLINE void plAtomicUtils::And(volatile plInt32& ref_iDest, plInt32 value)
{
  _InterlockedAnd(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

PLASMA_ALWAYS_INLINE void plAtomicUtils::And(volatile plInt64& ref_iDest, plInt64 value)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old & value, old) != old);
#else
  _InterlockedAnd64(&ref_iDest, value);
#endif
}


PLASMA_ALWAYS_INLINE void plAtomicUtils::Or(volatile plInt32& ref_iDest, plInt32 value)
{
  _InterlockedOr(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

PLASMA_ALWAYS_INLINE void plAtomicUtils::Or(volatile plInt64& ref_iDest, plInt64 value)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old | value, old) != old);
#else
  _InterlockedOr64(&ref_iDest, value);
#endif
}


PLASMA_ALWAYS_INLINE void plAtomicUtils::Xor(volatile plInt32& ref_iDest, plInt32 value)
{
  _InterlockedXor(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

PLASMA_ALWAYS_INLINE void plAtomicUtils::Xor(volatile plInt64& ref_iDest, plInt64 value)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old ^ value, old) != old);
#else
  _InterlockedXor64(&ref_iDest, value);
#endif
}


inline void plAtomicUtils::Min(volatile plInt32& ref_iDest, plInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    plInt32 iOldValue = ref_iDest;
    plInt32 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ref_iDest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void plAtomicUtils::Min(volatile plInt64& ref_iDest, plInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    plInt64 iOldValue = ref_iDest;
    plInt64 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (_InterlockedCompareExchange64(&ref_iDest, iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void plAtomicUtils::Max(volatile plInt32& ref_iDest, plInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    plInt32 iOldValue = ref_iDest;
    plInt32 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ref_iDest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void plAtomicUtils::Max(volatile plInt64& ref_iDest, plInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    plInt64 iOldValue = ref_iDest;
    plInt64 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (_InterlockedCompareExchange64(&ref_iDest, iNewValue, iOldValue) == iOldValue)
      break;
  }
}


inline plInt32 plAtomicUtils::Set(volatile plInt32& ref_iDest, plInt32 value)
{
  return _InterlockedExchange(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

PLASMA_ALWAYS_INLINE plInt64 plAtomicUtils::Set(volatile plInt64& ref_iDest, plInt64 value)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, value, old) != old);
  return old;
#else
  return _InterlockedExchange64(&ref_iDest, value);
#endif
}


PLASMA_ALWAYS_INLINE bool plAtomicUtils::TestAndSet(volatile plInt32& ref_iDest, plInt32 iExpected, plInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ref_iDest), value, iExpected) == iExpected;
}

PLASMA_ALWAYS_INLINE bool plAtomicUtils::TestAndSet(volatile plInt64& ref_iDest, plInt64 iExpected, plInt64 value)
{
  return _InterlockedCompareExchange64(&ref_iDest, value, iExpected) == iExpected;
}

PLASMA_ALWAYS_INLINE bool plAtomicUtils::TestAndSet(void** volatile pDest, void* pExpected, void* value)
{
  return _InterlockedCompareExchangePointer(pDest, value, pExpected) == pExpected;
}

PLASMA_ALWAYS_INLINE plInt32 plAtomicUtils::CompareAndSwap(volatile plInt32& ref_iDest, plInt32 iExpected, plInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ref_iDest), value, iExpected);
}

PLASMA_ALWAYS_INLINE plInt64 plAtomicUtils::CompareAndSwap(volatile plInt64& ref_iDest, plInt64 iExpected, plInt64 value)
{
  return _InterlockedCompareExchange64(&ref_iDest, value, iExpected);
}

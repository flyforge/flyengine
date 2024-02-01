#ifdef PL_ATOMICUTLS_WIN_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define PL_ATOMICUTLS_WIN_INL_H_INCLUDED

#include <intrin.h>

PL_ALWAYS_INLINE plInt32 plAtomicUtils::Read(const plInt32& iSrc)
{
  return _InterlockedOr((long*)(&iSrc), 0);
}

PL_ALWAYS_INLINE plInt64 plAtomicUtils::Read(const plInt64& iSrc)
{
#if PL_ENABLED(PL_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = iSrc;
  } while (_InterlockedCompareExchange64(const_cast<plInt64*>(&iSrc), old, old) != old);
  return old;
#else
  return _InterlockedOr64(const_cast<plInt64*>(&iSrc), 0);
#endif
}

PL_ALWAYS_INLINE plInt32 plAtomicUtils::Increment(plInt32& ref_iDest)
{
  return _InterlockedIncrement(reinterpret_cast<long*>(&ref_iDest));
}

PL_ALWAYS_INLINE plInt64 plAtomicUtils::Increment(plInt64& ref_iDest)
{
#if PL_ENABLED(PL_PLATFORM_32BIT)
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

PL_ALWAYS_INLINE plInt32 plAtomicUtils::Decrement(plInt32& ref_iDest)
{
  return _InterlockedDecrement(reinterpret_cast<long*>(&ref_iDest));
}

PL_ALWAYS_INLINE plInt64 plAtomicUtils::Decrement(plInt64& ref_iDest)
{
#if PL_ENABLED(PL_PLATFORM_32BIT)
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

PL_ALWAYS_INLINE plInt32 plAtomicUtils::PostIncrement(plInt32& ref_iDest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<long*>(&ref_iDest), 1);
}

PL_ALWAYS_INLINE plInt64 plAtomicUtils::PostIncrement(plInt64& ref_iDest)
{
#if PL_ENABLED(PL_PLATFORM_32BIT)
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

PL_ALWAYS_INLINE plInt32 plAtomicUtils::PostDecrement(plInt32& ref_iDest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<long*>(&ref_iDest), -1);
}

PL_ALWAYS_INLINE plInt64 plAtomicUtils::PostDecrement(plInt64& ref_iDest)
{
#if PL_ENABLED(PL_PLATFORM_32BIT)
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

PL_ALWAYS_INLINE void plAtomicUtils::Add(plInt32& ref_iDest, plInt32 value)
{
  _InterlockedExchangeAdd(reinterpret_cast<long*>(&ref_iDest), value);
}

PL_ALWAYS_INLINE void plAtomicUtils::Add(plInt64& ref_iDest, plInt64 value)
{
#if PL_ENABLED(PL_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old + value, old) != old);
#else
  _InterlockedExchangeAdd64(&ref_iDest, value);
#endif
}


PL_ALWAYS_INLINE void plAtomicUtils::And(plInt32& ref_iDest, plInt32 value)
{
  _InterlockedAnd(reinterpret_cast<long*>(&ref_iDest), value);
}

PL_ALWAYS_INLINE void plAtomicUtils::And(plInt64& ref_iDest, plInt64 value)
{
#if PL_ENABLED(PL_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old & value, old) != old);
#else
  _InterlockedAnd64(&ref_iDest, value);
#endif
}


PL_ALWAYS_INLINE void plAtomicUtils::Or(plInt32& ref_iDest, plInt32 value)
{
  _InterlockedOr(reinterpret_cast<long*>(&ref_iDest), value);
}

PL_ALWAYS_INLINE void plAtomicUtils::Or(plInt64& ref_iDest, plInt64 value)
{
#if PL_ENABLED(PL_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old | value, old) != old);
#else
  _InterlockedOr64(&ref_iDest, value);
#endif
}


PL_ALWAYS_INLINE void plAtomicUtils::Xor(plInt32& ref_iDest, plInt32 value)
{
  _InterlockedXor(reinterpret_cast<long*>(&ref_iDest), value);
}

PL_ALWAYS_INLINE void plAtomicUtils::Xor(plInt64& ref_iDest, plInt64 value)
{
#if PL_ENABLED(PL_PLATFORM_32BIT)
  plInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old ^ value, old) != old);
#else
  _InterlockedXor64(&ref_iDest, value);
#endif
}


inline void plAtomicUtils::Min(plInt32& ref_iDest, plInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    plInt32 iOldValue = ref_iDest;
    plInt32 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void plAtomicUtils::Min(plInt64& ref_iDest, plInt64 value)
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

inline void plAtomicUtils::Max(plInt32& ref_iDest, plInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    plInt32 iOldValue = ref_iDest;
    plInt32 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void plAtomicUtils::Max(plInt64& ref_iDest, plInt64 value)
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


inline plInt32 plAtomicUtils::Set(plInt32& ref_iDest, plInt32 value)
{
  return _InterlockedExchange(reinterpret_cast<long*>(&ref_iDest), value);
}

PL_ALWAYS_INLINE plInt64 plAtomicUtils::Set(plInt64& ref_iDest, plInt64 value)
{
#if PL_ENABLED(PL_PLATFORM_32BIT)
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


PL_ALWAYS_INLINE bool plAtomicUtils::TestAndSet(plInt32& ref_iDest, plInt32 iExpected, plInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), value, iExpected) == iExpected;
}

PL_ALWAYS_INLINE bool plAtomicUtils::TestAndSet(plInt64& ref_iDest, plInt64 iExpected, plInt64 value)
{
  return _InterlockedCompareExchange64(&ref_iDest, value, iExpected) == iExpected;
}

PL_ALWAYS_INLINE bool plAtomicUtils::TestAndSet(void** pDest, void* pExpected, void* value)
{
  return _InterlockedCompareExchangePointer(pDest, value, pExpected) == pExpected;
}

PL_ALWAYS_INLINE plInt32 plAtomicUtils::CompareAndSwap(plInt32& ref_iDest, plInt32 iExpected, plInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), value, iExpected);
}

PL_ALWAYS_INLINE plInt64 plAtomicUtils::CompareAndSwap(plInt64& ref_iDest, plInt64 iExpected, plInt64 value)
{
  return _InterlockedCompareExchange64(&ref_iDest, value, iExpected);
}

#pragma once

/// \brief Manages a lock (e.g. a mutex) and ensures that it is properly released as the lock object goes out of scope.
template <typename T>
class plLock
{
public:
  PL_ALWAYS_INLINE explicit plLock(T& ref_lock)
    : m_Lock(ref_lock)
  {
    m_Lock.Lock();
  }

  PL_ALWAYS_INLINE ~plLock() { m_Lock.Unlock(); }

private:
  plLock();
  plLock(const plLock<T>& rhs);
  void operator=(const plLock<T>& rhs);

  T& m_Lock;
};

/// \brief Shortcut for plLock<Type> l(lock)
#define PL_LOCK(lock) plLock<decltype(lock)> PL_CONCAT(l_, PL_SOURCE_LINE)(lock)

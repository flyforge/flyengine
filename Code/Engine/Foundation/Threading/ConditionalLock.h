#pragma once

/// \brief Manages a lock (e.g. a mutex) and ensures that it is properly released as the lock object goes out of scope. The lock/unlock will
/// only be done if the boolean condition is satisfied at scope creation time.
template <typename T>
class plConditionalLock
{
public:
  PLASMA_ALWAYS_INLINE explicit plConditionalLock(T& lock, bool bCondition)
    : m_lock(lock)
    , m_bCondition(bCondition)
  {
    if (m_bCondition)
    {
      m_lock.Lock();
    }
  }

  PLASMA_ALWAYS_INLINE ~plConditionalLock()
  {
    if (m_bCondition)
    {
      m_lock.Unlock();
    }
  }

private:
  plConditionalLock();
  plConditionalLock(const plConditionalLock<T>& rhs);
  void operator=(const plConditionalLock<T>& rhs);

  T& m_lock;
  bool m_bCondition;
};

#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/ConditionVariable.h>

void plConditionVariable::Lock()
{
  m_Mutex.Lock();
  ++m_iLockCount;
}

plResult plConditionVariable::TryLock()
{
  if (m_Mutex.TryLock().Succeeded())
  {
    ++m_iLockCount;
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

void plConditionVariable::Unlock()
{
  PL_ASSERT_DEV(m_iLockCount > 0, "Cannot unlock a thread-signal that was not locked before.");
  --m_iLockCount;
  m_Mutex.Unlock();
}



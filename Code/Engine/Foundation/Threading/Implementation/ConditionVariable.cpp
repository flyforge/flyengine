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
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

void plConditionVariable::Unlock()
{
  PLASMA_ASSERT_DEV(m_iLockCount > 0, "Cannot unlock a thread-signal that was not locked before.");
  --m_iLockCount;
  m_Mutex.Unlock();
}

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/ConditionVariable_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/ConditionVariable_posix.h>
#else
#  error "Unsupported Platform."
#endif



PLASMA_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ConditionVariable);

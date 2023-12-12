#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <errno.h>
#include <pthread.h>

plConditionVariable::plConditionVariable()
{
  pthread_cond_init(&m_Data.m_ConditionVariable, nullptr);
}

plConditionVariable::~plConditionVariable()
{
  PLASMA_ASSERT_DEV(m_iLockCount == 0, "Thread-signal must be unlocked during destruction.");

  pthread_cond_destroy(&m_Data.m_ConditionVariable);
}

void plConditionVariable::SignalOne()
{
  pthread_cond_signal(&m_Data.m_ConditionVariable);
}

void plConditionVariable::SignalAll()
{
  pthread_cond_broadcast(&m_Data.m_ConditionVariable);
}

void plConditionVariable::UnlockWaitForSignalAndLock() const
{
  PLASMA_ASSERT_DEV(m_iLockCount > 0, "plConditionVariable must be locked when calling UnlockWaitForSignalAndLock.");

  pthread_cond_wait(&m_Data.m_ConditionVariable, &m_Mutex.GetMutexHandle());
}

plConditionVariable::WaitResult plConditionVariable::UnlockWaitForSignalAndLock(plTime timeout) const
{
  PLASMA_ASSERT_DEV(m_iLockCount > 0, "plConditionVariable must be locked when calling UnlockWaitForSignalAndLock.");

  // inside the lock
  --m_iLockCount;

  timeval now;
  gettimeofday(&now, nullptr);

  // pthread_cond_timedwait needs an absolute time value, so compute it from the current time.
  struct timespec timeToWait;

  const plInt64 iNanoSecondsPerSecond = 1000000000LL;
  const plInt64 iMicroSecondsPerNanoSecond = 1000LL;

  plInt64 endTime = now.tv_sec * iNanoSecondsPerSecond + now.tv_usec * iMicroSecondsPerNanoSecond + static_cast<plInt64>(timeout.GetNanoseconds());

  timeToWait.tv_sec = endTime / iNanoSecondsPerSecond;
  timeToWait.tv_nsec = endTime % iNanoSecondsPerSecond;

  if (pthread_cond_timedwait(&m_Data.m_ConditionVariable, &m_Mutex.GetMutexHandle(), &timeToWait) == ETIMEDOUT)
  {
    // inside the lock
    ++m_iLockCount;
    return WaitResult::Timeout;
  }

  // inside the lock
  ++m_iLockCount;
  return WaitResult::Signaled;
}

#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

// Posix implementation of thread helper functions

#include <pthread.h>

static pthread_t g_MainThread = (pthread_t)0;

void plThreadUtils::Initialize()
{
  g_MainThread = pthread_self();
}

void plThreadUtils::YieldTimeSlice()
{
  sched_yield();
}

void plThreadUtils::YieldHardwareThread()
{
  // No equivalent to mm_pause on linux
}

void plThreadUtils::Sleep(const plTime& duration)
{
  timespec SleepTime;
  SleepTime.tv_sec = duration.GetSeconds();
  SleepTime.tv_nsec = ((plInt64)duration.GetMilliseconds() * 1000000LL) % 1000000000LL;
  nanosleep(&SleepTime, nullptr);
}

// plThreadHandle plThreadUtils::GetCurrentThreadHandle()
//{
//  return pthread_self();
//}

plThreadID plThreadUtils::GetCurrentThreadID()
{
  return pthread_self();
}

bool plThreadUtils::IsMainThread()
{
  return pthread_self() == g_MainThread;
}

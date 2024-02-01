#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)

#  include <Foundation/Threading/ThreadUtils.h>

static DWORD g_uiMainThreadID = 0xFFFFFFFF;

void plThreadUtils::Initialize()
{
  g_uiMainThreadID = GetCurrentThreadId();
}

void plThreadUtils::YieldTimeSlice()
{
  ::Sleep(0);
}

void plThreadUtils::YieldHardwareThread()
{
  YieldProcessor();
}

void plThreadUtils::Sleep(const plTime& duration)
{
  ::Sleep((DWORD)duration.GetMilliseconds());
}

plThreadID plThreadUtils::GetCurrentThreadID()
{
  return ::GetCurrentThreadId();
}

bool plThreadUtils::IsMainThread()
{
  return GetCurrentThreadID() == g_uiMainThreadID;
}

#endif



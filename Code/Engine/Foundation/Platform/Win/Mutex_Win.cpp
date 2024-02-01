#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Mutex.h>

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

template <plUInt32 a, plUInt32 b>
struct SameSize
{
  static_assert(a == b, "Critical section has incorrect size");
};

template <plUInt32 a, plUInt32 b>
struct SameAlignment
{
  static_assert(a == b, "Critical section has incorrect alignment");
};


plMutex::plMutex()
{
  SameSize<sizeof(plMutexHandle), sizeof(CRITICAL_SECTION)> check1;
  (void)check1;
  SameAlignment<alignof(plMutexHandle), alignof(CRITICAL_SECTION)> check2;
  (void)check2;
  InitializeCriticalSection((CRITICAL_SECTION*)&m_hHandle);
}

plMutex::~plMutex()
{
  DeleteCriticalSection((CRITICAL_SECTION*)&m_hHandle);
}
#endif



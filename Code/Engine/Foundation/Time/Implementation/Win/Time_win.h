#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

static double g_fInvQpcFrequency;

void plTime::Initialize()
{
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);

  g_fInvQpcFrequency = 1.0 / double(frequency.QuadPart);
}

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
thread_local plInt64 s_LastTime;
#endif

plTime plTime::Now()
{
  LARGE_INTEGER temp;
  QueryPerformanceCounter(&temp);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  PLASMA_ASSERT_DEV(s_LastTime <= temp.QuadPart, "Serious problem, Steve. This is like \"Houston, forget that other thing\".\n\n\n"
                                             "When this happens the PC timer is unreliable. It was probably called from different threads "
                                             "and the clocks on different CPU cores seem to return different results.\n"
                                             "Under these conditions the engine cannot run reliably, it might crash or act weird.");
  s_LastTime = temp.QuadPart;
#endif

  return plTime::Seconds(double(temp.QuadPart) * g_fInvQpcFrequency);
}

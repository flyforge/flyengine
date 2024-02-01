#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_OSX)

#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

static double g_TimeFactor = 0;

void plTime::Initialize()
{
  mach_timebase_info_data_t TimebaseInfo;
  mach_timebase_info(&TimebaseInfo);
  g_TimeFactor = (double)TimebaseInfo.numer / (double)TimebaseInfo.denom / (double)1000000000LL;
}

plTime plTime::Now()
{
  // mach_absolute_time() returns nanoseconds after factoring in the mach_timebase_info_data_t
  return plTime::MakeFromSeconds((double)mach_absolute_time() * g_TimeFactor);
}

#endif



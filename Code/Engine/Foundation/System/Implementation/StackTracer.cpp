#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/System/StackTracer.h>

// Include inline file
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)

#  if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#    include <Foundation/System/Implementation/Win/StackTracer_uwp.h>
#  else
#    include <Foundation/System/Implementation/Win/StackTracer_win.h>
#  endif

#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
#  include <Foundation/System/Implementation/Posix/StackTracer_posix.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/System/Implementation/Android/StackTracer_android.h>
#else
#  error "StackTracer is not implemented on current platform"
#endif

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Foundation, StackTracer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Time"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plPlugin::Events().AddEventHandler(plStackTracer::OnPluginEvent);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plPlugin::Events().RemoveEventHandler(plStackTracer::OnPluginEvent);
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

void plStackTracer::PrintStackTrace(const plArrayPtr<void*>& trace, plStackTracer::PrintFunc printFunc)
{
  char buffer[32];
  const plUInt32 uiNumTraceEntries = trace.GetCount();
  for (plUInt32 i = 0; i < uiNumTraceEntries; i++)
  {
    plStringUtils::snprintf(buffer, PLASMA_ARRAY_SIZE(buffer), "%s%p", i == 0 ? "" : "|", trace[i]);
    printFunc(buffer);
  }
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_System_Implementation_StackTracer);

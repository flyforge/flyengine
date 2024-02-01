#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/System/StackTracer.h>

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(Foundation, StackTracer)

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

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

void plStackTracer::PrintStackTrace(const plArrayPtr<void*>& trace, plStackTracer::PrintFunc printFunc)
{
  char buffer[32];
  const plUInt32 uiNumTraceEntries = trace.GetCount();
  for (plUInt32 i = 0; i < uiNumTraceEntries; i++)
  {
    plStringUtils::snprintf(buffer, PL_ARRAY_SIZE(buffer), "%s%p", i == 0 ? "" : "|", trace[i]);
    printFunc(buffer);
  }
}

PL_STATICLINK_FILE(Foundation, Foundation_System_Implementation_StackTracer);

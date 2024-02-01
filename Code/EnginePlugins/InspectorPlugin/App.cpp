#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Utilities/Stats.h>

static plAssertHandler g_PreviousAssertHandler = nullptr;

static bool TelemetryAssertHandler(const char* szSourceFile, plUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  if (plTelemetry::IsConnectedToClient())
  {
    plTelemetryMessage msg;
    msg.SetMessageID(' APP', 'ASRT');
    msg.GetWriter() << szSourceFile;
    msg.GetWriter() << uiLine;
    msg.GetWriter() << szFunction;
    msg.GetWriter() << szExpression;
    msg.GetWriter() << szAssertMsg;

    plTelemetry::Broadcast(plTelemetry::Reliable, msg);

    // messages might not arrive, if the network does not get enough time to transmit them
    // since we are crashing the application in (half) 'a second', we need to make sure the network traffic has indeed been sent
    for (plUInt32 i = 0; i < 5; ++i)
    {
      plThreadUtils::Sleep(plTime::MakeFromMilliseconds(100));
      plTelemetry::UpdateNetwork();
    }
  }

  if (g_PreviousAssertHandler)
    return g_PreviousAssertHandler(szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);

  return true;
}

void AddTelemetryAssertHandler()
{
  g_PreviousAssertHandler = plGetAssertHandler();
  plSetAssertHandler(TelemetryAssertHandler);
}

void RemoveTelemetryAssertHandler()
{
  plSetAssertHandler(g_PreviousAssertHandler);
  g_PreviousAssertHandler = nullptr;
}

void SetAppStats()
{
  plStringBuilder sOut;
  const plSystemInformation info = plSystemInformation::Get();

  plStats::SetStat("Platform/Name", info.GetPlatformName());

  plStats::SetStat("Hardware/CPU Cores", info.GetCPUCoreCount());

  plStats::SetStat("Hardware/RAM[GB]", info.GetInstalledMainMemory() / 1024.0f / 1024.0f / 1024.0f);

  sOut = info.Is64BitOS() ? "64 Bit" : "32 Bit";
  plStats::SetStat("Platform/Architecture", sOut.GetData());

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  sOut = "Debug";
#elif PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  sOut = "Dev";
#else
  sOut = "Release";
#endif
  plStats::SetStat("Platform/Build", sOut.GetData());

#if PL_ENABLED(PL_USE_PROFILING)
  sOut = "Enabled";
#else
  sOut = "Disabled";
#endif
  plStats::SetStat("Features/Profiling", sOut.GetData());

  if constexpr (plAllocatorTrackingMode::Default >= plAllocatorTrackingMode::AllocationStats)
    sOut = "Enabled";
  else
    sOut = "Disabled";

  plStats::SetStat("Features/Allocation Tracking", sOut.GetData());

  if constexpr (plAllocatorTrackingMode::Default >= plAllocatorTrackingMode::AllocationStatsAndStacktraces)
    sOut = "Enabled";
  else
    sOut = "Disabled";

  plStats::SetStat("Features/Allocation Stack Tracing", sOut.GetData());

#if PL_ENABLED(PL_PLATFORM_LITTLE_ENDIAN)
  sOut = "Little";
#else
  sOut = "Big";
#endif
  plStats::SetStat("Platform/Endianess", sOut.GetData());
}



PL_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_App);

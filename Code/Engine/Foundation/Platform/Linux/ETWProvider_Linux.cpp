#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_LINUX) && defined(BUILDSYSTEM_ENABLE_TRACELOGGING_LTTNG_SUPPORT)

#  include <Foundation/Platform/Linux/ETWProvider_Linux.h>

#  include <tracelogging/TraceLoggingProvider.h>

TRACELOGGING_DECLARE_PROVIDER(g_plETWLogProvider);

// Define the GUID to use for the pl ETW Logger
// {BFD4350A-BA77-463D-B4BE-E30374E42494}
#  define PL_LOGGER_GUID (0xbfd4350a, 0xba77, 0x463d, 0xb4, 0xbe, 0xe3, 0x3, 0x74, 0xe4, 0x24, 0x94)

TRACELOGGING_DEFINE_PROVIDER(g_plETWLogProvider, "plLogProvider", PL_LOGGER_GUID);

plETWProvider::plETWProvider()
{
  TraceLoggingRegister(g_plETWLogProvider);
}

plETWProvider::~plETWProvider()
{
  TraceLoggingUnregister(g_plETWLogProvider);
}

void plETWProvider::LogMessage(plLogMsgType::Enum eventType, plUInt8 uiIndentation, plStringView sText)
{
  const plStringBuilder sTemp = sText;

  TraceLoggingWrite(g_plETWLogProvider, "LogMessage", TraceLoggingValue((int)eventType, "Type"), TraceLoggingValue(uiIndentation, "Indentation"),
    TraceLoggingValue(sTemp.GetData(), "Text"));
}

plETWProvider& plETWProvider::GetInstance()
{
  static plETWProvider instance;
  return instance;
}
#endif



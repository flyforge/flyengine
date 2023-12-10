#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Implementation/Win/ETWProvider_win.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <TraceLoggingProvider.h>

// Workaround to support TraceLoggingProvider.h and /utf-8 compiler switch.
#  undef _TlgPragmaUtf8Begin
#  undef _TlgPragmaUtf8End
#  define _TlgPragmaUtf8Begin
#  define _TlgPragmaUtf8End
#  undef _tlgPragmaUtf8Begin
#  undef _tlgPragmaUtf8End
#  define _tlgPragmaUtf8Begin
#  define _tlgPragmaUtf8End

TRACELOGGING_DECLARE_PROVIDER(g_plETWLogProvider);

// Define the GUID to use for the pl ETW Logger
// {BFD4350A-BA77-463D-B4BE-E30374E42494}
#  define PLASMA_LOGGER_GUID (0xbfd4350a, 0xba77, 0x463d, 0xb4, 0xbe, 0xe3, 0x3, 0x74, 0xe4, 0x24, 0x94)

TRACELOGGING_DEFINE_PROVIDER(g_plETWLogProvider, "plLogProvider", PLASMA_LOGGER_GUID);

plETWProvider::plETWProvider()
{
  TraceLoggingRegister(g_plETWLogProvider);
}

plETWProvider::~plETWProvider()
{
  TraceLoggingUnregister(g_plETWLogProvider);
}

void plETWProvider::LogMessge(plLogMsgType::Enum eventType, plUInt8 uiIndentation, plStringView sText)
{
  const plStringBuilder sTemp = sText;

  TraceLoggingWrite(g_plETWLogProvider, "LogMessge", TraceLoggingValue((int)eventType, "Type"), TraceLoggingValue(uiIndentation, "Indentation"),
    TraceLoggingValue(sTemp.GetData(), "Text"));
}

plETWProvider& plETWProvider::GetInstance()
{
  static plETWProvider instance;
  return instance;
}
#endif


PLASMA_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_Win_ETWProvider_win);

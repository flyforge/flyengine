#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/ETWWriter.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS) || (PL_ENABLED(PL_PLATFORM_LINUX) && defined(BUILDSYSTEM_ENABLE_TRACELOGGING_LTTNG_SUPPORT))

#  if PL_ENABLED(PL_PLATFORM_WINDOWS)
#    include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#    include <Foundation/Platform/Win/ETWProvider_Win.h>
#  else
#    include <Foundation/Platform/Linux/ETWProvider_Linux.h>
#  endif

void plLogWriter::ETW::LogMessageHandler(const plLoggingEventData& eventData)
{
  if (eventData.m_EventType == plLogMsgType::Flush)
    return;

  plETWProvider::GetInstance().LogMessage(eventData.m_EventType, eventData.m_uiIndentation, eventData.m_sText);
}

void plLogWriter::ETW::LogMessage(plLogMsgType::Enum eventType, plUInt8 uiIndentation, plStringView sText)
{
  if (eventType == plLogMsgType::Flush)
    return;

  plETWProvider::GetInstance().LogMessage(eventType, uiIndentation, sText);
}

#else

void plLogWriter::ETW::LogMessageHandler(const plLoggingEventData& eventData) {}

void plLogWriter::ETW::LogMessage(plLogMsgType::Enum eventType, plUInt8 uiIndentation, plStringView sText) {}

#endif

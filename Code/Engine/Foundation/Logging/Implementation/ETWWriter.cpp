#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/ETWWriter.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Logging/Implementation/Win/ETWProvider_win.h>

void plLogWriter::ETW::LogMessageHandler(const plLoggingEventData& eventData)
{
  if (eventData.m_EventType == plLogMsgType::Flush)
    return;

  plETWProvider::GetInstance().LogMessge(eventData.m_EventType, eventData.m_uiIndentation, eventData.m_sText);
}

void plLogWriter::ETW::LogMessage(plLogMsgType::Enum eventType, plUInt8 uiIndentation, plStringView sText)
{
  if (eventType == plLogMsgType::Flush)
    return;

  plETWProvider::GetInstance().LogMessge(eventType, uiIndentation, sText);
}

#else

void plLogWriter::ETW::LogMessageHandler(const plLoggingEventData& eventData) {}

void plLogWriter::ETW::LogMessage(plLogMsgType::Enum eventType, plUInt8 uiIndentation, plStringView sText) {}

#endif

PLASMA_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_ETWWriter);

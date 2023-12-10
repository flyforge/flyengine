#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/StringConversion.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

void plLogWriter::VisualStudio::LogMessageHandler(const plLoggingEventData& eventData)
{
  if (eventData.m_EventType == plLogMsgType::Flush)
    return;

  static plMutex WriterLock; // will only be created if this writer is used at all
  PLASMA_LOCK(WriterLock);

  if (eventData.m_EventType == plLogMsgType::BeginGroup)
    OutputDebugStringA("\n");

  for (plUInt32 i = 0; i < eventData.m_uiIndentation; ++i)
    OutputDebugStringA(" ");

  plStringBuilder s;

  switch (eventData.m_EventType)
  {
    case plLogMsgType::BeginGroup:
      s.Format("+++++ {} ({}) +++++\n", eventData.m_sText, eventData.m_sTag);
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::EndGroup:
#  if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
      s.Format("----- {} ({} sec) -----\n\n", eventData.m_sText, eventData.m_fSeconds);
#  else
      s.Format("----- {} (timing info not available) -----\n\n", eventData.m_sText);
#  endif
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::ErrorMsg:
      s.Format("Error: {}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::SeriousWarningMsg:
      s.Format("Seriously: {}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::WarningMsg:
      s.Format("Warning: {}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::SuccessMsg:
      s.Format("{}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::InfoMsg:
      s.Format("{}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::DevMsg:
      s.Format("{}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::DebugMsg:
      s.Format("{}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));
      break;

    default:
      s.Format("{}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));

      plLog::Warning("Unknown Message Type {0}", eventData.m_EventType);
      break;
  }
}

#else

void plLogWriter::VisualStudio::LogMessageHandler(const plLoggingEventData& eventData) {}

#endif



PLASMA_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_VisualStudioWriter);

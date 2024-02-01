#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/StringConversion.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

void plLogWriter::VisualStudio::LogMessageHandler(const plLoggingEventData& eventData)
{
  if (eventData.m_EventType == plLogMsgType::Flush)
    return;

  static plMutex WriterLock; // will only be created if this writer is used at all
  PL_LOCK(WriterLock);

  if (eventData.m_EventType == plLogMsgType::BeginGroup)
    OutputDebugStringA("\n");

  for (plUInt32 i = 0; i < eventData.m_uiIndentation; ++i)
    OutputDebugStringA(" ");

  plStringBuilder s;

  switch (eventData.m_EventType)
  {
    case plLogMsgType::BeginGroup:
      s.SetFormat("+++++ {} ({}) +++++\n", eventData.m_sText, eventData.m_sTag);
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::EndGroup:
#  if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
      s.SetFormat("----- {} ({} sec) -----\n\n", eventData.m_sText, eventData.m_fSeconds);
#  else
      s.SetFormat("----- {} (timing info not available) -----\n\n", eventData.m_sText);
#  endif
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::ErrorMsg:
      s.SetFormat("Error: {}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::SeriousWarningMsg:
      s.SetFormat("Seriously: {}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::WarningMsg:
      s.SetFormat("Warning: {}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::SuccessMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::InfoMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::DevMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));
      break;

    case plLogMsgType::DebugMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));
      break;

    default:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(plStringWChar(s));

      plLog::Warning("Unknown Message Type {0}", eventData.m_EventType);
      break;
  }
}

#else

void plLogWriter::VisualStudio::LogMessageHandler(const plLoggingEventData& eventData) {}

#endif



#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Time/Timestamp.h>

#if PL_ENABLED(PL_PLATFORM_ANDROID)
#  include <android/log.h>
#  define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "plEngine", __VA_ARGS__)
#endif

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

static void SetConsoleColor(WORD ui)
{
#  if PL_DISABLED(PL_PLATFORM_WINDOWS_UWP)
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ui);
#  endif
}
#elif PL_ENABLED(PL_PLATFORM_OSX) || PL_ENABLED(PL_PLATFORM_LINUX) || PL_ENABLED(PL_PLATFORM_ANDROID)
static void SetConsoleColor(plUInt8 ui) {}
#else
#  error "Unknown Platform."
static void SetConsoleColor(plUInt8 ui) {}
#endif

plLog::TimestampMode plLogWriter::Console::s_TimestampMode = plLog::TimestampMode::None;

void plLogWriter::Console::LogMessageHandler(const plLoggingEventData& eventData)
{
  plStringBuilder sTimestamp;
  plLog::GenerateFormattedTimestamp(s_TimestampMode, sTimestamp);

  static plMutex WriterLock; // will only be created if this writer is used at all
  PL_LOCK(WriterLock);

  if (eventData.m_EventType == plLogMsgType::BeginGroup)
    printf("\n");

  for (plUInt32 i = 0; i < eventData.m_uiIndentation; ++i)
    printf(" ");

  plStringBuilder sTemp1, sTemp2;

  switch (eventData.m_EventType)
  {
    case plLogMsgType::Flush:
      fflush(stdout);
      break;

    case plLogMsgType::BeginGroup:
      SetConsoleColor(0x02);
      printf("+++++ %s (%s) +++++\n", eventData.m_sText.GetData(sTemp1), eventData.m_sTag.GetData(sTemp2));
      break;

    case plLogMsgType::EndGroup:
      SetConsoleColor(0x02);
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
      printf("----- %s (%.6f sec)-----\n\n", eventData.m_sText.GetData(sTemp1), eventData.m_fSeconds);
#else
      printf("----- %s (%s)-----\n\n", eventData.m_sText.GetData(sTemp1), "timing info not available");
#endif
      break;

    case plLogMsgType::ErrorMsg:
      SetConsoleColor(0x0C);
      printf("%sError: %s\n", sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      fflush(stdout);
      break;

    case plLogMsgType::SeriousWarningMsg:
      SetConsoleColor(0x0C);
      printf("%sSeriously: %s\n", sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    case plLogMsgType::WarningMsg:
      SetConsoleColor(0x0E);
      printf("%sWarning: %s\n", sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    case plLogMsgType::SuccessMsg:
      SetConsoleColor(0x0A);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      fflush(stdout);
      break;

    case plLogMsgType::InfoMsg:
      SetConsoleColor(0x07);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    case plLogMsgType::DevMsg:
      SetConsoleColor(0x08);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    case plLogMsgType::DebugMsg:
      SetConsoleColor(0x09);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    default:
      SetConsoleColor(0x0D);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));

      plLog::Warning("Unknown Message Type {0}", eventData.m_EventType);
      break;
  }

  SetConsoleColor(0x07);
}

void plLogWriter::Console::SetTimestampMode(plLog::TimestampMode mode)
{
  s_TimestampMode = mode;
}
#if PL_ENABLED(PL_PLATFORM_ANDROID)
#  undef printf
#endif



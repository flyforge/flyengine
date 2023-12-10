#pragma once

#include <TestFramework/Framework/TestFramework.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <android/log.h>
#endif
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Logging/ETWWriter.h>
inline void SetConsoleColorInl(WORD ui)
{
#  if PLASMA_DISABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ui);
#  endif
}
#else
inline void SetConsoleColorInl(plUInt8 ui) {}
#endif

inline void OutputToConsole(plTestOutput::Enum type, const char* szMsg)
{
  static plInt32 iIndentation = 0;
  static bool bAnyError = false;

  switch (type)
  {
    case plTestOutput::StartOutput:
      break;
    case plTestOutput::BeginBlock:
      iIndentation += 2;
      break;
    case plTestOutput::EndBlock:
      iIndentation -= 2;
      break;
    case plTestOutput::Details:
      SetConsoleColorInl(0x07);
      break;
    case plTestOutput::ImportantInfo:
      SetConsoleColorInl(0x07);
      break;
    case plTestOutput::Success:
      SetConsoleColorInl(0x0A);
      break;
    case plTestOutput::Message:
      SetConsoleColorInl(0x0E);
      break;
    case plTestOutput::Warning:
      SetConsoleColorInl(0x0C);
      break;
    case plTestOutput::Error:
      SetConsoleColorInl(0x0C);
      bAnyError = true;
      break;
    case plTestOutput::Duration:
    case plTestOutput::ImageDiffFile:
    case plTestOutput::InvalidType:
    case plTestOutput::AllOutputTypes:
      return;

    case plTestOutput::FinalResult:
      if (bAnyError)
        SetConsoleColorInl(0x0C);
      else
        SetConsoleColorInl(0x0A);

      // reset it for the next test round
      bAnyError = false;
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  printf("%*s%s\n", iIndentation, "", szMsg);
  SetConsoleColorInl(0x07);

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  plLogMsgType::Enum logType = plLogMsgType::None;
  switch (Type)
  {
    case plTestOutput::StartOutput:
    case plTestOutput::InvalidType:
    case plTestOutput::AllOutputTypes:
      logType = plLogMsgType::None;
      break;
    case plTestOutput::BeginBlock:
      logType = plLogMsgType::BeginGroup;
      break;
    case plTestOutput::EndBlock:
      logType = plLogMsgType::EndGroup;
      break;
    case plTestOutput::ImportantInfo:
    case plTestOutput::Details:
    case plTestOutput::Message:
    case plTestOutput::Duration:
    case plTestOutput::FinalResult:
      logType = plLogMsgType::InfoMsg;
      break;
    case plTestOutput::Success:
      logType = plLogMsgType::SuccessMsg;
      break;
    case plTestOutput::Warning:
      logType = plLogMsgType::WarningMsg;
      break;
    case plTestOutput::Error:
      logType = plLogMsgType::ErrorMsg;
      break;
    case plTestOutput::ImageDiffFile:
      logType = plLogMsgType::DevMsg;
      break;
    default:
      break;
  }
  if (logType != plLogMsgType::None)
  {
    plLogWriter::ETW::LogMessage(plLogMsgType::InfoMsg, iIndentation, szMsg);
  }
#endif
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  char sz[4096];
  plStringUtils::snprintf(sz, 4096, "%*s%s\n", iIndentation, "", szMsg);
  OutputDebugStringW(plStringWChar(sz).GetData());
#endif
#if PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
  __android_log_print(ANDROID_LOG_DEBUG, "plEngine", "%*s%s\n", iIndentation, "", szMsg);
#endif

  if (type >= plTestOutput::Error)
  {
    fflush(stdout);
  }
}

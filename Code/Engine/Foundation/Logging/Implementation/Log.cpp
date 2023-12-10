#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Logging/Implementation/Win/ETWProvider_win.h>
#endif
#if PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <android/log.h>
#endif

#include <stdarg.h>

plLogMsgType::Enum plLog::s_DefaultLogLevel = plLogMsgType::All;
plLog::PrintFunction plLog::s_CustomPrintFunction = nullptr;
plAtomicInteger32 plGlobalLog::s_uiMessageCount[plLogMsgType::ENUM_COUNT];
plLoggingEvent plGlobalLog::s_LoggingEvent;
plLogInterface* plGlobalLog::s_pOverrideLog = nullptr;
static thread_local bool s_bAllowOverrideLog = true;
static plMutex s_OverrideLogMutex;

/// \brief The log system that messages are sent to when the user specifies no system himself.
static thread_local plLogInterface* s_DefaultLogSystem = nullptr;


plEventSubscriptionID plGlobalLog::AddLogWriter(plLoggingEvent::Handler handler)
{
  if (s_LoggingEvent.HasEventHandler(handler))
    return 0;

  return s_LoggingEvent.AddEventHandler(handler);
}

void plGlobalLog::RemoveLogWriter(plLoggingEvent::Handler handler)
{
  if (!s_LoggingEvent.HasEventHandler(handler))
    return;

  s_LoggingEvent.RemoveEventHandler(handler);
}

void plGlobalLog::RemoveLogWriter(plEventSubscriptionID& ref_subscriptionID)
{
  s_LoggingEvent.RemoveEventHandler(ref_subscriptionID);
}

void plGlobalLog::SetGlobalLogOverride(plLogInterface* pInterface)
{
  PLASMA_LOCK(s_OverrideLogMutex);

  PLASMA_ASSERT_DEV(pInterface == nullptr || s_pOverrideLog == nullptr, "Only one override log can be set at a time");
  s_pOverrideLog = pInterface;
}

void plGlobalLog::HandleLogMessage(const plLoggingEventData& le)
{
  if (s_pOverrideLog != nullptr && s_pOverrideLog != this && s_bAllowOverrideLog)
  {
    // only enter the lock when really necessary
    PLASMA_LOCK(s_OverrideLogMutex);

    // since s_bAllowOverrideLog is thread_local we do not need to re-check it

    // check this again under the lock, to be safe
    if (s_pOverrideLog != nullptr && s_pOverrideLog != this)
    {
      // disable the override log for the period in which it handles the event
      // to prevent infinite recursions
      s_bAllowOverrideLog = false;
      s_pOverrideLog->HandleLogMessage(le);
      s_bAllowOverrideLog = true;

      return;
    }
  }

  // else
  {
    const plLogMsgType::Enum ThisType = le.m_EventType;

    if ((ThisType > plLogMsgType::None) && (ThisType < plLogMsgType::All))
      s_uiMessageCount[ThisType].Increment();

    s_LoggingEvent.Broadcast(le);
  }
}

plLogBlock::plLogBlock(plStringView sName, plStringView sContextInfo)
{
  m_pLogInterface = plLog::GetThreadLocalLogSystem();

  if (!m_pLogInterface)
    return;

  m_sName = sName;
  m_sContextInfo = sContextInfo;
  m_bWritten = false;

  m_pParentBlock = m_pLogInterface->m_pCurrentBlock;
  m_pLogInterface->m_pCurrentBlock = this;

  m_uiBlockDepth = m_pParentBlock ? (m_pParentBlock->m_uiBlockDepth + 1) : 0;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = plTime::Now().GetSeconds();
#endif
}


plLogBlock::plLogBlock(plLogInterface* pInterface, plStringView sName, plStringView sContextInfo)
{
  m_pLogInterface = pInterface;

  if (!m_pLogInterface)
    return;

  m_sName = sName;
  m_sContextInfo = sContextInfo;
  m_bWritten = false;

  m_pParentBlock = m_pLogInterface->m_pCurrentBlock;
  m_pLogInterface->m_pCurrentBlock = this;

  m_uiBlockDepth = m_pParentBlock ? (m_pParentBlock->m_uiBlockDepth + 1) : 0;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = plTime::Now().GetSeconds();
#endif
}

plLogBlock::~plLogBlock()
{
  if (!m_pLogInterface)
    return;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = plTime::Now().GetSeconds() - m_fSeconds;
#endif

  m_pLogInterface->m_pCurrentBlock = m_pParentBlock;

  plLog::EndLogBlock(m_pLogInterface, this);
}


void plLog::EndLogBlock(plLogInterface* pInterface, plLogBlock* pBlock)
{
  if (pBlock->m_bWritten)
  {
    plLoggingEventData le;
    le.m_EventType = plLogMsgType::EndGroup;
    le.m_sText = pBlock->m_sName;
    le.m_uiIndentation = pBlock->m_uiBlockDepth;
    le.m_sTag = pBlock->m_sContextInfo;
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
    le.m_fSeconds = pBlock->m_fSeconds;
#endif

    pInterface->HandleLogMessage(le);
  }
}

void plLog::WriteBlockHeader(plLogInterface* pInterface, plLogBlock* pBlock)
{
  if (!pBlock || pBlock->m_bWritten)
    return;

  pBlock->m_bWritten = true;

  WriteBlockHeader(pInterface, pBlock->m_pParentBlock);

  plLoggingEventData le;
  le.m_EventType = plLogMsgType::BeginGroup;
  le.m_sText = pBlock->m_sName;
  le.m_uiIndentation = pBlock->m_uiBlockDepth;
  le.m_sTag = pBlock->m_sContextInfo;

  pInterface->HandleLogMessage(le);
}

void plLog::BroadcastLoggingEvent(plLogInterface* pInterface, plLogMsgType::Enum type, plStringView sString)
{
  plLogBlock* pTopBlock = pInterface->m_pCurrentBlock;
  plUInt8 uiIndentation = 0;

  if (pTopBlock)
  {
    uiIndentation = pTopBlock->m_uiBlockDepth + 1;

    WriteBlockHeader(pInterface, pTopBlock);
  }

  char szTag[32] = "";

  if (sString.StartsWith("["))
  {
    const char* szAfterTag = sString.GetStartPointer();

    ++szAfterTag;

    plInt32 iPos = 0;

    // only treat it as a tag, if it is properly enclosed in square brackets and doesn't contain spaces
    while ((*szAfterTag != '\0') && (*szAfterTag != '[') && (*szAfterTag != ']') && (*szAfterTag != ' ') && (iPos < 31))
    {
      szTag[iPos] = *szAfterTag;
      ++szAfterTag;
      ++iPos;
    }

    if (*szAfterTag == ']')
    {
      szTag[iPos] = '\0';
      sString.SetStartPosition(szAfterTag + 1);
    }
    else
    {
      szTag[0] = '\0';
    }
  }

  plLoggingEventData le;
  le.m_EventType = type;
  le.m_sText = sString;
  le.m_uiIndentation = uiIndentation;
  le.m_sTag = szTag;

  pInterface->HandleLogMessage(le);
  pInterface->m_uiLoggedMsgsSinceFlush++;
}

void plLog::Print(const char* szText)
{
  printf("%s", szText);

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  plETWProvider::GetInstance().LogMessge(plLogMsgType::ErrorMsg, 0, szText);
#endif
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  OutputDebugStringW(plStringWChar(szText).GetData());
#endif
#if PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
  __android_log_print(ANDROID_LOG_ERROR, "plEngine", "%s", szText);
#endif

  if (s_CustomPrintFunction)
  {
    s_CustomPrintFunction(szText);
  }

  fflush(stdout);
  fflush(stderr);
}

void plLog::Printf(const char* szFormat, ...)
{
  va_list args;
  va_start(args, szFormat);

  char buffer[4096];
  plStringUtils::vsnprintf(buffer, PLASMA_ARRAY_SIZE(buffer), szFormat, args);

  Print(buffer);

  va_end(args);
}

void plLog::SetCustomPrintFunction(PrintFunction func)
{
  s_CustomPrintFunction = func;
}

void plLog::OsMessageBox(const plFormatString& text)
{
  plStringBuilder tmp;
  plStringBuilder display = text.GetText(tmp);
  display.Trim(" \n\r\t");

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  const char* title = "";
  if (plApplication::GetApplicationInstance())
  {
    title = plApplication::GetApplicationInstance()->GetApplicationName();
  }

  MessageBoxW(nullptr, plStringWChar(display).GetData(), plStringWChar(title), MB_OK);
#else
  plLog::Print(display);
  PLASMA_ASSERT_NOT_IMPLEMENTED;
#endif
}

void plLog::GenerateFormattedTimestamp(TimestampMode mode, plStringBuilder& ref_sTimestampOut)
{
  // if mode is 'None', early out to not even retrieve a timestamp
  if (mode == TimestampMode::None)
  {
    return;
  }

  const plDateTime dateTime = plDateTime::MakeFromTimestamp(plTimestamp::CurrentTimestamp());

  switch (mode)
  {
    case TimestampMode::Numeric:
      ref_sTimestampOut.Format("[{}] ", plArgDateTime(dateTime, plArgDateTime::ShowDate | plArgDateTime::ShowMilliseconds | plArgDateTime::ShowTimeZone));
      break;
    case TimestampMode::TimeOnly:
      ref_sTimestampOut.Format("[{}] ", plArgDateTime(dateTime, plArgDateTime::ShowMilliseconds));
      break;
    case TimestampMode::Textual:
      ref_sTimestampOut.Format(
        "[{}] ", plArgDateTime(dateTime, plArgDateTime::TextualDate | plArgDateTime::ShowMilliseconds | plArgDateTime::ShowTimeZone));
      break;
    default:
      PLASMA_ASSERT_DEV(false, "Unknown timestamp mode.");
      break;
  }
}

void plLog::SetThreadLocalLogSystem(plLogInterface* pInterface)
{
  PLASMA_ASSERT_DEV(pInterface != nullptr,
    "You cannot set a nullptr logging system. If you want to discard all log information, set a dummy system that does not do anything.");

  s_DefaultLogSystem = pInterface;
}

plLogInterface* plLog::GetThreadLocalLogSystem()
{
  if (s_DefaultLogSystem == nullptr)
  {
    // use new, not PLASMA_DEFAULT_NEW, to prevent tracking
    s_DefaultLogSystem = new plGlobalLog;
  }

  return s_DefaultLogSystem;
}

void plLog::SetDefaultLogLevel(plLogMsgType::Enum logLevel)
{
  PLASMA_ASSERT_DEV(logLevel >= plLogMsgType::None && logLevel <= plLogMsgType::All, "Invalid default log level {}", (int)logLevel);

  s_DefaultLogLevel = logLevel;
}

plLogMsgType::Enum plLog::GetDefaultLogLevel()
{
  return s_DefaultLogLevel;
}

#define LOG_LEVEL_FILTER(MaxLevel)                                                                                                  \
  if (pInterface == nullptr)                                                                                                        \
    return;                                                                                                                         \
  if ((pInterface->GetLogLevel() == plLogMsgType::GlobalDefault ? plLog::s_DefaultLogLevel : pInterface->GetLogLevel()) < MaxLevel) \
    return;


void plLog::Error(plLogInterface* pInterface, const plFormatString& string)
{
  LOG_LEVEL_FILTER(plLogMsgType::ErrorMsg);

  plStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, plLogMsgType::ErrorMsg, string.GetText(tmp));
}

void plLog::SeriousWarning(plLogInterface* pInterface, const plFormatString& string)
{
  LOG_LEVEL_FILTER(plLogMsgType::SeriousWarningMsg);

  plStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, plLogMsgType::SeriousWarningMsg, string.GetText(tmp));
}

void plLog::Warning(plLogInterface* pInterface, const plFormatString& string)
{
  LOG_LEVEL_FILTER(plLogMsgType::WarningMsg);

  plStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, plLogMsgType::WarningMsg, string.GetText(tmp));
}

void plLog::Success(plLogInterface* pInterface, const plFormatString& string)
{
  LOG_LEVEL_FILTER(plLogMsgType::SuccessMsg);

  plStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, plLogMsgType::SuccessMsg, string.GetText(tmp));
}

void plLog::Info(plLogInterface* pInterface, const plFormatString& string)
{
  LOG_LEVEL_FILTER(plLogMsgType::InfoMsg);

  plStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, plLogMsgType::InfoMsg, string.GetText(tmp));
}

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)

void plLog::Dev(plLogInterface* pInterface, const plFormatString& string)
{
  LOG_LEVEL_FILTER(plLogMsgType::DevMsg);

  plStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, plLogMsgType::DevMsg, string.GetText(tmp));
}

#endif

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)

void plLog::Debug(plLogInterface* pInterface, const plFormatString& string)
{
  LOG_LEVEL_FILTER(plLogMsgType::DebugMsg);

  plStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, plLogMsgType::DebugMsg, string.GetText(tmp));
}

#endif

bool plLog::Flush(plUInt32 uiNumNewMsgThreshold, plTime timeIntervalThreshold, plLogInterface* pInterface /*= GetThreadLocalLogSystem()*/)
{
  if (pInterface == nullptr || pInterface->m_uiLoggedMsgsSinceFlush == 0) // if really nothing was logged, don't execute a flush
    return false;

  if (pInterface->m_uiLoggedMsgsSinceFlush <= uiNumNewMsgThreshold && plTime::Now() - pInterface->m_LastFlushTime < timeIntervalThreshold)
    return false;

  BroadcastLoggingEvent(pInterface, plLogMsgType::Flush, nullptr);

  pInterface->m_uiLoggedMsgsSinceFlush = 0;
  pInterface->m_LastFlushTime = plTime::Now();

  return true;
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_Log);

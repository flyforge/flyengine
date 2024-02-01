#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Time/Time.h>

/// \brief Use this helper macro to easily create a scoped logging group. Will generate unique variable names to make the static code
/// analysis happy.
#define PL_LOG_BLOCK plLogBlock PL_CONCAT(_logblock_, PL_SOURCE_LINE)

/// \brief Use this helper macro to easily mute all logging in a scope.
#define PL_LOG_BLOCK_MUTE()                            \
  plMuteLog PL_CONCAT(_logmuteblock_, PL_SOURCE_LINE); \
  plLogSystemScope PL_CONCAT(_logscope_, PL_SOURCE_LINE)(&PL_CONCAT(_logmuteblock_, PL_SOURCE_LINE))

// Forward declaration, class is at the end of this file
class plLogBlock;


/// \brief Describes the types of events that plLog sends.
struct PL_FOUNDATION_DLL plLogMsgType
{
  using StorageType = plInt8;

  enum Enum : plInt8
  {
    GlobalDefault = -4,    ///< Takes the log level from the plLog default value. See plLog::SetDefaultLogLevel().
    Flush = -3,            ///< The user explicitly called plLog::Flush() to instruct log writers to flush any cached output.
    BeginGroup = -2,       ///< A logging group has been opened.
    EndGroup = -1,         ///< A logging group has been closed.
    None = 0,              ///< Can be used to disable all log message types.
    ErrorMsg = 1,          ///< An error message.
    SeriousWarningMsg = 2, ///< A serious warning message.
    WarningMsg = 3,        ///< A warning message.
    SuccessMsg = 4,        ///< A success message.
    InfoMsg = 5,           ///< An info message.
    DevMsg = 6,            ///< A development message.
    DebugMsg = 7,          ///< A debug message.
    All = 8,               ///< Can be used to enable all log message types.
    ENUM_COUNT,
    Default = None,
  };
};

/// \brief The data that is sent through plLogInterface.
struct PL_FOUNDATION_DLL plLoggingEventData
{
  /// \brief The type of information that is sent.
  plLogMsgType::Enum m_EventType = plLogMsgType::None;

  /// \brief How many "levels" to indent.
  plUInt8 m_uiIndentation = 0;

  /// \brief The information text.
  plStringView m_sText;

  /// \brief An optional tag extracted from the log-string (if it started with "[SomeTag]Logging String.") Can be used by log-writers for
  /// additional configuration, or simply be ignored.
  plStringView m_sTag;

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  /// \brief Used by log-blocks for profiling the duration of the block
  double m_fSeconds = 0;
#endif
};

using plLoggingEvent = plEvent<const plLoggingEventData&, plMutex>;

/// \brief Base class for all logging classes.
///
/// You can derive from this class to create your own logging system,
/// which you can pass to the functions in plLog.
class PL_FOUNDATION_DLL plLogInterface
{
public:
  /// \brief Override this function to handle logging events.
  virtual void HandleLogMessage(const plLoggingEventData& le) = 0;

  /// \brief LogLevel is between plLogEventType::None and plLogEventType::All and defines which messages will be logged and which will be
  /// filtered out.
  PL_ALWAYS_INLINE void SetLogLevel(plLogMsgType::Enum logLevel) { m_LogLevel = logLevel; }

  /// \brief Returns the currently set log level.
  PL_ALWAYS_INLINE plLogMsgType::Enum GetLogLevel() { return m_LogLevel; }

private:
  friend class plLog;
  friend class plLogBlock;
  plLogBlock* m_pCurrentBlock = nullptr;
  plLogMsgType::Enum m_LogLevel = plLogMsgType::GlobalDefault;
  plUInt32 m_uiLoggedMsgsSinceFlush = 0;
  plTime m_LastFlushTime;
};


/// \brief Used to ignore all log messages.
/// \sa PL_LOG_BLOCK_MUTE
class plMuteLog : public plLogInterface
{
public:
  plMuteLog()
  {
    SetLogLevel(plLogMsgType::None);
  }

  virtual void HandleLogMessage(const plLoggingEventData&) override {}
};


/// \brief This is the standard log system that plLog sends all messages to.
///
/// It allows to register log writers, such that you can be informed of all log messages and write them
/// to different outputs.
class PL_FOUNDATION_DLL plGlobalLog : public plLogInterface
{
public:
  virtual void HandleLogMessage(const plLoggingEventData& le) override;

  /// \brief Allows to register a function as an event receiver.
  static plEventSubscriptionID AddLogWriter(plLoggingEvent::Handler handler);

  /// \brief Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveLogWriter(plLoggingEvent::Handler handler);

  /// \brief Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveLogWriter(plEventSubscriptionID& ref_subscriptionID);

  /// \brief Returns how many message of the given type occurred.
  static plUInt32 GetMessageCount(plLogMsgType::Enum messageType) { return s_uiMessageCount[messageType]; }

  /// plLogInterfaces are thread_local and therefore a dedicated plGlobalLog is created per thread.
  /// Especially during testing one may want to replace the log system everywhere, to catch certain messages, no matter on which thread they
  /// happen. Unfortunately that is not so easy, as one cannot modify the thread_local system for all the other threads. This function makes
  /// it possible to at least force all messages that go through any plGlobalLog to be redirected to one other log interface. Be aware that
  /// that interface has to be thread-safe. Also, only one override can be set at a time, SetGlobalLogOverride() will assert that no other
  /// override is set at the moment.
  static void SetGlobalLogOverride(plLogInterface* pInterface);

private:
  /// \brief Counts the number of messages of each type.
  static plAtomicInteger32 s_uiMessageCount[plLogMsgType::ENUM_COUNT];

  /// \brief Manages all the Event Handlers for the logging events.
  static plLoggingEvent s_LoggingEvent;

  static plLogInterface* s_pOverrideLog;

private:
  PL_DISALLOW_COPY_AND_ASSIGN(plGlobalLog);

  friend class plLog; // only plLog may create instances of this class
  plGlobalLog() = default;
};

/// \brief Static class that allows to write out logging information.
///
/// This class takes logging information, prepares it and then broadcasts it to all interested code
/// via the event interface. It does not write anything on disk or somewhere else, itself. Instead it
/// allows to register custom log writers that can then write it to disk, to console, send it over a
/// network or pop up a message box. Whatever suits the current situation.
/// Since event handlers can be registered only temporarily, it is also possible to just gather all
/// errors that occur during some operation and then unregister the event handler again.
class PL_FOUNDATION_DLL plLog
{
public:
  /// \brief Allows to change which logging system is used by default on the current thread. If nothing is set, plGlobalLog is used.
  ///
  /// Replacing the log system on a thread does not delete the previous system, so it can be reinstated later again.
  /// This can be used to temporarily route all logging to a custom system.
  static void SetThreadLocalLogSystem(plLogInterface* pInterface);

  /// \brief Returns the currently set default logging system, or a thread local instance of plGlobalLog, if nothing else was set.
  static plLogInterface* GetThreadLocalLogSystem();

  /// \brief Sets the default log level which is used by all plLogInterface's that have their log level set to plLogMsgType::GlobalDefault
  static void SetDefaultLogLevel(plLogMsgType::Enum logLevel);

  /// \brief Returns the currently set default log level.
  static plLogMsgType::Enum GetDefaultLogLevel();

  /// \brief An error that needs to be fixed as soon as possible.
  static void Error(plLogInterface* pInterface, const plFormatString& string);

  /// \brief An error that needs to be fixed as soon as possible.
  template <typename... ARGS>
  static void Error(plStringView sFormat, ARGS&&... args)
  {
    Error(GetThreadLocalLogSystem(), plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Overload of Error() to output messages to a specific log.
  template <typename... ARGS>
  static void Error(plLogInterface* pInterface, plStringView sFormat, ARGS&&... args)
  {
    Error(pInterface, plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Not an error, but definitely a big problem, that should be looked into very soon.
  static void SeriousWarning(plLogInterface* pInterface, const plFormatString& string);

  /// \brief Not an error, but definitely a big problem, that should be looked into very soon.
  template <typename... ARGS>
  static void SeriousWarning(plStringView sFormat, ARGS&&... args)
  {
    SeriousWarning(GetThreadLocalLogSystem(), plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Overload of SeriousWarning() to output messages to a specific log.
  template <typename... ARGS>
  static void SeriousWarning(plLogInterface* pInterface, plStringView sFormat, ARGS&&... args)
  {
    SeriousWarning(pInterface, plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief A potential problem or a performance warning. Might be possible to ignore it.
  static void Warning(plLogInterface* pInterface, const plFormatString& string);

  /// \brief A potential problem or a performance warning. Might be possible to ignore it.
  template <typename... ARGS>
  static void Warning(plStringView sFormat, ARGS&&... args)
  {
    Warning(GetThreadLocalLogSystem(), plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Overload of Warning() to output messages to a specific log.
  template <typename... ARGS>
  static void Warning(plLogInterface* pInterface, plStringView sFormat, ARGS&&... args)
  {
    Warning(pInterface, plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Status information that something was completed successfully.
  static void Success(plLogInterface* pInterface, const plFormatString& string);

  /// \brief Status information that something was completed successfully.
  template <typename... ARGS>
  static void Success(plStringView sFormat, ARGS&&... args)
  {
    Success(GetThreadLocalLogSystem(), plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Overload of Success() to output messages to a specific log.
  template <typename... ARGS>
  static void Success(plLogInterface* pInterface, plStringView sFormat, ARGS&&... args)
  {
    Success(pInterface, plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Status information that is important.
  static void Info(plLogInterface* pInterface, const plFormatString& string);

  /// \brief Status information that is important.
  template <typename... ARGS>
  static void Info(plStringView sFormat, ARGS&&... args)
  {
    Info(GetThreadLocalLogSystem(), plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Overload of Info() to output messages to a specific log.
  template <typename... ARGS>
  static void Info(plLogInterface* pInterface, plStringView sFormat, ARGS&&... args)
  {
    Info(pInterface, plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Status information that is nice to have during development.
  ///
  /// This function is compiled out in non-development builds.
  static void Dev(plLogInterface* pInterface, const plFormatString& string);

  /// \brief Status information that is nice to have during development.
  ///
  /// This function is compiled out in non-development builds.
  template <typename... ARGS>
  static void Dev(plStringView sFormat, ARGS&&... args)
  {
    Dev(GetThreadLocalLogSystem(), plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Overload of Dev() to output messages to a specific log.
  template <typename... ARGS>
  static void Dev(plLogInterface* pInterface, plStringView sFormat, ARGS&&... args)
  {
    Dev(pInterface, plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Status information during debugging. Very verbose. Usually only temporarily added to the code.
  ///
  /// This function is compiled out in non-debug builds.
  static void Debug(plLogInterface* pInterface, const plFormatString& string);

  /// \brief Status information during debugging. Very verbose. Usually only temporarily added to the code.
  ///
  /// This function is compiled out in non-debug builds.
  template <typename... ARGS>
  static void Debug(plStringView sFormat, ARGS&&... args)
  {
    Debug(GetThreadLocalLogSystem(), plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Overload of Debug() to output messages to a specific log.
  template <typename... ARGS>
  static void Debug(plLogInterface* pInterface, plStringView sFormat, ARGS&&... args)
  {
    Debug(pInterface, plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Instructs log writers to flush their caches, to ensure all log output (even non-critical information) is written.
  ///
  /// On some log writers this has no effect.
  /// Do not call this too frequently as it incurs a performance penalty.
  ///
  /// \param uiNumNewMsgThreshold
  ///   If this is set to a number larger than zero, the flush may be ignored if the given plLogInterface
  ///   has logged fewer than this many messages since the last flush.
  /// \param timeIntervalThreshold
  ///   The flush may be ignored if less time has past than this, since the last flush.
  ///
  /// If either enough messages have been logged, or the flush interval has been exceeded, the flush is executed.
  /// To force a flush, set \a uiNumNewMsgThreshold  to zero.
  /// However, a flush is always ignored if not a single message was logged in between.
  ///
  /// \return Returns true if the flush is executed.
  static bool Flush(plUInt32 uiNumNewMsgThreshold = 0, plTime timeIntervalThreshold = plTime::MakeFromSeconds(10), plLogInterface* pInterface = GetThreadLocalLogSystem());

  /// \brief Usually called internally by the other log functions, but can be called directly, if the message type is already known.
  /// pInterface must be != nullptr.
  static void BroadcastLoggingEvent(plLogInterface* pInterface, plLogMsgType::Enum type, plStringView sString);

  /// \brief Calls low-level OS functionality to print a string to the typical outputs, e.g. printf and OutputDebugString.
  ///
  /// Use this function to log unrecoverable errors like asserts, crash handlers etc.
  /// This function is meant for short term debugging when actual printing to the console is desired. Code using it should be temporary.
  /// This function flushes the output immediately, to ensure output is never lost during a crash. Consequently it has a high performance
  /// overhead.
  static void Print(const char* szText);

  /// \brief Calls low-level OS functionality to print a string to the typical outputs. Forwards to Print.
  /// \note This function uses actual printf formatting, not plFormatString syntax.
  /// \sa plLog::Print
  static void Printf(const char* szFormat, ...);

  /// \brief Signature of the custom print function used by plLog::SetCustomPrintFunction.
  using PrintFunction = void (*)(const char* szText);

  /// \brief Sets a custom function that is called in addition to the default behavior of plLog::Print.
  static void SetCustomPrintFunction(PrintFunction func);

  /// \brief Shows a simple message box using the OS functionality.
  ///
  /// This should only be used for critical information that can't be conveyed in another way.
  static void OsMessageBox(const plFormatString& text);

  /// \brief This enum is used in context of outputting timestamp information to indicate a formatting for said timestamps.
  enum class TimestampMode
  {
    None = 0,     ///< No timestamp will be added at all.
    Numeric = 1,  ///< A purely numeric timestamp will be added. Ex.: [2019-08-16 13:40:30.345 (UTC)] Log message.
    Textual = 2,  ///< A timestamp with textual fields will be added. Ex.: [2019 Aug 16 (Fri) 13:40:30.345 (UTC)] Log message.
    TimeOnly = 3, ///< A short timestamp (time only, no timplone indicator) is added. Ex: [13:40:30.345] Log message.
  };

  static void GenerateFormattedTimestamp(TimestampMode mode, plStringBuilder& ref_sTimestampOut);

private:
  // Needed to call 'EndLogBlock'
  friend class plLogBlock;

  /// \brief Which messages to filter out by default.
  static plLogMsgType::Enum s_DefaultLogLevel;

  /// \brief Ends grouping log messages.
  static void EndLogBlock(plLogInterface* pInterface, plLogBlock* pBlock);

  static void WriteBlockHeader(plLogInterface* pInterface, plLogBlock* pBlock);

  static PrintFunction s_CustomPrintFunction;
};


/// \brief Instances of this class will group messages in a scoped block together.
class PL_FOUNDATION_DLL plLogBlock
{
public:
  /// \brief Creates a named grouping block for log messages.
  ///
  /// Use the szContextInfo to pass in a string that can give additional context information (e.g. a file name).
  /// This string must point to valid memory until after the log block object is destroyed.
  /// Log writers get these strings provided through the plLoggingEventData::m_szTag variable.
  /// \note The log block header (and context info) will not be printed until a message is successfully logged,
  /// i.e. as long as all messages in this block are filtered out (via the LogLevel setting), the log block
  /// header will not be printed, to prevent spamming the log.
  ///
  /// This constructor will output the log block data to the plGlobalLog.
  plLogBlock(plStringView sName, plStringView sContextInfo = {});

  /// \brief Creates a named grouping block for log messages.
  ///
  /// This variant of the constructor takes an explicit plLogInterface to write the log messages to.
  plLogBlock(plLogInterface* pInterface, plStringView sName, plStringView sContextInfo = {});

  ~plLogBlock();

private:
  friend class plLog;

  plLogInterface* m_pLogInterface;
  plLogBlock* m_pParentBlock;
  plStringView m_sName;
  plStringView m_sContextInfo;
  plUInt8 m_uiBlockDepth;
  bool m_bWritten;
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  double m_fSeconds; // for profiling
#endif
};

/// \brief A class that sets a custom plLogInterface as the thread local default log system,
/// and resets the previous system when it goes out of scope.
class PL_FOUNDATION_DLL plLogSystemScope
{
public:
  /// \brief The given plLogInterface is passed to plLog::SetThreadLocalLogSystem().
  explicit plLogSystemScope(plLogInterface* pInterface)
  {
    m_pPrevious = plLog::GetThreadLocalLogSystem();
    plLog::SetThreadLocalLogSystem(pInterface);
  }

  /// \brief Resets the previous plLogInterface through plLog::SetThreadLocalLogSystem()
  ~plLogSystemScope() { plLog::SetThreadLocalLogSystem(m_pPrevious); }

protected:
  plLogInterface* m_pPrevious;

private:
  PL_DISALLOW_COPY_AND_ASSIGN(plLogSystemScope);
};


/// \brief A simple log interface implementation that gathers all messages in a string buffer.
class plLogSystemToBuffer : public plLogInterface
{
public:
  virtual void HandleLogMessage(const plLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case plLogMsgType::ErrorMsg:
        m_sBuffer.Append("Error: ", le.m_sText, "\n");
        break;
      case plLogMsgType::SeriousWarningMsg:
      case plLogMsgType::WarningMsg:
        m_sBuffer.Append("Warning: ", le.m_sText, "\n");
        break;
      case plLogMsgType::SuccessMsg:
      case plLogMsgType::InfoMsg:
      case plLogMsgType::DevMsg:
      case plLogMsgType::DebugMsg:
        m_sBuffer.Append(le.m_sText, "\n");
        break;
      default:
        break;
    }
  }

  plStringBuilder m_sBuffer;
};

#include <Foundation/Logging/Implementation/Log_inl.h>

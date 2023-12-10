#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_FOUNDATION_DLL, plLogMsgType);

/// \brief A persistent log entry created from a plLoggingEventData.
/// Allows for a log event to survive for longer than just the event
/// and is reflected, allowing for it to be sent to remote targets.
struct PLASMA_FOUNDATION_DLL plLogEntry
{
  plLogEntry();
  plLogEntry(const plLoggingEventData& le);

  plString m_sMsg;
  plString m_sTag;
  plEnum<plLogMsgType> m_Type;
  plUInt8 m_uiIndentation = 0;
  double m_fSeconds = 0;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_FOUNDATION_DLL, plLogEntry);

/// \brief A log interface implementation that converts a log event into
/// a plLogEntry and calls a delegate with it.
///
/// A typical use case is to re-route and store log messages in a scope:
/// \code{.cpp}
///   {
///     plLogEntryDelegate logger(([&array](plLogEntry& entry) -> void
///     {
///       array.PushBack(std::move(entry));
///     }));
///     plLogSystemScope logScope(&logger);
///     *log something*
///   }
/// \endcode
class PLASMA_FOUNDATION_DLL plLogEntryDelegate : public plLogInterface
{
public:
  using Callback = plDelegate<void(plLogEntry&)>;
  /// \brief Log events will be delegated to the given callback.
  plLogEntryDelegate(Callback callback, plLogMsgType::Enum logLevel = plLogMsgType::All);
  virtual void HandleLogMessage(const plLoggingEventData& le) override;

private:
  Callback m_Callback;
};

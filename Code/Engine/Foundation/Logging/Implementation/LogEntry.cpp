#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/LogEntry.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plLogMsgType, 1)
  PL_BITFLAGS_CONSTANTS(plLogMsgType::Flush, plLogMsgType::BeginGroup, plLogMsgType::EndGroup, plLogMsgType::None)
  PL_BITFLAGS_CONSTANTS(plLogMsgType::ErrorMsg, plLogMsgType::SeriousWarningMsg, plLogMsgType::WarningMsg, plLogMsgType::SuccessMsg, plLogMsgType::InfoMsg, plLogMsgType::DevMsg, plLogMsgType::DebugMsg, plLogMsgType::All)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_TYPE(plLogEntry, plNoBase, 1, plRTTIDefaultAllocator<plLogEntry>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Msg", m_sMsg),
    PL_MEMBER_PROPERTY("Tag", m_sTag),
    PL_ENUM_MEMBER_PROPERTY("Type", plLogMsgType, m_Type),
    PL_MEMBER_PROPERTY("Indentation", m_uiIndentation),
    PL_MEMBER_PROPERTY("Time", m_fSeconds),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

plLogEntry::plLogEntry() = default;

plLogEntry::plLogEntry(const plLoggingEventData& le)
{
  m_sMsg = le.m_sText;
  m_sTag = le.m_sTag;
  m_Type = le.m_EventType;
  m_uiIndentation = le.m_uiIndentation;
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = le.m_fSeconds;
#else
  m_fSeconds = 0.0f;
#endif
}

plLogEntryDelegate::plLogEntryDelegate(Callback callback, plLogMsgType::Enum logLevel)
  : m_Callback(callback)
{
  SetLogLevel(logLevel);
}

void plLogEntryDelegate::HandleLogMessage(const plLoggingEventData& le)
{
  plLogEntry e(le);
  m_Callback(e);
}

PL_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_LogEntry);

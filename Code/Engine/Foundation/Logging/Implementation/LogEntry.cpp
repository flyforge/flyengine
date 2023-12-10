#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/LogEntry.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plLogMsgType, 1)
  PLASMA_BITFLAGS_CONSTANTS(plLogMsgType::Flush, plLogMsgType::BeginGroup, plLogMsgType::EndGroup, plLogMsgType::None)
  PLASMA_BITFLAGS_CONSTANTS(plLogMsgType::ErrorMsg, plLogMsgType::SeriousWarningMsg, plLogMsgType::WarningMsg, plLogMsgType::SuccessMsg, plLogMsgType::InfoMsg, plLogMsgType::DevMsg, plLogMsgType::DebugMsg, plLogMsgType::All)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plLogEntry, plNoBase, 1, plRTTIDefaultAllocator<plLogEntry>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Msg", m_sMsg),
    PLASMA_MEMBER_PROPERTY("Tag", m_sTag),
    PLASMA_ENUM_MEMBER_PROPERTY("Type", plLogMsgType, m_Type),
    PLASMA_MEMBER_PROPERTY("Indentation", m_uiIndentation),
    PLASMA_MEMBER_PROPERTY("Time", m_fSeconds),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plLogEntry::plLogEntry() = default;

plLogEntry::plLogEntry(const plLoggingEventData& le)
{
  m_sMsg = le.m_sText;
  m_sTag = le.m_sTag;
  m_Type = le.m_EventType;
  m_uiIndentation = le.m_uiIndentation;
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
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

PLASMA_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_LogEntry);

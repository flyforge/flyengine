#pragma once

#include <Foundation/Logging/Log.h>

namespace plLogWriter
{

  /// \brief A simple log writer that outputs all log messages to the pl ETW provider.
  class PL_FOUNDATION_DLL ETW
  {
  public:
    /// \brief Register this at plLog to write all log messages to ETW.
    static void LogMessageHandler(const plLoggingEventData& eventData);

    /// \brief Log Message to ETW.
    static void LogMessage(plLogMsgType::Enum eventType, plUInt8 uiIndentation, plStringView sText);
  };
} // namespace plLogWriter

#pragma once

#include <Foundation/Logging/Log.h>

namespace plLogWriter
{
  /// \brief A simple log writer that writes out log messages using printf.
  class PL_FOUNDATION_DLL Console
  {
  public:
    /// \brief Register this at plLog to write all log messages to the console using printf.
    static void LogMessageHandler(const plLoggingEventData& eventData);

    /// \brief Allows to indicate in what form timestamps should be added to log messages.
    static void SetTimestampMode(plLog::TimestampMode mode);

  private:
    static plLog::TimestampMode s_TimestampMode;
  };
} // namespace plLogWriter

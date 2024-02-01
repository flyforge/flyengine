#pragma once

#include <Foundation/Logging/Log.h>

namespace plLogWriter
{

  /// \brief A simple log writer that outputs all log messages to visual studios output window
  class PL_FOUNDATION_DLL VisualStudio
  {
  public:
    /// \brief Register this at plLog to write all log messages to visual studios output window.
    static void LogMessageHandler(const plLoggingEventData& eventData);
  };
} // namespace plLogWriter

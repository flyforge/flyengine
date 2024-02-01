#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Logging/Log.h>

namespace plLogWriter
{
  /// \brief This log-writer will broadcast all messages through plTelemetry, such that external applications can display the log messages.
  class Telemetry
  {
  public:
    /// \brief Register this at plLog to broadcast all log messages through plTelemetry.
    static void LogMessageHandler(const plLoggingEventData& eventData)
    {
      plTelemetryMessage msg;
      msg.SetMessageID(' LOG', ' MSG');

      msg.GetWriter() << (plInt8)eventData.m_EventType;
      msg.GetWriter() << (plUInt8)eventData.m_uiIndentation;
      msg.GetWriter() << eventData.m_sTag;
      msg.GetWriter() << eventData.m_sText;

      if (eventData.m_EventType == plLogMsgType::EndGroup)
      {
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
        msg.GetWriter() << eventData.m_fSeconds;
#else
        msg.GetWriter() << 0.0f;
#endif
      }

      plTelemetry::Broadcast(plTelemetry::Reliable, msg);
    }
  };
} // namespace plLogWriter

void AddLogWriter()
{
  plGlobalLog::AddLogWriter(&plLogWriter::Telemetry::LogMessageHandler);
}

void RemoveLogWriter()
{
  plGlobalLog::RemoveLogWriter(&plLogWriter::Telemetry::LogMessageHandler);
}



PL_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Log);

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Plugin.h>

namespace PluginsDetail
{
  static void SendPluginTelemetry()
  {
    if (!plTelemetry::IsConnectedToClient())
      return;

    plTelemetry::Broadcast(plTelemetry::Reliable, 'PLUG', ' CLR', nullptr, 0);

    plHybridArray<plPlugin::PluginInfo, 16> infos;
    plPlugin::GetAllPluginInfos(infos);

    for (const auto& pi : infos)
    {
      plTelemetryMessage msg;
      msg.SetMessageID('PLUG', 'DATA');
      msg.GetWriter() << pi.m_sName;
      msg.GetWriter() << false; // deprecated 'IsReloadable' flag

      plStringBuilder s;

      for (const auto& dep : pi.m_sDependencies)
      {
        s.AppendWithSeparator(" | ", dep);
      }

      msg.GetWriter() << s;

      plTelemetry::Broadcast(plTelemetry::Reliable, msg);
    }
  }

  static void TelemetryEventsHandler(const plTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case plTelemetry::TelemetryEventData::ConnectedToClient:
        SendPluginTelemetry();
        break;

      default:
        break;
    }
  }

  static void PluginEventHandler(const plPluginEvent& e)
  {
    switch (e.m_EventType)
    {
      case plPluginEvent::AfterPluginChanges:
        SendPluginTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace PluginsDetail

void AddPluginEventHandler()
{
  plTelemetry::AddEventHandler(PluginsDetail::TelemetryEventsHandler);
  plPlugin::Events().AddEventHandler(PluginsDetail::PluginEventHandler);
}

void RemovePluginEventHandler()
{
  plPlugin::Events().RemoveEventHandler(PluginsDetail::PluginEventHandler);
  plTelemetry::RemoveEventHandler(PluginsDetail::TelemetryEventsHandler);
}



PL_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Plugins);

#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>

namespace StartupDetail
{
  static void SendSubsystemTelemetry();
  static plInt32 s_iSendSubSystemTelemetry = 0;
} // namespace StartupDetail

PL_ON_GLOBAL_EVENT(plStartup_StartupCoreSystems_End)
{
  StartupDetail::SendSubsystemTelemetry();
}

PL_ON_GLOBAL_EVENT(plStartup_StartupHighLevelSystems_End)
{
  StartupDetail::SendSubsystemTelemetry();
}

PL_ON_GLOBAL_EVENT(plStartup_ShutdownCoreSystems_End)
{
  StartupDetail::SendSubsystemTelemetry();
}

PL_ON_GLOBAL_EVENT(plStartup_ShutdownHighLevelSystems_End)
{
  StartupDetail::SendSubsystemTelemetry();
}

namespace StartupDetail
{
  static void SendSubsystemTelemetry()
  {
    if (s_iSendSubSystemTelemetry <= 0)
      return;

    plTelemetry::Broadcast(plTelemetry::Reliable, 'STRT', ' CLR', nullptr, 0);

    plSubSystem* pSub = plSubSystem::GetFirstInstance();

    while (pSub)
    {
      plTelemetryMessage msg;
      msg.SetMessageID('STRT', 'SYST');
      msg.GetWriter() << pSub->GetGroupName();
      msg.GetWriter() << pSub->GetSubSystemName();
      msg.GetWriter() << pSub->GetPluginName();

      for (plUInt32 i = 0; i < plStartupStage::ENUM_COUNT; ++i)
        msg.GetWriter() << pSub->IsStartupPhaseDone((plStartupStage::Enum)i);

      plUInt8 uiDependencies = 0;
      while (pSub->GetDependency(uiDependencies) != nullptr)
        ++uiDependencies;

      msg.GetWriter() << uiDependencies;

      for (plUInt8 i = 0; i < uiDependencies; ++i)
        msg.GetWriter() << pSub->GetDependency(i);

      plTelemetry::Broadcast(plTelemetry::Reliable, msg);

      pSub = pSub->GetNextInstance();
    }
  }

  static void TelemetryEventsHandler(const plTelemetry::TelemetryEventData& e)
  {
    if (!plTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case plTelemetry::TelemetryEventData::ConnectedToClient:
        SendSubsystemTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace StartupDetail

void AddStartupEventHandler()
{
  ++StartupDetail::s_iSendSubSystemTelemetry;
  plTelemetry::AddEventHandler(StartupDetail::TelemetryEventsHandler);
}

void RemoveStartupEventHandler()
{
  --StartupDetail::s_iSendSubSystemTelemetry;
  plTelemetry::RemoveEventHandler(StartupDetail::TelemetryEventsHandler);
}



PL_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Startup);

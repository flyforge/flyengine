#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Time/Clock.h>

static void TimeEventHandler(const plClock::EventData& e)
{
  if (!plTelemetry::IsConnectedToClient())
    return;

  plTelemetryMessage Msg;
  Msg.SetMessageID('TIME', 'UPDT');
  Msg.GetWriter() << e.m_sClockName;
  Msg.GetWriter() << plTime::Now();
  Msg.GetWriter() << e.m_RawTimeStep;
  Msg.GetWriter() << e.m_SmoothedTimeStep;

  plTelemetry::Broadcast(plTelemetry::Unreliable, Msg);
}

void AddTimeEventHandler()
{
  plClock::AddEventHandler(TimeEventHandler);
}

void RemoveTimeEventHandler()
{
  plClock::RemoveEventHandler(TimeEventHandler);
}



PLASMA_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Time);

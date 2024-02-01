#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>

#include <Core/GameApplication/GameApplicationBase.h>

static plGlobalEvent::EventMap s_LastState;

static void SendGlobalEventTelemetry(plStringView sEvent, const plGlobalEvent::EventData& ed)
{
  if (!plTelemetry::IsConnectedToClient())
    return;

  plTelemetryMessage msg;
  msg.SetMessageID('EVNT', 'DATA');
  msg.GetWriter() << sEvent;
  msg.GetWriter() << ed.m_uiNumTimesFired;
  msg.GetWriter() << ed.m_uiNumEventHandlersRegular;
  msg.GetWriter() << ed.m_uiNumEventHandlersOnce;

  plTelemetry::Broadcast(plTelemetry::Reliable, msg);
}

static void SendAllGlobalEventTelemetry()
{
  if (!plTelemetry::IsConnectedToClient())
    return;

  // clear
  {
    plTelemetryMessage msg;
    plTelemetry::Broadcast(plTelemetry::Reliable, 'EVNT', ' CLR', nullptr, 0);
  }

  plGlobalEvent::UpdateGlobalEventStatistics();

  s_LastState = plGlobalEvent::GetEventStatistics();

  for (plGlobalEvent::EventMap::ConstIterator it = s_LastState.GetIterator(); it.IsValid(); ++it)
  {
    SendGlobalEventTelemetry(it.Key(), it.Value());
  }
}

static void SendChangedGlobalEventTelemetry()
{
  if (!plTelemetry::IsConnectedToClient())
    return;

  static plTime LastUpdate = plTime::Now();

  if ((plTime::Now() - LastUpdate).GetSeconds() < 0.5)
    return;

  LastUpdate = plTime::Now();

  plGlobalEvent::UpdateGlobalEventStatistics();

  const plGlobalEvent::EventMap& data = plGlobalEvent::GetEventStatistics();

  if (data.GetCount() != s_LastState.GetCount())
  {
    SendAllGlobalEventTelemetry();
    return;
  }

  for (plGlobalEvent::EventMap::ConstIterator it = data.GetIterator(); it.IsValid(); ++it)
  {
    const plGlobalEvent::EventData& currentEventData = it.Value();
    plGlobalEvent::EventData& lastEventData = s_LastState[it.Key()];

    if (plMemoryUtils::Compare(&currentEventData, &lastEventData) != 0)
    {
      SendGlobalEventTelemetry(it.Key().GetData(), it.Value());

      lastEventData = currentEventData;
    }
  }
}

namespace GlobalEventsDetail
{
  static void TelemetryEventsHandler(const plTelemetry::TelemetryEventData& e)
  {
    if (!plTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case plTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllGlobalEventTelemetry();
        break;

      default:
        break;
    }
  }

  static void PerframeUpdateHandler(const plGameApplicationExecutionEvent& e)
  {
    if (!plTelemetry::IsConnectedToClient())
      return;

    switch (e.m_Type)
    {
      case plGameApplicationExecutionEvent::Type::AfterPresent:
        SendChangedGlobalEventTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace GlobalEventsDetail

void AddGlobalEventHandler()
{
  plTelemetry::AddEventHandler(GlobalEventsDetail::TelemetryEventsHandler);

  // We're handling the per frame update by a different event since
  // using plTelemetry::TelemetryEventData::PerFrameUpdate can lead
  // to deadlocks between the plStats and plTelemetry system.
  if (plGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(GlobalEventsDetail::PerframeUpdateHandler);
  }
}

void RemoveGlobalEventHandler()
{
  if (plGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(GlobalEventsDetail::PerframeUpdateHandler);
  }

  plTelemetry::RemoveEventHandler(GlobalEventsDetail::TelemetryEventsHandler);
}



PL_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_GlobalEvents);

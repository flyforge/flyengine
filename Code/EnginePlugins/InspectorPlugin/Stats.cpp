#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Utilities/Stats.h>

static void StatsEventHandler(const plStats::StatsEventData& e)
{
  if (!plTelemetry::IsConnectedToClient())
    return;

  plTelemetry::TransmitMode Mode = plTelemetry::Reliable;

  switch (e.m_EventType)
  {
    case plStats::StatsEventData::Set:
      Mode = plTelemetry::Unreliable;
      // fall-through
    case plStats::StatsEventData::Add:
    {
      plTelemetryMessage msg;
      msg.SetMessageID('STAT', ' SET');
      msg.GetWriter() << e.m_sStatName;
      msg.GetWriter() << e.m_NewStatValue;
      msg.GetWriter() << plTime::Now();

      plTelemetry::Broadcast(Mode, msg);
    }
    break;
    case plStats::StatsEventData::Remove:
    {
      plTelemetryMessage msg;
      msg.SetMessageID('STAT', ' DEL');
      msg.GetWriter() << e.m_sStatName;
      msg.GetWriter() << plTime::Now();

      plTelemetry::Broadcast(plTelemetry::Reliable, msg);
    }
    break;
  }
}


static void SendAllStatsTelemetry()
{
  if (!plTelemetry::IsConnectedToClient())
    return;

  for (plStats::MapType::ConstIterator it = plStats::GetAllStats().GetIterator(); it.IsValid(); ++it)
  {
    plTelemetryMessage msg;
    msg.SetMessageID('STAT', ' SET');
    msg.GetWriter() << it.Key().GetData();
    msg.GetWriter() << it.Value();

    plTelemetry::Broadcast(plTelemetry::Reliable, msg);
  }
}

static void TelemetryEventsHandler(const plTelemetry::TelemetryEventData& e)
{
  switch (e.m_EventType)
  {
    case plTelemetry::TelemetryEventData::ConnectedToClient:
      SendAllStatsTelemetry();
      break;

    default:
      break;
  }
}

static void PerFrameUpdateHandler(const plGameApplicationExecutionEvent& e)
{
  switch (e.m_Type)
  {
    case plGameApplicationExecutionEvent::Type::AfterPresent:
    {
      plTime FrameTime;

      if (plGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
      {
        FrameTime = plGameApplicationBase::GetGameApplicationBaseInstance()->GetFrameTime();
      }

      plStringBuilder s;
      plStats::SetStat("App/FrameTime[ms]", FrameTime.GetMilliseconds());
      plStats::SetStat("App/FPS", 1.0 / FrameTime.GetSeconds());

      plStats::SetStat("App/Active Threads", plOSThread::GetThreadCount());

      // Tasksystem Thread Utilization
      {
        for (plUInt32 t = 0; t < plTaskSystem::GetWorkerThreadCount(plWorkerThreadType::ShortTasks); ++t)
        {
          plUInt32 uiNumTasks = 0;
          const double Utilization = plTaskSystem::GetThreadUtilization(plWorkerThreadType::ShortTasks, t, &uiNumTasks);

          s.SetFormat("Utilization/Short{0}_Load[%%]", plArgI(t, 2, true));
          plStats::SetStat(s.GetData(), Utilization * 100.0);

          s.SetFormat("Utilization/Short{0}_Tasks", plArgI(t, 2, true));
          plStats::SetStat(s.GetData(), uiNumTasks);
        }

        for (plUInt32 t = 0; t < plTaskSystem::GetWorkerThreadCount(plWorkerThreadType::LongTasks); ++t)
        {
          plUInt32 uiNumTasks = 0;
          const double Utilization = plTaskSystem::GetThreadUtilization(plWorkerThreadType::LongTasks, t, &uiNumTasks);

          s.SetFormat("Utilization/Long{0}_Load[%%]", plArgI(t, 2, true));
          plStats::SetStat(s.GetData(), Utilization * 100.0);

          s.SetFormat("Utilization/Long{0}_Tasks", plArgI(t, 2, true));
          plStats::SetStat(s.GetData(), uiNumTasks);
        }

        for (plUInt32 t = 0; t < plTaskSystem::GetWorkerThreadCount(plWorkerThreadType::FileAccess); ++t)
        {
          plUInt32 uiNumTasks = 0;
          const double Utilization = plTaskSystem::GetThreadUtilization(plWorkerThreadType::FileAccess, t, &uiNumTasks);

          s.SetFormat("Utilization/File{0}_Load[%%]", plArgI(t, 2, true));
          plStats::SetStat(s.GetData(), Utilization * 100.0);

          s.SetFormat("Utilization/File{0}_Tasks", plArgI(t, 2, true));
          plStats::SetStat(s.GetData(), uiNumTasks);
        }
      }
    }
    break;

    default:
      break;
  }
}

void AddStatsEventHandler()
{
  plStats::AddEventHandler(StatsEventHandler);

  plTelemetry::AddEventHandler(TelemetryEventsHandler);

  // We're handling the per frame update by a different event since
  // using plTelemetry::TelemetryEventData::PerFrameUpdate can lead
  // to deadlocks between the plStats and plTelemetry system.
  if (plGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(PerFrameUpdateHandler);
  }
}

void RemoveStatsEventHandler()
{
  if (plGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(PerFrameUpdateHandler);
  }

  plTelemetry::RemoveEventHandler(TelemetryEventsHandler);

  plStats::RemoveEventHandler(StatsEventHandler);
}



PL_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Stats);

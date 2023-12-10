#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Utilities/Stats.h>

#include <Core/GameApplication/GameApplicationBase.h>

namespace MemoryDetail
{

  static void BroadcastMemoryStats()
  {
    plUInt64 uiTotalAllocations = 0;
    plUInt64 uiTotalPerFrameAllocationSize = 0;
    plTime TotalPerFrameAllocationTime;

    {
      plTelemetryMessage msg;
      msg.SetMessageID(' MEM', 'BGN');
      plTelemetry::Broadcast(plTelemetry::Unreliable, msg);
    }

    for (auto it = plMemoryTracker::GetIterator(); it.IsValid(); ++it)
    {
      plTelemetryMessage msg;
      msg.SetMessageID(' MEM', 'STAT');
      msg.GetWriter() << it.Id().m_Data;
      msg.GetWriter() << it.Name();
      msg.GetWriter() << (it.ParentId().IsInvalidated() ? plInvalidIndex : it.ParentId().m_Data);
      msg.GetWriter() << it.Stats();

      uiTotalAllocations += it.Stats().m_uiNumAllocations;
      uiTotalPerFrameAllocationSize += it.Stats().m_uiPerFrameAllocationSize;
      TotalPerFrameAllocationTime += it.Stats().m_PerFrameAllocationTime;

      plTelemetry::Broadcast(plTelemetry::Unreliable, msg);
    }

    {
      plTelemetryMessage msg;
      msg.SetMessageID(' MEM', 'END');
      plTelemetry::Broadcast(plTelemetry::Unreliable, msg);
    }

    static plUInt64 uiLastTotalAllocations = 0;

    plStats::SetStat("App/Allocs Per Frame", uiTotalAllocations - uiLastTotalAllocations);
    plStats::SetStat("App/Per Frame Alloc Size (byte)", uiTotalPerFrameAllocationSize);
    plStats::SetStat("App/Per Frame Alloc Time", TotalPerFrameAllocationTime);

    uiLastTotalAllocations = uiTotalAllocations;

    plMemoryTracker::ResetPerFrameAllocatorStats();
  }

  static void PerframeUpdateHandler(const plGameApplicationExecutionEvent& e)
  {
    if (!plTelemetry::IsConnectedToClient())
      return;

    switch (e.m_Type)
    {
      case plGameApplicationExecutionEvent::Type::AfterPresent:
        BroadcastMemoryStats();
        break;

      default:
        break;
    }
  }
} // namespace MemoryDetail


void AddMemoryEventHandler()
{
  // We're handling the per frame update by a different event since
  // using plTelemetry::TelemetryEventData::PerFrameUpdate can lead
  // to deadlocks between the plStats and plTelemetry system.
  if (plGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(MemoryDetail::PerframeUpdateHandler);
  }
}

void RemoveMemoryEventHandler()
{
  if (plGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(MemoryDetail::PerframeUpdateHandler);
  }
}



PLASMA_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Memory);

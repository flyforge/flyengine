#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Communication/Telemetry.h>

namespace ResourceManagerDetail
{

  static void SendFullResourceInfo(const plResource* pRes)
  {
    plTelemetryMessage Msg;

    Msg.SetMessageID('RESM', ' SET');

    Msg.GetWriter() << pRes->GetResourceIDHash();
    Msg.GetWriter() << pRes->GetResourceID();
    Msg.GetWriter() << pRes->GetDynamicRTTI()->GetTypeName();
    Msg.GetWriter() << static_cast<plUInt8>(pRes->GetPriority());
    Msg.GetWriter() << static_cast<plUInt8>(pRes->GetBaseResourceFlags().GetValue());
    Msg.GetWriter() << static_cast<plUInt8>(pRes->GetLoadingState());
    Msg.GetWriter() << pRes->GetNumQualityLevelsDiscardable();
    Msg.GetWriter() << pRes->GetNumQualityLevelsLoadable();
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryCPU;
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryGPU;
    Msg.GetWriter() << pRes->GetResourceDescription();

    plTelemetry::Broadcast(plTelemetry::Reliable, Msg);
  }

  static void SendSmallResourceInfo(const plResource* pRes)
  {
    plTelemetryMessage Msg;

    Msg.SetMessageID('RESM', 'UPDT');

    Msg.GetWriter() << pRes->GetResourceIDHash();
    Msg.GetWriter() << static_cast<plUInt8>(pRes->GetPriority());
    Msg.GetWriter() << static_cast<plUInt8>(pRes->GetBaseResourceFlags().GetValue());
    Msg.GetWriter() << static_cast<plUInt8>(pRes->GetLoadingState());
    Msg.GetWriter() << pRes->GetNumQualityLevelsDiscardable();
    Msg.GetWriter() << pRes->GetNumQualityLevelsLoadable();
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryCPU;
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryGPU;

    plTelemetry::Broadcast(plTelemetry::Reliable, Msg);
  }

  static void SendDeleteResourceInfo(const plResource* pRes)
  {
    plTelemetryMessage Msg;

    Msg.SetMessageID('RESM', ' DEL');

    Msg.GetWriter() << pRes->GetResourceIDHash();

    plTelemetry::Broadcast(plTelemetry::Reliable, Msg);
  }

  static void SendAllResourceTelemetry() { plResourceManager::BroadcastExistsEvent(); }

  static void TelemetryEventsHandler(const plTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case plTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllResourceTelemetry();
        break;

      default:
        break;
    }
  }

  static void ResourceManagerEventHandler(const plResourceEvent& e)
  {
    if (!plTelemetry::IsConnectedToClient())
      return;

    switch (e.m_Type)
    {
      case plResourceEvent::Type::ResourceCreated:
      case plResourceEvent::Type::ResourceExists:
        SendFullResourceInfo(e.m_pResource);
        return;

      case plResourceEvent::Type::ResourceDeleted:
        SendDeleteResourceInfo(e.m_pResource);
        return;

      case plResourceEvent::Type::ResourceContentUpdated:
      case plResourceEvent::Type::ResourceContentUnloading:
      case plResourceEvent::Type::ResourcePriorityChanged:
        SendSmallResourceInfo(e.m_pResource);
        return;

      default:
        PL_ASSERT_NOT_IMPLEMENTED;
    }
  }
} // namespace ResourceManagerDetail

void AddResourceManagerEventHandler()
{
  plTelemetry::AddEventHandler(ResourceManagerDetail::TelemetryEventsHandler);
  plResourceManager::GetResourceEvents().AddEventHandler(ResourceManagerDetail::ResourceManagerEventHandler);
}

void RemoveResourceManagerEventHandler()
{
  plResourceManager::GetResourceEvents().RemoveEventHandler(ResourceManagerDetail::ResourceManagerEventHandler);
  plTelemetry::RemoveEventHandler(ResourceManagerDetail::TelemetryEventsHandler);
}

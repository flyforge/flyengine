#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Reflection/Reflection.h>

namespace ReflectionDetail
{

  static void SendBasicTypesGroup()
  {
    plTelemetryMessage msg;
    msg.SetMessageID('RFLC', 'DATA');
    msg.GetWriter() << "Basic Types";
    msg.GetWriter() << "";
    msg.GetWriter() << 0;
    msg.GetWriter() << "";
    msg.GetWriter() << (plUInt32)0U;
    msg.GetWriter() << (plUInt32)0U;

    plTelemetry::Broadcast(plTelemetry::Reliable, msg);
  }

  static plStringView GetParentType(const plRTTI* pRTTI)
  {
    if (pRTTI->GetParentType())
    {
      return pRTTI->GetParentType()->GetTypeName();
    }

    if ((pRTTI->GetTypeName() == "bool") || (pRTTI->GetTypeName() == "float") ||
        (pRTTI->GetTypeName() == "double") || (pRTTI->GetTypeName() == "plInt8") ||
        (pRTTI->GetTypeName() == "plUInt8") || (pRTTI->GetTypeName() == "plInt16") ||
        (pRTTI->GetTypeName() == "plUInt16") || (pRTTI->GetTypeName() == "plInt32") ||
        (pRTTI->GetTypeName() == "plUInt32") || (pRTTI->GetTypeName() == "plInt64") ||
        (pRTTI->GetTypeName() == "plUInt64") || (pRTTI->GetTypeName() == "plConstCharPtr") ||
        (pRTTI->GetTypeName() == "plVec2") || (pRTTI->GetTypeName() == "plVec3") ||
        (pRTTI->GetTypeName() == "plVec4") || (pRTTI->GetTypeName() == "plMat3") ||
        (pRTTI->GetTypeName() == "plMat4") || (pRTTI->GetTypeName() == "plTime") ||
        (pRTTI->GetTypeName() == "plUuid") || (pRTTI->GetTypeName() == "plColor") ||
        (pRTTI->GetTypeName() == "plVariant") || (pRTTI->GetTypeName() == "plQuat"))
    {
      return "Basic Types";
    }

    return {};
  }

  static void SendReflectionTelemetry(const plRTTI* pRTTI)
  {
    plTelemetryMessage msg;
    msg.SetMessageID('RFLC', 'DATA');
    msg.GetWriter() << pRTTI->GetTypeName();
    msg.GetWriter() << GetParentType(pRTTI);
    msg.GetWriter() << pRTTI->GetTypeSize();
    msg.GetWriter() << pRTTI->GetPluginName();

    {
      auto properties = pRTTI->GetProperties();

      msg.GetWriter() << properties.GetCount();

      for (auto& prop : properties)
      {
        msg.GetWriter() << prop->GetPropertyName();
        msg.GetWriter() << (plInt8)prop->GetCategory();

        const plRTTI* pType = prop->GetSpecificType();
        msg.GetWriter() << (pType ? pType->GetTypeName() : "<Unknown Type>");
      }
    }

    {
      const plArrayPtr<plAbstractMessageHandler*>& Messages = pRTTI->GetMessageHandlers();

      msg.GetWriter() << Messages.GetCount();

      for (plUInt32 i = 0; i < Messages.GetCount(); ++i)
      {
        msg.GetWriter() << Messages[i]->GetMessageId();
      }
    }

    plTelemetry::Broadcast(plTelemetry::Reliable, msg);
  }

  static void SendAllReflectionTelemetry()
  {
    if (!plTelemetry::IsConnectedToClient())
      return;

    // clear
    {
      plTelemetryMessage msg;
      plTelemetry::Broadcast(plTelemetry::Reliable, 'RFLC', ' CLR', nullptr, 0);
    }

    SendBasicTypesGroup();

    plRTTI::ForEachType([](const plRTTI* pRtti) { SendReflectionTelemetry(pRtti); });
  }


  static void TelemetryEventsHandler(const plTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case plTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllReflectionTelemetry();
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
        SendAllReflectionTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace ReflectionDetail

void AddReflectionEventHandler()
{
  plTelemetry::AddEventHandler(ReflectionDetail::TelemetryEventsHandler);

  plPlugin::Events().AddEventHandler(ReflectionDetail::PluginEventHandler);
}

void RemoveReflectionEventHandler()
{
  plPlugin::Events().RemoveEventHandler(ReflectionDetail::PluginEventHandler);

  plTelemetry::RemoveEventHandler(ReflectionDetail::TelemetryEventsHandler);
}



PL_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Reflection);

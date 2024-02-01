#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>

static void TelemetryMessage(void* pPassThrough)
{
  plTelemetryMessage Msg;

  while (plTelemetry::RetrieveMessage('SVAR', Msg) == PL_SUCCESS)
  {
    if (Msg.GetMessageID() == ' SET')
    {
      plString sCVar;
      plUInt8 uiType;

      float fValue;
      plInt32 iValue;
      bool bValue;
      plString sValue;

      Msg.GetReader() >> sCVar;
      Msg.GetReader() >> uiType;

      switch (uiType)
      {
        case plCVarType::Float:
          Msg.GetReader() >> fValue;
          break;
        case plCVarType::Int:
          Msg.GetReader() >> iValue;
          break;
        case plCVarType::Bool:
          Msg.GetReader() >> bValue;
          break;
        case plCVarType::String:
          Msg.GetReader() >> sValue;
          break;
      }

      plCVar* pCVar = plCVar::GetFirstInstance();

      while (pCVar)
      {
        if (((plUInt8)pCVar->GetType() == uiType) && (pCVar->GetName() == sCVar))
        {
          switch (uiType)
          {
            case plCVarType::Float:
              *((plCVarFloat*)pCVar) = fValue;
              break;
            case plCVarType::Int:
              *((plCVarInt*)pCVar) = iValue;
              break;
            case plCVarType::Bool:
              *((plCVarBool*)pCVar) = bValue;
              break;
            case plCVarType::String:
              *((plCVarString*)pCVar) = sValue;
              break;
          }
        }

        pCVar = pCVar->GetNextInstance();
      }
    }
  }
}

static void SendCVarTelemetry(plCVar* pCVar)
{
  plTelemetryMessage msg;
  msg.SetMessageID('CVAR', 'DATA');
  msg.GetWriter() << pCVar->GetName();
  msg.GetWriter() << pCVar->GetPluginName();
  // msg.GetWriter() << (plUInt8) pCVar->GetFlags().GetValue(); // currently not used
  msg.GetWriter() << (plUInt8)pCVar->GetType();
  msg.GetWriter() << pCVar->GetDescription();

  switch (pCVar->GetType())
  {
    case plCVarType::Float:
    {
      const float val = ((plCVarFloat*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;
    case plCVarType::Int:
    {
      const int val = ((plCVarInt*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;
    case plCVarType::Bool:
    {
      const bool val = ((plCVarBool*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;
    case plCVarType::String:
    {
      plStringView val = ((plCVarString*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;

    case plCVarType::ENUM_COUNT:
      PL_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  plTelemetry::Broadcast(plTelemetry::Reliable, msg);
}

static void SendAllCVarTelemetry()
{
  if (!plTelemetry::IsConnectedToClient())
    return;

  // clear
  {
    plTelemetryMessage msg;
    plTelemetry::Broadcast(plTelemetry::Reliable, 'CVAR', ' CLR', nullptr, 0);
  }

  plCVar* pCVar = plCVar::GetFirstInstance();

  while (pCVar)
  {
    SendCVarTelemetry(pCVar);

    pCVar = pCVar->GetNextInstance();
  }

  {
    plTelemetryMessage msg;
    plTelemetry::Broadcast(plTelemetry::Reliable, 'CVAR', 'SYNC', nullptr, 0);
  }
}

namespace CVarsDetail
{

  static void TelemetryEventsHandler(const plTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case plTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllCVarTelemetry();
        break;

      default:
        break;
    }
  }

  static void CVarEventHandler(const plCVarEvent& e)
  {
    if (!plTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case plCVarEvent::ValueChanged:
        SendCVarTelemetry(e.m_pCVar);
        break;

      case plCVarEvent::ListOfVarsChanged:
        SendAllCVarTelemetry();
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
        SendAllCVarTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace CVarsDetail

void AddCVarEventHandler()
{
  plTelemetry::AddEventHandler(CVarsDetail::TelemetryEventsHandler);
  plTelemetry::AcceptMessagesForSystem('SVAR', true, TelemetryMessage, nullptr);

  plCVar::s_AllCVarEvents.AddEventHandler(CVarsDetail::CVarEventHandler);
  plPlugin::Events().AddEventHandler(CVarsDetail::PluginEventHandler);
}

void RemoveCVarEventHandler()
{
  plPlugin::Events().RemoveEventHandler(CVarsDetail::PluginEventHandler);
  plCVar::s_AllCVarEvents.RemoveEventHandler(CVarsDetail::CVarEventHandler);

  plTelemetry::RemoveEventHandler(CVarsDetail::TelemetryEventsHandler);
  plTelemetry::AcceptMessagesForSystem('SVAR', false);
}



PL_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_CVars);

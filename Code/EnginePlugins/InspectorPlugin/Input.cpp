#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Communication/Telemetry.h>

namespace InputDetail
{

  static void SendInputSlotData(plStringView sInputSlot)
  {
    float fValue = 0.0f;

    plTelemetryMessage msg;
    msg.SetMessageID('INPT', 'SLOT');
    msg.GetWriter() << sInputSlot;
    msg.GetWriter() << plInputManager::GetInputSlotFlags(sInputSlot).GetValue();
    msg.GetWriter() << (plUInt8)plInputManager::GetInputSlotState(sInputSlot, &fValue);
    msg.GetWriter() << fValue;
    msg.GetWriter() << plInputManager::GetInputSlotDeadZone(sInputSlot);

    plTelemetry::Broadcast(plTelemetry::Reliable, msg);
  }

  static void SendInputActionData(plStringView sInputSet, plStringView sInputAction)
  {
    float fValue = 0.0f;

    const plInputActionConfig cfg = plInputManager::GetInputActionConfig(sInputSet, sInputAction);

    plTelemetryMessage msg;
    msg.SetMessageID('INPT', 'ACTN');
    msg.GetWriter() << sInputSet;
    msg.GetWriter() << sInputAction;
    msg.GetWriter() << (plUInt8)plInputManager::GetInputActionState(sInputSet, sInputAction, &fValue);
    msg.GetWriter() << fValue;
    msg.GetWriter() << cfg.m_bApplyTimeScaling;

    for (plUInt32 i = 0; i < plInputActionConfig::MaxInputSlotAlternatives; ++i)
    {
      msg.GetWriter() << cfg.m_sInputSlotTrigger[i];
      msg.GetWriter() << cfg.m_fInputSlotScale[i];
    }

    plTelemetry::Broadcast(plTelemetry::Reliable, msg);
  }

  static void SendAllInputSlots()
  {
    plDynamicArray<plStringView> InputSlots;
    plInputManager::RetrieveAllKnownInputSlots(InputSlots);

    for (plUInt32 i = 0; i < InputSlots.GetCount(); ++i)
    {
      SendInputSlotData(InputSlots[i]);
    }
  }

  static void SendAllInputActions()
  {
    plDynamicArray<plString> InputSetNames;
    plInputManager::GetAllInputSets(InputSetNames);

    for (plUInt32 s = 0; s < InputSetNames.GetCount(); ++s)
    {
      plHybridArray<plString, 24> InputActions;

      plInputManager::GetAllInputActions(InputSetNames[s].GetData(), InputActions);

      for (plUInt32 a = 0; a < InputActions.GetCount(); ++a)
        SendInputActionData(InputSetNames[s].GetData(), InputActions[a].GetData());
    }
  }

  static void TelemetryEventsHandler(const plTelemetry::TelemetryEventData& e)
  {
    if (!plTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case plTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllInputSlots();
        SendAllInputActions();
        break;

      default:
        break;
    }
  }

  static void InputManagerEventHandler(const plInputManager::InputEventData& e)
  {
    if (!plTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case plInputManager::InputEventData::InputActionChanged:
        SendInputActionData(e.m_sInputSet, e.m_sInputAction);
        break;
      case plInputManager::InputEventData::InputSlotChanged:
        SendInputSlotData(e.m_sInputSlot);
        break;

      default:
        break;
    }
  }
} // namespace InputDetail

void AddInputEventHandler()
{
  plTelemetry::AddEventHandler(InputDetail::TelemetryEventsHandler);
  plInputManager::AddEventHandler(InputDetail::InputManagerEventHandler);
}

void RemoveInputEventHandler()
{
  plInputManager::RemoveEventHandler(InputDetail::InputManagerEventHandler);
  plTelemetry::RemoveEventHandler(InputDetail::TelemetryEventsHandler);
}



PLASMA_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Input);

#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

plInputManager::plEventInput plInputManager::s_InputEvents;
plInputManager::InternalData* plInputManager::s_pData = nullptr;
plUInt32 plInputManager::s_uiLastCharacter = '\0';
bool plInputManager::s_bInputSlotResetRequired = true;
plString plInputManager::s_sExclusiveInputSet;

plInputManager::InternalData& plInputManager::GetInternals()
{
  if (s_pData == nullptr)
    s_pData = PLASMA_DEFAULT_NEW(InternalData);

  return *s_pData;
}

void plInputManager::DeallocateInternals()
{
  PLASMA_DEFAULT_DELETE(s_pData);
}

plInputManager::plInputSlot::plInputSlot()
{
  m_fValue = 0.0f;
  m_State = plKeyState::Up;
  m_fDeadZone = 0.0f;
}

void plInputManager::RegisterInputSlot(plStringView sInputSlot, plStringView sDefaultDisplayName, plBitflags<plInputSlotFlags> SlotFlags)
{
  plMap<plString, plInputSlot>::Iterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
  {
    if (it.Value().m_SlotFlags != SlotFlags)
    {
      if ((it.Value().m_SlotFlags != plInputSlotFlags::Default) && (SlotFlags != plInputSlotFlags::Default))
      {
        plStringBuilder tmp, tmp2;
        tmp.Printf("Different devices register Input Slot '%s' with different Slot Flags: %16b vs. %16b",
          sInputSlot.GetData(tmp2), it.Value().m_SlotFlags.GetValue(), SlotFlags.GetValue());

        plLog::Warning(tmp);
      }

      it.Value().m_SlotFlags |= SlotFlags;
    }

    // If the key already exists, but key and display string are identical, then overwrite the display string with the incoming string
    if (it.Value().m_sDisplayName != it.Key())
      return;
  }

  // plLog::Debug("Registered Input Slot: '{0}'", sInputSlot);

  plInputSlot& sm = GetInternals().s_InputSlots[sInputSlot];

  sm.m_sDisplayName = sDefaultDisplayName;
  sm.m_SlotFlags = SlotFlags;

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_sInputSlot = sInputSlot;

  s_InputEvents.Broadcast(e);
}

plBitflags<plInputSlotFlags> plInputManager::GetInputSlotFlags(plStringView sInputSlot)
{
  plMap<plString, plInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
    return it.Value().m_SlotFlags;

  plLog::Warning("plInputManager::GetInputSlotFlags: Input Slot '{0}' does not exist (yet).", sInputSlot);

  return plInputSlotFlags::Default;
}

void plInputManager::SetInputSlotDisplayName(plStringView sInputSlot, plStringView sDefaultDisplayName)
{
  RegisterInputSlot(sInputSlot, sDefaultDisplayName, plInputSlotFlags::Default);
  GetInternals().s_InputSlots[sInputSlot].m_sDisplayName = sDefaultDisplayName;

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_sInputSlot = sInputSlot;

  s_InputEvents.Broadcast(e);
}

plStringView plInputManager::GetInputSlotDisplayName(plStringView sInputSlot)
{
  plMap<plString, plInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
    return it.Value().m_sDisplayName.GetData();

  plLog::Warning("plInputManager::GetInputSlotDisplayName: Input Slot '{0}' does not exist (yet).", sInputSlot);
  return sInputSlot;
}

plStringView plInputManager::GetInputSlotDisplayName(plStringView sInputSet, plStringView sAction, plInt32 iTrigger)
{
  /// \test This is new

  const auto cfg = GetInputActionConfig(sInputSet, sAction);

  if (iTrigger < 0)
  {
    for (iTrigger = 0; iTrigger < plInputActionConfig::MaxInputSlotAlternatives; ++iTrigger)
    {
      if (!cfg.m_sInputSlotTrigger[iTrigger].IsEmpty())
        break;
    }
  }

  if (iTrigger >= plInputActionConfig::MaxInputSlotAlternatives)
    return nullptr;

  return GetInputSlotDisplayName(cfg.m_sInputSlotTrigger[iTrigger]);
}

void plInputManager::SetInputSlotDeadZone(plStringView sInputSlot, float fDeadZone)
{
  RegisterInputSlot(sInputSlot, sInputSlot, plInputSlotFlags::Default);
  GetInternals().s_InputSlots[sInputSlot].m_fDeadZone = plMath::Max(fDeadZone, 0.0001f);

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_sInputSlot = sInputSlot;

  s_InputEvents.Broadcast(e);
}

float plInputManager::GetInputSlotDeadZone(plStringView sInputSlot)
{
  plMap<plString, plInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
    return it.Value().m_fDeadZone;

  plLog::Warning("plInputManager::GetInputSlotDeadZone: Input Slot '{0}' does not exist (yet).", sInputSlot);

  plInputSlot s;
  return s.m_fDeadZone; // return the default value
}

plKeyState::Enum plInputManager::GetInputSlotState(plStringView sInputSlot, float* pValue)
{
  plMap<plString, plInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
  {
    if (pValue)
      *pValue = it.Value().m_fValue;

    return it.Value().m_State;
  }

  if (pValue)
    *pValue = 0.0f;

  plLog::Warning("plInputManager::GetInputSlotState: Input Slot '{0}' does not exist (yet). To ensure all devices are initialized, call "
                 "plInputManager::Update before querying device states, or at least call plInputManager::PollHardware.",
    sInputSlot);

  RegisterInputSlot(sInputSlot, sInputSlot, plInputSlotFlags::None);

  return plKeyState::Up;
}

void plInputManager::PollHardware()
{
  if (s_bInputSlotResetRequired)
  {
    s_bInputSlotResetRequired = false;
    ResetInputSlotValues();
  }

  plInputDevice::UpdateAllDevices();

  GatherDeviceInputSlotValues();
}

void plInputManager::Update(plTime timeDifference)
{
  PollHardware();

  UpdateInputSlotStates();

  s_uiLastCharacter = plInputDevice::RetrieveLastCharacterFromAllDevices();

  UpdateInputActions(timeDifference);

  plInputDevice::ResetAllDevices();

  plInputDevice::UpdateAllHardwareStates(timeDifference);

  s_bInputSlotResetRequired = true;
}

void plInputManager::ResetInputSlotValues()
{
  // set all input slot values to zero
  // this is crucial for accumulating the new values and for resetting the input state later
  for (plInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    it.Value().m_fValue = 0.0f;
  }
}

void plInputManager::GatherDeviceInputSlotValues()
{
  for (plInputDevice* pDevice = plInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->m_bGeneratedInputRecently = false;

    // iterate over all the input slots that this device provides
    for (auto it = pDevice->m_InputSlotValues.GetIterator(); it.IsValid(); it.Next())
    {
      if (it.Value() > 0.0f)
      {
        plInputManager::plInputSlot& Slot = GetInternals().s_InputSlots[it.Key()];

        // do not store a value larger than 0 unless it exceeds the dead-zone threshold
        if (it.Value() > Slot.m_fDeadZone)
        {
          Slot.m_fValue = plMath::Max(Slot.m_fValue, it.Value()); // 'accumulate' the values for one slot from all the connected devices

          pDevice->m_bGeneratedInputRecently = true;
        }
      }
    }
  }

  plMap<plString, float>::Iterator it = GetInternals().s_InjectedInputSlots.GetIterator();

  for (; it.IsValid(); ++it)
  {
    plInputManager::plInputSlot& Slot = GetInternals().s_InputSlots[it.Key()];

    // do not store a value larger than 0 unless it exceeds the dead-zone threshold
    if (it.Value() > Slot.m_fDeadZone)
      Slot.m_fValue = plMath::Max(Slot.m_fValue, it.Value()); // 'accumulate' the values for one slot from all the connected devices
  }

  GetInternals().s_InjectedInputSlots.Clear();
}

void plInputManager::UpdateInputSlotStates()
{
  for (plInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    // update the state of the input slot, depending on its current value
    // its value will only be larger than zero, if it is also larger than its dead-zone value
    const plKeyState::Enum NewState = plKeyState::GetNewKeyState(it.Value().m_State, it.Value().m_fValue > 0.0f);

    if ((it.Value().m_State != NewState) || (NewState != plKeyState::Up))
    {
      it.Value().m_State = NewState;

      InputEventData e;
      e.m_EventType = InputEventData::InputSlotChanged;
      e.m_sInputSlot = it.Key().GetData();

      s_InputEvents.Broadcast(e);
    }
  }
}

void plInputManager::RetrieveAllKnownInputSlots(plDynamicArray<plStringView>& out_inputSlots)
{
  out_inputSlots.Clear();
  out_inputSlots.Reserve(GetInternals().s_InputSlots.GetCount());

  // just copy all slot names into the given array
  for (plInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    out_inputSlots.PushBack(it.Key().GetData());
  }
}

plUInt32 plInputManager::RetrieveLastCharacter(bool bResetCurrent)
{
  if (!bResetCurrent)
    return s_uiLastCharacter;

  plUInt32 Temp = s_uiLastCharacter;
  s_uiLastCharacter = L'\0';
  return Temp;
}

void plInputManager::InjectInputSlotValue(plStringView sInputSlot, float fValue)
{
  GetInternals().s_InjectedInputSlots[sInputSlot] = plMath::Max(GetInternals().s_InjectedInputSlots[sInputSlot], fValue);
}

plStringView plInputManager::GetPressedInputSlot(plInputSlotFlags::Enum mustHaveFlags, plInputSlotFlags::Enum mustNotHaveFlags)
{
  for (plInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_State != plKeyState::Pressed)
      continue;

    if (it.Value().m_SlotFlags.IsAnySet(mustNotHaveFlags))
      continue;

    if (it.Value().m_SlotFlags.AreAllSet(mustHaveFlags))
      return it.Key().GetData();
  }

  return plInputSlot_None;
}

plStringView plInputManager::GetInputSlotTouchPoint(plUInt32 uiIndex)
{
  switch (uiIndex)
  {
    case 0:
      return plInputSlot_TouchPoint0;
    case 1:
      return plInputSlot_TouchPoint1;
    case 2:
      return plInputSlot_TouchPoint2;
    case 3:
      return plInputSlot_TouchPoint3;
    case 4:
      return plInputSlot_TouchPoint4;
    case 5:
      return plInputSlot_TouchPoint5;
    case 6:
      return plInputSlot_TouchPoint6;
    case 7:
      return plInputSlot_TouchPoint7;
    case 8:
      return plInputSlot_TouchPoint8;
    case 9:
      return plInputSlot_TouchPoint9;
    default:
      PLASMA_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

plStringView plInputManager::GetInputSlotTouchPointPositionX(plUInt32 uiIndex)
{
  switch (uiIndex)
  {
    case 0:
      return plInputSlot_TouchPoint0_PositionX;
    case 1:
      return plInputSlot_TouchPoint1_PositionX;
    case 2:
      return plInputSlot_TouchPoint2_PositionX;
    case 3:
      return plInputSlot_TouchPoint3_PositionX;
    case 4:
      return plInputSlot_TouchPoint4_PositionX;
    case 5:
      return plInputSlot_TouchPoint5_PositionX;
    case 6:
      return plInputSlot_TouchPoint6_PositionX;
    case 7:
      return plInputSlot_TouchPoint7_PositionX;
    case 8:
      return plInputSlot_TouchPoint8_PositionX;
    case 9:
      return plInputSlot_TouchPoint9_PositionX;
    default:
      PLASMA_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

plStringView plInputManager::GetInputSlotTouchPointPositionY(plUInt32 uiIndex)
{
  switch (uiIndex)
  {
    case 0:
      return plInputSlot_TouchPoint0_PositionY;
    case 1:
      return plInputSlot_TouchPoint1_PositionY;
    case 2:
      return plInputSlot_TouchPoint2_PositionY;
    case 3:
      return plInputSlot_TouchPoint3_PositionY;
    case 4:
      return plInputSlot_TouchPoint4_PositionY;
    case 5:
      return plInputSlot_TouchPoint5_PositionY;
    case 6:
      return plInputSlot_TouchPoint6_PositionY;
    case 7:
      return plInputSlot_TouchPoint7_PositionY;
    case 8:
      return plInputSlot_TouchPoint8_PositionY;
    case 9:
      return plInputSlot_TouchPoint9_PositionY;
    default:
      PLASMA_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

PLASMA_STATICLINK_FILE(Core, Core_Input_Implementation_InputManager);

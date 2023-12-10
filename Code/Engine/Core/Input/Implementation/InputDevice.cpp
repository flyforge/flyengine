#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

PLASMA_ENUMERABLE_CLASS_IMPLEMENTATION(plInputDevice);

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plInputDevice, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plKeyState::Enum plKeyState::GetNewKeyState(plKeyState::Enum PrevState, bool bKeyDown)
{
  switch (PrevState)
  {
    case plKeyState::Down:
    case plKeyState::Pressed:
      return bKeyDown ? plKeyState::Down : plKeyState::Released;
    case plKeyState::Released:
    case plKeyState::Up:
      return bKeyDown ? plKeyState::Pressed : plKeyState::Up;
  }

  return plKeyState::Up;
}

plInputDevice::plInputDevice()
{
  m_bInitialized = false;
  m_uiLastCharacter = '\0';
}

void plInputDevice::RegisterInputSlot(plStringView sName, plStringView sDefaultDisplayName, plBitflags<plInputSlotFlags> SlotFlags)
{
  plInputManager::RegisterInputSlot(sName, sDefaultDisplayName, SlotFlags);
}

void plInputDevice::Initialize()
{
  if (m_bInitialized)
    return;

  PLASMA_LOG_BLOCK("Initializing Input Device", GetDynamicRTTI()->GetTypeName());

  plLog::Dev("Input Device Type: {0}, Device Name: {1}", GetDynamicRTTI()->GetParentType()->GetTypeName(), GetDynamicRTTI()->GetTypeName());

  m_bInitialized = true;

  RegisterInputSlots();
  InitializeDevice();
}


void plInputDevice::UpdateAllHardwareStates(plTime tTimeDifference)
{
  // tell each device to update its hardware
  for (plInputDevice* pDevice = plInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->UpdateHardwareState(tTimeDifference);
  }
}

void plInputDevice::UpdateAllDevices()
{
  // tell each device to update its current input slot values
  for (plInputDevice* pDevice = plInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->Initialize();
    pDevice->UpdateInputSlotValues();
  }
}

void plInputDevice::ResetAllDevices()
{
  // tell all devices that the input update is through and they might need to reset some values now
  // this is especially important for device types that will get input messages at some undefined time after this call
  // but not during 'UpdateInputSlotValues'
  for (plInputDevice* pDevice = plInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->ResetInputSlotValues();
  }
}

plUInt32 plInputDevice::RetrieveLastCharacter()
{
  plUInt32 Temp = m_uiLastCharacter;
  m_uiLastCharacter = L'\0';
  return Temp;
}

plUInt32 plInputDevice::RetrieveLastCharacterFromAllDevices()
{
  for (plInputDevice* pDevice = plInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    const plUInt32 Char = pDevice->RetrieveLastCharacter();

    if (Char != L'\0')
      return Char;
  }

  return '\0';
}

float plInputDevice::GetInputSlotState(plStringView sSlot) const
{
  return m_InputSlotValues.GetValueOrDefault(sSlot, 0.f);
}

bool plInputDevice::HasDeviceBeenUsedLastFrame() const
{
  return m_bGeneratedInputRecently;
}

PLASMA_STATICLINK_FILE(Core, Core_Input_Implementation_InputDevice);

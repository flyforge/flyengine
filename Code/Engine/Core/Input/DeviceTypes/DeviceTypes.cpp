#include <Core/CorePCH.h>

#include <Core/Input/DeviceTypes/Controller.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plInputDeviceMouseKeyboard, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plInputDeviceController, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plInt32 plInputDeviceMouseKeyboard::s_iMouseIsOverWindowNumber = -1;

plInputDeviceController::plInputDeviceController()
{
  m_uiVibrationTrackPos = 0;

  for (plInt8 c = 0; c < MaxControllers; ++c)
  {
    m_bVibrationEnabled[c] = false;
    m_iControllerMapping[c] = c;

    for (plInt8 m = 0; m < Motor::ENUM_COUNT; ++m)
    {
      m_fVibrationStrength[c][m] = 0.0f;

      for (plUInt8 t = 0; t < MaxVibrationSamples; ++t)
        m_fVibrationTracks[c][m][t] = 0.0f;
    }
  }
}

void plInputDeviceController::EnableVibration(plUInt8 uiVirtual, bool bEnable)
{
  PL_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);

  m_bVibrationEnabled[uiVirtual] = bEnable;
}

bool plInputDeviceController::IsVibrationEnabled(plUInt8 uiVirtual) const
{
  PL_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);

  return m_bVibrationEnabled[uiVirtual];
}

void plInputDeviceController::SetVibrationStrength(plUInt8 uiVirtual, Motor::Enum motor, float fValue)
{
  PL_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);
  PL_ASSERT_DEV(motor < Motor::ENUM_COUNT, "Invalid Vibration Motor Index.");

  m_fVibrationStrength[uiVirtual][motor] = plMath::Clamp(fValue, 0.0f, 1.0f);
}

float plInputDeviceController::GetVibrationStrength(plUInt8 uiVirtual, Motor::Enum motor)
{
  PL_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);
  PL_ASSERT_DEV(motor < Motor::ENUM_COUNT, "Invalid Vibration Motor Index.");

  return m_fVibrationStrength[uiVirtual][motor];
}

void plInputDeviceController::SetControllerMapping(plUInt8 uiVirtualController, plInt8 iTakeInputFromPhysical)
{
  PL_ASSERT_DEV(
    uiVirtualController < MaxControllers, "Virtual Controller Index {0} is larger than allowed ({1}).", uiVirtualController, MaxControllers);
  PL_ASSERT_DEV(
    iTakeInputFromPhysical < MaxControllers, "Physical Controller Index {0} is larger than allowed ({1}).", iTakeInputFromPhysical, MaxControllers);

  if (iTakeInputFromPhysical < 0)
  {
    // deactivates this virtual controller
    m_iControllerMapping[uiVirtualController] = -1;
  }
  else
  {
    // if any virtual controller already maps to the given physical controller, let it use the physical controller that
    // uiVirtualController is currently mapped to
    for (plInt32 c = 0; c < MaxControllers; ++c)
    {
      if (m_iControllerMapping[c] == iTakeInputFromPhysical)
      {
        m_iControllerMapping[c] = m_iControllerMapping[uiVirtualController];
        break;
      }
    }

    m_iControllerMapping[uiVirtualController] = iTakeInputFromPhysical;
  }
}

plInt8 plInputDeviceController::GetControllerMapping(plUInt8 uiVirtual) const
{
  PL_ASSERT_DEV(uiVirtual < MaxControllers, "Virtual Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);

  return m_iControllerMapping[uiVirtual];
}

void plInputDeviceController::AddVibrationTrack(
  plUInt8 uiVirtual, Motor::Enum motor, float* pVibrationTrackValue, plUInt32 uiSamples, float fScalingFactor)
{
  uiSamples = plMath::Min<plUInt32>(uiSamples, MaxVibrationSamples);

  for (plUInt32 s = 0; s < uiSamples; ++s)
  {
    float& fVal = m_fVibrationTracks[uiVirtual][motor][(m_uiVibrationTrackPos + 1 + s) % MaxVibrationSamples];

    fVal = plMath::Max(fVal, pVibrationTrackValue[s] * fScalingFactor);
    fVal = plMath::Clamp(fVal, 0.0f, 1.0f);
  }
}

void plInputDeviceController::UpdateVibration(plTime tTimeDifference)
{
  static plTime tElapsedTime;
  tElapsedTime += tTimeDifference;

  const plTime tTimePerSample = plTime::MakeFromSeconds(1.0 / (double)VibrationSamplesPerSecond);

  // advance the vibration track sampling
  while (tElapsedTime >= tTimePerSample)
  {
    tElapsedTime -= tTimePerSample;

    for (plUInt32 c = 0; c < MaxControllers; ++c)
    {
      for (plUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
        m_fVibrationTracks[c][m][m_uiVibrationTrackPos] = 0.0f;
    }

    m_uiVibrationTrackPos = (m_uiVibrationTrackPos + 1) % MaxVibrationSamples;
  }

  // we will temporarily store how much vibration is to be applied on each physical controller
  float fVibrationToApply[MaxControllers][Motor::ENUM_COUNT];

  // Initialize with zero (we might not set all values later)
  for (plUInt32 c = 0; c < MaxControllers; ++c)
  {
    for (plUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
      fVibrationToApply[c][m] = 0.0f;
  }

  // go through all controllers and motors
  for (plUInt8 c = 0; c < MaxControllers; ++c)
  {
    // ignore if vibration is disabled on this controller
    if (!m_bVibrationEnabled[c])
      continue;

    // check which physical controller this virtual controller is attached to
    const plInt8 iPhysical = GetControllerMapping(c);

    // if it is attached to any physical controller, store the vibration value
    if (iPhysical >= 0)
    {
      for (plUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
        fVibrationToApply[(plUInt8)iPhysical][m] = plMath::Max(m_fVibrationStrength[c][m], m_fVibrationTracks[c][m][m_uiVibrationTrackPos]);
    }
  }

  // now send the back-end all the information about how to vibrate which physical controller
  // this also always resets vibration to zero for controllers that might have been changed to another virtual controller etc.
  for (plUInt8 c = 0; c < MaxControllers; ++c)
  {
    for (plUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
    {
      ApplyVibration(c, (Motor::Enum)m, fVibrationToApply[c][m]);
    }
  }
}

void plInputDeviceMouseKeyboard::UpdateInputSlotValues()
{
  const char* slots[3] = {plInputSlot_MouseButton0, plInputSlot_MouseButton1, plInputSlot_MouseButton2};
  const char* dlbSlots[3] = {plInputSlot_MouseDblClick0, plInputSlot_MouseDblClick1, plInputSlot_MouseDblClick2};

  const plTime tNow = plTime::Now();

  for (int i = 0; i < 3; ++i)
  {
    m_InputSlotValues[dlbSlots[i]] = 0.0f;

    const bool bDown = m_InputSlotValues[slots[i]] > 0;
    if (bDown)
    {
      if (!m_bMouseDown[i])
      {
        if (tNow - m_LastMouseClick[i] <= m_DoubleClickTime)
        {
          m_InputSlotValues[dlbSlots[i]] = 1.0f;
          m_LastMouseClick[i] = plTime::MakeZero(); // this prevents triple-clicks from appearing as two double clicks
        }
        else
        {
          m_LastMouseClick[i] = tNow;
        }
      }
    }

    m_bMouseDown[i] = bDown;
  }
}

PL_STATICLINK_FILE(Core, Core_Input_DeviceTypes_DeviceTypes);

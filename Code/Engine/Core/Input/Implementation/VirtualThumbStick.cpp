#include <Core/CorePCH.h>

#include <Core/Input/VirtualThumbStick.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plVirtualThumbStick, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plInt32 plVirtualThumbStick::s_iThumbsticks = 0;

plVirtualThumbStick::plVirtualThumbStick()
{
  SetAreaFocusMode(plInputActionConfig::RequireKeyUp, plInputActionConfig::KeepFocus);
  SetTriggerInputSlot(plVirtualThumbStick::Input::Touchpoint);
  SetThumbstickOutput(plVirtualThumbStick::Output::Controller0_LeftStick);

  SetInputArea(plVec2(0.0f), plVec2(0.0f), 0.0f, 0.0f);

  plStringBuilder s;
  s.SetFormat("Thumbstick_{0}", s_iThumbsticks);
  m_sName = s;

  ++s_iThumbsticks;

  m_bEnabled = false;
  m_bConfigChanged = false;
  m_bIsActive = false;
}

plVirtualThumbStick::~plVirtualThumbStick()
{
  plInputManager::RemoveInputAction(GetDynamicRTTI()->GetTypeName(), m_sName.GetData());
}

void plVirtualThumbStick::SetTriggerInputSlot(plVirtualThumbStick::Input::Enum input, const plInputActionConfig* pCustomConfig)
{
  for (plInt32 i = 0; i < plInputActionConfig::MaxInputSlotAlternatives; ++i)
  {
    m_ActionConfig.m_sFilterByInputSlotX[i] = plInputSlot_None;
    m_ActionConfig.m_sFilterByInputSlotY[i] = plInputSlot_None;
    m_ActionConfig.m_sInputSlotTrigger[i] = plInputSlot_None;
  }

  switch (input)
  {
    case plVirtualThumbStick::Input::Touchpoint:
    {
      m_ActionConfig.m_sFilterByInputSlotX[0] = plInputSlot_TouchPoint0_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[0] = plInputSlot_TouchPoint0_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[0] = plInputSlot_TouchPoint0;

      m_ActionConfig.m_sFilterByInputSlotX[1] = plInputSlot_TouchPoint1_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[1] = plInputSlot_TouchPoint1_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[1] = plInputSlot_TouchPoint1;

      m_ActionConfig.m_sFilterByInputSlotX[2] = plInputSlot_TouchPoint2_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[2] = plInputSlot_TouchPoint2_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[2] = plInputSlot_TouchPoint2;
    }
    break;
    case plVirtualThumbStick::Input::MousePosition:
    {
      m_ActionConfig.m_sFilterByInputSlotX[0] = plInputSlot_MousePositionX;
      m_ActionConfig.m_sFilterByInputSlotY[0] = plInputSlot_MousePositionY;
      m_ActionConfig.m_sInputSlotTrigger[0] = plInputSlot_MouseButton0;
    }
    break;
    case plVirtualThumbStick::Input::Custom:
    {
      PL_ASSERT_DEV(pCustomConfig != nullptr, "Must pass a custom config, if you want to have a custom config.");

      for (plInt32 i = 0; i < plInputActionConfig::MaxInputSlotAlternatives; ++i)
      {
        m_ActionConfig.m_sFilterByInputSlotX[i] = pCustomConfig->m_sFilterByInputSlotX[i];
        m_ActionConfig.m_sFilterByInputSlotY[i] = pCustomConfig->m_sFilterByInputSlotY[i];
        m_ActionConfig.m_sInputSlotTrigger[i] = pCustomConfig->m_sInputSlotTrigger[i];
      }
    }
    break;
  }

  m_bConfigChanged = true;
}

void plVirtualThumbStick::SetThumbstickOutput(plVirtualThumbStick::Output::Enum output, plStringView sOutputLeft, plStringView sOutputRight, plStringView sOutputUp, plStringView sOutputDown)
{
  switch (output)
  {
    case plVirtualThumbStick::Output::Controller0_LeftStick:
    {
      m_sOutputLeft = plInputSlot_Controller0_LeftStick_NegX;
      m_sOutputRight = plInputSlot_Controller0_LeftStick_PosX;
      m_sOutputUp = plInputSlot_Controller0_LeftStick_PosY;
      m_sOutputDown = plInputSlot_Controller0_LeftStick_NegY;
    }
    break;
    case plVirtualThumbStick::Output::Controller0_RightStick:
    {
      m_sOutputLeft = plInputSlot_Controller0_RightStick_NegX;
      m_sOutputRight = plInputSlot_Controller0_RightStick_PosX;
      m_sOutputUp = plInputSlot_Controller0_RightStick_PosY;
      m_sOutputDown = plInputSlot_Controller0_RightStick_NegY;
    }
    break;
    case plVirtualThumbStick::Output::Controller1_LeftStick:
    {
      m_sOutputLeft = plInputSlot_Controller1_LeftStick_NegX;
      m_sOutputRight = plInputSlot_Controller1_LeftStick_PosX;
      m_sOutputUp = plInputSlot_Controller1_LeftStick_PosY;
      m_sOutputDown = plInputSlot_Controller1_LeftStick_NegY;
    }
    break;
    case plVirtualThumbStick::Output::Controller1_RightStick:
    {
      m_sOutputLeft = plInputSlot_Controller1_RightStick_NegX;
      m_sOutputRight = plInputSlot_Controller1_RightStick_PosX;
      m_sOutputUp = plInputSlot_Controller1_RightStick_PosY;
      m_sOutputDown = plInputSlot_Controller1_RightStick_NegY;
    }
    break;
    case plVirtualThumbStick::Output::Controller2_LeftStick:
    {
      m_sOutputLeft = plInputSlot_Controller2_LeftStick_NegX;
      m_sOutputRight = plInputSlot_Controller2_LeftStick_PosX;
      m_sOutputUp = plInputSlot_Controller2_LeftStick_PosY;
      m_sOutputDown = plInputSlot_Controller2_LeftStick_NegY;
    }
    break;
    case plVirtualThumbStick::Output::Controller2_RightStick:
    {
      m_sOutputLeft = plInputSlot_Controller2_RightStick_NegX;
      m_sOutputRight = plInputSlot_Controller2_RightStick_PosX;
      m_sOutputUp = plInputSlot_Controller2_RightStick_PosY;
      m_sOutputDown = plInputSlot_Controller2_RightStick_NegY;
    }
    break;
    case plVirtualThumbStick::Output::Controller3_LeftStick:
    {
      m_sOutputLeft = plInputSlot_Controller3_LeftStick_NegX;
      m_sOutputRight = plInputSlot_Controller3_LeftStick_PosX;
      m_sOutputUp = plInputSlot_Controller3_LeftStick_PosY;
      m_sOutputDown = plInputSlot_Controller3_LeftStick_NegY;
    }
    break;
    case plVirtualThumbStick::Output::Controller3_RightStick:
    {
      m_sOutputLeft = plInputSlot_Controller3_RightStick_NegX;
      m_sOutputRight = plInputSlot_Controller3_RightStick_PosX;
      m_sOutputUp = plInputSlot_Controller3_RightStick_PosY;
      m_sOutputDown = plInputSlot_Controller3_RightStick_NegY;
    }
    break;
    case plVirtualThumbStick::Output::Custom:
    {
      m_sOutputLeft = sOutputLeft;
      m_sOutputRight = sOutputRight;
      m_sOutputUp = sOutputUp;
      m_sOutputDown = sOutputDown;
    }
    break;
  }

  m_bConfigChanged = true;
}

void plVirtualThumbStick::SetAreaFocusMode(plInputActionConfig::OnEnterArea onEnter, plInputActionConfig::OnLeaveArea onLeave)
{
  m_bConfigChanged = true;

  m_ActionConfig.m_OnEnterArea = onEnter;
  m_ActionConfig.m_OnLeaveArea = onLeave;
}

void plVirtualThumbStick::SetInputArea(
  const plVec2& vLowerLeft, const plVec2& vUpperRight, float fThumbstickRadius, float fPriority, CenterMode::Enum center)
{
  m_bConfigChanged = true;

  m_vLowerLeft = vLowerLeft;
  m_vUpperRight = vUpperRight;
  m_fRadius = fThumbstickRadius;
  m_ActionConfig.m_fFilteredPriority = fPriority;
  m_CenterMode = center;
}

void plVirtualThumbStick::GetInputArea(plVec2& out_vLowerLeft, plVec2& out_vUpperRight)
{
  out_vLowerLeft = m_vLowerLeft;
  out_vUpperRight = m_vUpperRight;
}

void plVirtualThumbStick::UpdateActionMapping()
{
  if (!m_bConfigChanged)
    return;

  m_ActionConfig.m_fFilterXMinValue = m_vLowerLeft.x;
  m_ActionConfig.m_fFilterXMaxValue = m_vUpperRight.x;
  m_ActionConfig.m_fFilterYMinValue = m_vLowerLeft.y;
  m_ActionConfig.m_fFilterYMaxValue = m_vUpperRight.y;

  plInputManager::SetInputActionConfig(GetDynamicRTTI()->GetTypeName(), m_sName.GetData(), m_ActionConfig, false);

  m_bConfigChanged = false;
}

void plVirtualThumbStick::UpdateInputSlotValues()
{
  m_bIsActive = false;

  m_InputSlotValues[m_sOutputLeft] = 0.0f;
  m_InputSlotValues[m_sOutputRight] = 0.0f;
  m_InputSlotValues[m_sOutputUp] = 0.0f;
  m_InputSlotValues[m_sOutputDown] = 0.0f;

  if (!m_bEnabled)
  {
    plInputManager::RemoveInputAction(GetDynamicRTTI()->GetTypeName(), m_sName.GetData());
    return;
  }

  UpdateActionMapping();

  float fValue;
  plInt8 iTriggerAlt;

  const plKeyState::Enum ks = plInputManager::GetInputActionState(GetDynamicRTTI()->GetTypeName(), m_sName.GetData(), &fValue, &iTriggerAlt);

  if (ks != plKeyState::Up)
  {
    m_bIsActive = true;

    plVec2 vTouchPos(0.0f);

    plInputManager::GetInputSlotState(m_ActionConfig.m_sFilterByInputSlotX[(plUInt32)iTriggerAlt].GetData(), &vTouchPos.x);
    plInputManager::GetInputSlotState(m_ActionConfig.m_sFilterByInputSlotY[(plUInt32)iTriggerAlt].GetData(), &vTouchPos.y);

    if (ks == plKeyState::Pressed)
    {
      switch (m_CenterMode)
      {
        case CenterMode::InputArea:
          m_vCenter = m_vLowerLeft + (m_vUpperRight - m_vLowerLeft) * 0.5f;
          break;
        case CenterMode::ActivationPoint:
          m_vCenter = vTouchPos;
          break;
      }
    }

    plVec2 vDir = vTouchPos - m_vCenter;
    vDir.y *= -1;

    const float fLength = plMath::Min(vDir.GetLength(), m_fRadius) / m_fRadius;
    vDir.Normalize();

    m_InputSlotValues[m_sOutputLeft] = plMath::Max(0.0f, -vDir.x) * fLength;
    m_InputSlotValues[m_sOutputRight] = plMath::Max(0.0f, vDir.x) * fLength;
    m_InputSlotValues[m_sOutputUp] = plMath::Max(0.0f, vDir.y) * fLength;
    m_InputSlotValues[m_sOutputDown] = plMath::Max(0.0f, -vDir.y) * fLength;
  }
}

void plVirtualThumbStick::RegisterInputSlots()
{
  RegisterInputSlot(plInputSlot_Controller0_LeftStick_NegX, "Left Stick Left", plInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(plInputSlot_Controller0_LeftStick_PosX, "Left Stick Right", plInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(plInputSlot_Controller0_LeftStick_NegY, "Left Stick Down", plInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(plInputSlot_Controller0_LeftStick_PosY, "Left Stick Up", plInputSlotFlags::IsAnalogStick);

  RegisterInputSlot(plInputSlot_Controller0_RightStick_NegX, "Right Stick Left", plInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(plInputSlot_Controller0_RightStick_PosX, "Right Stick Right", plInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(plInputSlot_Controller0_RightStick_NegY, "Right Stick Down", plInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(plInputSlot_Controller0_RightStick_PosY, "Right Stick Up", plInputSlotFlags::IsAnalogStick);
}


PL_STATICLINK_FILE(Core, Core_Input_Implementation_VirtualThumbStick);

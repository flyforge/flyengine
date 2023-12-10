#include <Core/Input/InputManager.h>
#include <Core/System/Implementation/Win/InputDevice_win32.h>
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringConversion.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plStandardInputDevice, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool plStandardInputDevice::s_bMainWindowUsed = false;

plStandardInputDevice::plStandardInputDevice(plUInt32 uiWindowNumber)
{
  m_uiWindowNumber = uiWindowNumber;

  if (uiWindowNumber == 0)
  {
    PLASMA_ASSERT_RELEASE(!s_bMainWindowUsed, "You cannot have two devices of Type plStandardInputDevice with the window number zero.");
    plStandardInputDevice::s_bMainWindowUsed = true;
  }

  m_DoubleClickTime = plTime::Milliseconds(GetDoubleClickTime());
}

plStandardInputDevice::~plStandardInputDevice()
{
  if (!m_bShowCursor)
  {
    ShowCursor(true);
  }

  if (m_uiWindowNumber == 0)
    plStandardInputDevice::s_bMainWindowUsed = false;
}

void plStandardInputDevice::InitializeDevice()
{
  if (m_uiWindowNumber == 0)
  {
    RAWINPUTDEVICE Rid[2];

    // keyboard
    Rid[0].usUsagePage = 0x01;
    Rid[0].usUsage = 0x06;
    Rid[0].dwFlags = RIDEV_NOHOTKEYS; // Disables Windows-Key and Application-Key
    Rid[0].hwndTarget = nullptr;

    // mouse
    Rid[1].usUsagePage = 0x01;
    Rid[1].usUsage = 0x02;
    Rid[1].dwFlags = 0;
    Rid[1].hwndTarget = nullptr;

    if (RegisterRawInputDevices(&Rid[0], (UINT)2, sizeof(RAWINPUTDEVICE)) == FALSE)
    {
      plLog::Error("Could not initialize RawInput for Mouse and Keyboard input.");
    }
    else
      plLog::Success("Initialized RawInput for Mouse and Keyboard input.");
  }
  else
    plLog::Info("Window {0} does not need to initialize Mouse or Keyboard.", m_uiWindowNumber);
}

void plStandardInputDevice::RegisterInputSlots()
{
  RegisterInputSlot(plInputSlot_KeyLeft, "Left", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyRight, "Right", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyUp, "Up", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyDown, "Down", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_KeyEscape, "Escape", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeySpace, "Space", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyBackspace, "Backspace", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyReturn, "Return", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyTab, "Tab", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_KeyLeftShift, "Left Shift", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyRightShift, "Right Shift", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_KeyLeftCtrl, "Left Ctrl", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyRightCtrl, "Right Ctrl", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_KeyLeftAlt, "Left Alt", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyRightAlt, "Right Alt", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_KeyLeftWin, "Left Win", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyRightWin, "Right Win", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_KeyBracketOpen, "[", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyBracketClose, "]", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_KeySemicolon, ";", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyApostrophe, "'", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeySlash, "/", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyEquals, "=", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyTilde, "~", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyHyphen, "-", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyComma, ",", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyPeriod, ".", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyBackslash, "\\", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyPipe, "|", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_Key1, "1", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_Key2, "2", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_Key3, "3", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_Key4, "4", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_Key5, "5", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_Key6, "6", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_Key7, "7", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_Key8, "8", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_Key9, "9", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_Key0, "0", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_KeyNumpad1, "Numpad 1", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpad2, "Numpad 2", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpad3, "Numpad 3", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpad4, "Numpad 4", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpad5, "Numpad 5", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpad6, "Numpad 6", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpad7, "Numpad 7", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpad8, "Numpad 8", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpad9, "Numpad 9", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpad0, "Numpad 0", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_KeyA, "A", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyB, "B", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyC, "C", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyD, "D", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyE, "E", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyF, "F", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyG, "G", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyH, "H", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyI, "I", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyJ, "J", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyK, "K", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyL, "L", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyM, "M", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyN, "N", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyO, "O", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyP, "P", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyQ, "Q", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyR, "R", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyS, "S", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyT, "T", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyU, "U", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyV, "V", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyW, "W", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyX, "X", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyY, "Y", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyZ, "Z", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_KeyF1, "F1", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyF2, "F2", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyF3, "F3", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyF4, "F4", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyF5, "F5", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyF6, "F6", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyF7, "F7", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyF8, "F8", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyF9, "F9", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyF10, "F10", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyF11, "F11", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyF12, "F12", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_KeyHome, "Home", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyEnd, "End", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyDelete, "Delete", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyInsert, "Insert", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyPageUp, "Page Up", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyPageDown, "Page Down", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_KeyNumLock, "Numlock", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpadPlus, "Numpad +", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpadMinus, "Numpad -", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpadStar, "Numpad *", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpadSlash, "Numpad /", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpadPeriod, "Numpad .", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNumpadEnter, "Enter", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_KeyCapsLock, "Capslock", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyPrint, "Print", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyScroll, "Scroll", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyPause, "Pause", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyApps, "Application", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_KeyPrevTrack, "Previous Track", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNextTrack, "Next Track", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyPlayPause, "Play / Pause", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyStop, "Stop", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyVolumeUp, "Volume Up", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyVolumeDown, "Volume Down", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyMute, "Mute", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_MouseWheelUp, "Mousewheel Up", plInputSlotFlags::IsMouseWheel);
  RegisterInputSlot(plInputSlot_MouseWheelDown, "Mousewheel Down", plInputSlotFlags::IsMouseWheel);

  RegisterInputSlot(plInputSlot_MouseMoveNegX, "Mouse Move Left", plInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(plInputSlot_MouseMovePosX, "Mouse Move Right", plInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(plInputSlot_MouseMoveNegY, "Mouse Move Down", plInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(plInputSlot_MouseMovePosY, "Mouse Move Up", plInputSlotFlags::IsMouseAxisMove);

  RegisterInputSlot(plInputSlot_MouseButton0, "Mousebutton 0", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_MouseButton1, "Mousebutton 1", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_MouseButton2, "Mousebutton 2", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_MouseButton3, "Mousebutton 3", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_MouseButton4, "Mousebutton 4", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_MouseDblClick0, "Left Double Click", plInputSlotFlags::IsDoubleClick);
  RegisterInputSlot(plInputSlot_MouseDblClick1, "Right Double Click", plInputSlotFlags::IsDoubleClick);
  RegisterInputSlot(plInputSlot_MouseDblClick2, "Middle Double Click", plInputSlotFlags::IsDoubleClick);

  RegisterInputSlot(plInputSlot_MousePositionX, "Mouse Position X", plInputSlotFlags::IsMouseAxisPosition);
  RegisterInputSlot(plInputSlot_MousePositionY, "Mouse Position Y", plInputSlotFlags::IsMouseAxisPosition);


  RegisterInputSlot(plInputSlot_TouchPoint0, "Touchpoint 1", plInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(plInputSlot_TouchPoint0_PositionX, "Touchpoint 1 Position X", plInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(plInputSlot_TouchPoint0_PositionY, "Touchpoint 1 Position Y", plInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(plInputSlot_TouchPoint1, "Touchpoint 2", plInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(plInputSlot_TouchPoint1_PositionX, "Touchpoint 2 Position X", plInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(plInputSlot_TouchPoint1_PositionY, "Touchpoint 2 Position Y", plInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(plInputSlot_TouchPoint2, "Touchpoint 3", plInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(plInputSlot_TouchPoint2_PositionX, "Touchpoint 3 Position X", plInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(plInputSlot_TouchPoint2_PositionY, "Touchpoint 3 Position Y", plInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(plInputSlot_TouchPoint3, "Touchpoint 4", plInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(plInputSlot_TouchPoint3_PositionX, "Touchpoint 4 Position X", plInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(plInputSlot_TouchPoint3_PositionY, "Touchpoint 4 Position Y", plInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(plInputSlot_TouchPoint4, "Touchpoint 5", plInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(plInputSlot_TouchPoint4_PositionX, "Touchpoint 5 Position X", plInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(plInputSlot_TouchPoint4_PositionY, "Touchpoint 5 Position Y", plInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(plInputSlot_TouchPoint5, "Touchpoint 6", plInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(plInputSlot_TouchPoint5_PositionX, "Touchpoint 6 Position X", plInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(plInputSlot_TouchPoint5_PositionY, "Touchpoint 6 Position Y", plInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(plInputSlot_TouchPoint6, "Touchpoint 7", plInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(plInputSlot_TouchPoint6_PositionX, "Touchpoint 7 Position X", plInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(plInputSlot_TouchPoint6_PositionY, "Touchpoint 7 Position Y", plInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(plInputSlot_TouchPoint7, "Touchpoint 8", plInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(plInputSlot_TouchPoint7_PositionX, "Touchpoint 8 Position X", plInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(plInputSlot_TouchPoint7_PositionY, "Touchpoint 8 Position Y", plInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(plInputSlot_TouchPoint8, "Touchpoint 9", plInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(plInputSlot_TouchPoint8_PositionX, "Touchpoint 9 Position X", plInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(plInputSlot_TouchPoint8_PositionY, "Touchpoint 9 Position Y", plInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(plInputSlot_TouchPoint9, "Touchpoint 10", plInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(plInputSlot_TouchPoint9_PositionX, "Touchpoint 10 Position X", plInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(plInputSlot_TouchPoint9_PositionY, "Touchpoint 10 Position Y", plInputSlotFlags::IsTouchPosition);
}

void plStandardInputDevice::ResetInputSlotValues()
{
  m_InputSlotValues[plInputSlot_MouseWheelUp] = 0;
  m_InputSlotValues[plInputSlot_MouseWheelDown] = 0;
  m_InputSlotValues[plInputSlot_MouseMoveNegX] = 0;
  m_InputSlotValues[plInputSlot_MouseMovePosX] = 0;
  m_InputSlotValues[plInputSlot_MouseMoveNegY] = 0;
  m_InputSlotValues[plInputSlot_MouseMovePosY] = 0;
  m_InputSlotValues[plInputSlot_MouseDblClick0] = 0;
  m_InputSlotValues[plInputSlot_MouseDblClick1] = 0;
  m_InputSlotValues[plInputSlot_MouseDblClick2] = 0;
}

void plStandardInputDevice::UpdateInputSlotValues()
{
  const char* slotDown[5] = {plInputSlot_MouseButton0, plInputSlot_MouseButton1, plInputSlot_MouseButton2, plInputSlot_MouseButton3, plInputSlot_MouseButton4};

  // don't read uninitialized values
  if (!m_InputSlotValues.Contains(slotDown[4]))
  {
    for (int i = 0; i < 5; ++i)
    {
      m_InputSlotValues[slotDown[i]] = 0;
    }
  }

  for (int i = 0; i < 5; ++i)
  {
    if (m_InputSlotValues[slotDown[i]] > 0)
    {
      if (m_uiMouseButtonReceivedUp[i] > 0)
      {
        --m_uiMouseButtonReceivedUp[i];
        m_InputSlotValues[slotDown[i]] = 0;
      }
    }
    else
    {
      if (m_uiMouseButtonReceivedDown[i] > 0)
      {
        --m_uiMouseButtonReceivedDown[i];
        m_InputSlotValues[slotDown[i]] = 1.0f;
      }
    }
  }

  SUPER::UpdateInputSlotValues();
}

void plStandardInputDevice::ApplyClipRect(plMouseCursorClipMode::Enum mode, plMinWindows::HWND hWnd)
{
  if (!m_bApplyClipRect)
    return;

  m_bApplyClipRect = false;

  if (mode == plMouseCursorClipMode::NoClip)
  {
    ClipCursor(nullptr);
    return;
  }

  RECT r;
  {
    RECT area;
    GetClientRect(plMinWindows::ToNative(hWnd), &area);
    POINT p0, p1;
    p0.x = 0;
    p0.y = 0;
    p1.x = area.right;
    p1.y = area.bottom;

    ClientToScreen(plMinWindows::ToNative(hWnd), &p0);
    ClientToScreen(plMinWindows::ToNative(hWnd), &p1);

    r.top = p0.y;
    r.left = p0.x;
    r.right = p1.x;
    r.bottom = p1.y;
  }

  if (mode == plMouseCursorClipMode::ClipToPosition)
  {
    POINT mp;
    if (GetCursorPos(&mp))
    {
      // make sure the position is inside the window rect
      mp.x = plMath::Clamp(mp.x, r.left, r.right);
      mp.y = plMath::Clamp(mp.y, r.top, r.bottom);

      r.top = mp.y;
      r.bottom = mp.y;
      r.left = mp.x;
      r.right = mp.x;
    }
  }

  ClipCursor(&r);
}

void plStandardInputDevice::SetClipMouseCursor(plMouseCursorClipMode::Enum mode)
{
  if (m_ClipCursorMode == mode)
    return;

  m_ClipCursorMode = mode;
  m_bApplyClipRect = m_ClipCursorMode != plMouseCursorClipMode::NoClip;

  if (m_ClipCursorMode == plMouseCursorClipMode::NoClip)
    ClipCursor(nullptr);
}

// WM_INPUT mouse clicks do not work in some VMs.
// When this is enabled, mouse clicks are retrieved via standard WM_LBUTTONDOWN.
#define PLASMA_MOUSEBUTTON_COMPATIBILTY_MODE PLASMA_ON

void plStandardInputDevice::WindowMessage(
  plMinWindows::HWND hWnd, plMinWindows::UINT Msg, plMinWindows::WPARAM wParam, plMinWindows::LPARAM lParam)
{
#if PLASMA_ENABLED(PLASMA_MOUSEBUTTON_COMPATIBILTY_MODE)
  static plInt32 s_iMouseCaptureCount = 0;
#endif

  switch (Msg)
  {
    case WM_MOUSEWHEEL:
    {
      // The mousewheel does not work with rawinput over touchpads (at least not all)
      // So we handle that one individually

      const plInt32 iRotated = (plInt16)HIWORD(wParam);

      if (iRotated > 0)
        m_InputSlotValues[plInputSlot_MouseWheelUp] = iRotated / 120.0f;
      else
        m_InputSlotValues[plInputSlot_MouseWheelDown] = iRotated / -120.0f;

      break;
    }

    case WM_MOUSEMOVE:
    {
      RECT area;
      GetClientRect(plMinWindows::ToNative(hWnd), &area);

      const plUInt32 uiResX = area.right - area.left;
      const plUInt32 uiResY = area.bottom - area.top;

      const float fPosX = (float)((short)LOWORD(lParam));
      const float fPosY = (float)((short)HIWORD(lParam));

      s_iMouseIsOverWindowNumber = m_uiWindowNumber;
      m_InputSlotValues[plInputSlot_MousePositionX] = (fPosX / uiResX);
      m_InputSlotValues[plInputSlot_MousePositionY] = (fPosY / uiResY);

      if (m_ClipCursorMode == plMouseCursorClipMode::ClipToPosition || m_ClipCursorMode == plMouseCursorClipMode::ClipToWindowImmediate)
      {
        ApplyClipRect(m_ClipCursorMode, hWnd);
      }

      break;
    }

    case WM_SETFOCUS:
    {
      m_bApplyClipRect = true;
      ApplyClipRect(m_ClipCursorMode, hWnd);
      break;
    }

    case WM_KILLFOCUS:
    {
      OnFocusLost(hWnd);
      return;
    }

    case WM_CHAR:
      m_uiLastCharacter = (wchar_t)wParam;
      return;

      // these messages would only arrive, if the window had the flag CS_DBLCLKS
      // see https://docs.microsoft.com/windows/win32/inputdev/wm-lbuttondblclk
      // this would add lag and hide single clicks when the user double clicks
      // therefore it is not used
      //case WM_LBUTTONDBLCLK:
      //  m_InputSlotValues[plInputSlot_MouseDblClick0] = 1.0f;
      //  return;
      //case WM_RBUTTONDBLCLK:
      //  m_InputSlotValues[plInputSlot_MouseDblClick1] = 1.0f;
      //  return;
      //case WM_MBUTTONDBLCLK:
      //  m_InputSlotValues[plInputSlot_MouseDblClick2] = 1.0f;
      //  return;

#if PLASMA_ENABLED(PLASMA_MOUSEBUTTON_COMPATIBILTY_MODE)

    case WM_LBUTTONDOWN:
      m_uiMouseButtonReceivedDown[0]++;

      if (s_iMouseCaptureCount == 0)
        SetCapture(plMinWindows::ToNative(hWnd));
      ++s_iMouseCaptureCount;


      return;

    case WM_LBUTTONUP:
      m_uiMouseButtonReceivedUp[0]++;
      ApplyClipRect(m_ClipCursorMode, hWnd);

      --s_iMouseCaptureCount;
      if (s_iMouseCaptureCount <= 0)
        ReleaseCapture();

      return;

    case WM_RBUTTONDOWN:
      m_uiMouseButtonReceivedDown[1]++;

      if (s_iMouseCaptureCount == 0)
        SetCapture(plMinWindows::ToNative(hWnd));
      ++s_iMouseCaptureCount;

      return;

    case WM_RBUTTONUP:
      m_uiMouseButtonReceivedUp[1]++;
      ApplyClipRect(m_ClipCursorMode, hWnd);

      --s_iMouseCaptureCount;
      if (s_iMouseCaptureCount <= 0)
        ReleaseCapture();


      return;

    case WM_MBUTTONDOWN:
      m_uiMouseButtonReceivedDown[2]++;

      if (s_iMouseCaptureCount == 0)
        SetCapture(plMinWindows::ToNative(hWnd));
      ++s_iMouseCaptureCount;
      return;

    case WM_MBUTTONUP:
      m_uiMouseButtonReceivedUp[2]++;

      --s_iMouseCaptureCount;
      if (s_iMouseCaptureCount <= 0)
        ReleaseCapture();

      return;

    case WM_XBUTTONDOWN:
      if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
        m_uiMouseButtonReceivedDown[3]++;
      if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
        m_uiMouseButtonReceivedDown[4]++;

      if (s_iMouseCaptureCount == 0)
        SetCapture(plMinWindows::ToNative(hWnd));
      ++s_iMouseCaptureCount;

      return;

    case WM_XBUTTONUP:
      if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
        m_uiMouseButtonReceivedUp[3]++;
      if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
        m_uiMouseButtonReceivedUp[4]++;

      --s_iMouseCaptureCount;
      if (s_iMouseCaptureCount <= 0)
        ReleaseCapture();

      return;

    case WM_CAPTURECHANGED: // Sent to the window that is losing the mouse capture.
      s_iMouseCaptureCount = 0;
      return;

#else

    case WM_LBUTTONUP:
      ApplyClipRect(m_bClipCursor, hWnd);
      return;

#endif

    case WM_INPUT:
    {
      plUInt32 uiSize = 0;

      GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &uiSize, sizeof(RAWINPUTHEADER));

      if (uiSize == 0)
        return;

      plHybridArray<plUInt8, sizeof(RAWINPUT)> InputData;
      InputData.SetCountUninitialized(uiSize);

      if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &InputData[0], &uiSize, sizeof(RAWINPUTHEADER)) != uiSize)
        return;

      RAWINPUT* raw = (RAWINPUT*)&InputData[0];

      if (raw->header.dwType == RIM_TYPEKEYBOARD)
      {
        static bool bIgnoreNext = false;

        if (bIgnoreNext)
        {
          bIgnoreNext = false;
          return;
        }

        static bool bWasStupidLeftShift = false;

        const plUInt8 uiScanCode = static_cast<plUInt8>(raw->data.keyboard.MakeCode);
        const bool bIsExtended = (raw->data.keyboard.Flags & RI_KEY_E0) != 0;

        if (uiScanCode == 42 && bIsExtended) // 42 has to be special I guess
        {
          bWasStupidLeftShift = true;
          return;
        }

        plStringView sInputSlotName = plInputManager::ConvertScanCodeToEngineName(uiScanCode, bIsExtended);

        // On Windows this only happens with the Pause key, but it will actually send the 'Right Ctrl' key value
        // so we need to fix this manually
        if (raw->data.keyboard.Flags & RI_KEY_E1)
        {
          sInputSlotName = plInputSlot_KeyPause;
          bIgnoreNext = true;
        }

        // The Print key is sent as a two key sequence, first an 'extended left shift' and then the Numpad* key is sent
        // we ignore the first stupid shift key entirely and then modify the following Numpad* key
        // Note that the 'stupid shift' is sent along with several other keys as well (e.g. left/right/up/down arrows)
        // in these cases we can ignore them entirely, as the following key will have an unambiguous key code
        if (sInputSlotName == plInputSlot_KeyNumpadStar && bWasStupidLeftShift)
        {
          sInputSlotName = plInputSlot_KeyPrint;
        }

        bWasStupidLeftShift = false;

        int iRequest = raw->data.keyboard.MakeCode << 16;

        if (raw->data.keyboard.Flags & RI_KEY_E0)
          iRequest |= 1 << 24;

        const bool bPressed = !(raw->data.keyboard.Flags & 0x01);

        m_InputSlotValues[sInputSlotName] = bPressed ? 1.0f : 0.0f;

        if ((m_InputSlotValues[plInputSlot_KeyLeftCtrl] > 0.1f) && (m_InputSlotValues[plInputSlot_KeyLeftAlt] > 0.1f) &&
            (m_InputSlotValues[plInputSlot_KeyNumpadEnter] > 0.1f))
        {
          switch (GetClipMouseCursor())
          {
            case plMouseCursorClipMode::NoClip:
              SetClipMouseCursor(plMouseCursorClipMode::ClipToWindow);
              break;

            default:
              SetClipMouseCursor(plMouseCursorClipMode::NoClip);
              break;
          }
        }
      }
      else if (raw->header.dwType == RIM_TYPEMOUSE)
      {
        const plUInt32 uiButtons = raw->data.mouse.usButtonFlags;

        // "absolute" positions are only reported by devices such as Pens
        // if at all, we should handle them as touch points, not as mouse positions
        if ((raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
        {
          m_InputSlotValues[plInputSlot_MouseMoveNegX] +=
            ((raw->data.mouse.lLastX < 0) ? (float)-raw->data.mouse.lLastX : 0.0f) * GetMouseSpeed().x;
          m_InputSlotValues[plInputSlot_MouseMovePosX] +=
            ((raw->data.mouse.lLastX > 0) ? (float)raw->data.mouse.lLastX : 0.0f) * GetMouseSpeed().x;
          m_InputSlotValues[plInputSlot_MouseMoveNegY] +=
            ((raw->data.mouse.lLastY < 0) ? (float)-raw->data.mouse.lLastY : 0.0f) * GetMouseSpeed().y;
          m_InputSlotValues[plInputSlot_MouseMovePosY] +=
            ((raw->data.mouse.lLastY > 0) ? (float)raw->data.mouse.lLastY : 0.0f) * GetMouseSpeed().y;

// Mouse input does not always work via WM_INPUT
// e.g. some VMs don't send mouse click input via WM_INPUT when the mouse cursor is visible
// therefore in 'compatibility mode' it is just queried via standard WM_LBUTTONDOWN etc.
// to get 'high performance' mouse clicks, this code would work fine though
// but I doubt it makes much difference in latency
#if PLASMA_DISABLED(PLASMA_MOUSEBUTTON_COMPATIBILTY_MODE)
          for (plInt32 mb = 0; mb < 5; ++mb)
          {
            char szTemp[32];
            plStringUtils::snprintf(szTemp, 32, "mouse_button_%i", mb);

            if ((uiButtons & (RI_MOUSE_BUTTON_1_DOWN << (mb * 2))) != 0)
              m_InputSlotValues[szTemp] = 1.0f;

            if ((uiButtons & (RI_MOUSE_BUTTON_1_DOWN << (mb * 2 + 1))) != 0)
              m_InputSlotValues[szTemp] = 0.0f;
          }
#endif
        }
        else if ((raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
        {
          if ((raw->data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0)
          {
            // if this flag is set, we are getting mouse input through a remote desktop session
            // and that means we will not get any relative mouse move events, so we need to emulate them

            static const plInt32 iVirtualDesktopW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            static const plInt32 iVirtualDesktopH = GetSystemMetrics(SM_CYVIRTUALSCREEN);

            static plVec2 vLastPos(plMath::MaxValue<float>());
            const plVec2 vNewPos(
              (raw->data.mouse.lLastX / 65535.0f) * iVirtualDesktopW, (raw->data.mouse.lLastY / 65535.0f) * iVirtualDesktopH);

            if (vLastPos.x != plMath::MaxValue<float>())
            {
              const plVec2 vDiff = vNewPos - vLastPos;

              m_InputSlotValues[plInputSlot_MouseMoveNegX] += ((vDiff.x < 0) ? (float)-vDiff.x : 0.0f) * GetMouseSpeed().x;
              m_InputSlotValues[plInputSlot_MouseMovePosX] += ((vDiff.x > 0) ? (float)vDiff.x : 0.0f) * GetMouseSpeed().x;
              m_InputSlotValues[plInputSlot_MouseMoveNegY] += ((vDiff.y < 0) ? (float)-vDiff.y : 0.0f) * GetMouseSpeed().y;
              m_InputSlotValues[plInputSlot_MouseMovePosY] += ((vDiff.y > 0) ? (float)vDiff.y : 0.0f) * GetMouseSpeed().y;
            }

            vLastPos = vNewPos;
          }
          else
          {
            static int iTouchPoint = 0;
            static bool bTouchPointDown = false;

            plStringView sSlot = plInputManager::GetInputSlotTouchPoint(iTouchPoint);
            plStringView sSlotX = plInputManager::GetInputSlotTouchPointPositionX(iTouchPoint);
            plStringView sSlotY = plInputManager::GetInputSlotTouchPointPositionY(iTouchPoint);

            m_InputSlotValues[sSlotX] = (raw->data.mouse.lLastX / 65535.0f) + m_uiWindowNumber;
            m_InputSlotValues[sSlotY] = (raw->data.mouse.lLastY / 65535.0f);

            if ((uiButtons & (RI_MOUSE_BUTTON_1_DOWN | RI_MOUSE_BUTTON_2_DOWN)) != 0)
            {
              bTouchPointDown = true;
              m_InputSlotValues[sSlot] = 1.0f;
            }

            if ((uiButtons & (RI_MOUSE_BUTTON_1_UP | RI_MOUSE_BUTTON_2_UP)) != 0)
            {
              bTouchPointDown = false;
              m_InputSlotValues[sSlot] = 0.0f;
            }
          }
        }
        else
        {
          plLog::Info("Unknown Mouse Move: {0} | {1}, Flags = {2}", plArgF(raw->data.mouse.lLastX, 1), plArgF(raw->data.mouse.lLastY, 1),
            (plUInt32)raw->data.mouse.usFlags);
        }
      }
    }
  }
}


static void SetKeyNameForScanCode(int iScanCode, bool bExtended, const char* szInputSlot)
{
  const plUInt32 uiKeyCode = (iScanCode << 16) | (bExtended ? (1 << 24) : 0);

  wchar_t szKeyName[32] = {0};
  GetKeyNameTextW(uiKeyCode, szKeyName, 30);

  plStringUtf8 sName(szKeyName);

  plLog::Dev("Translated '{0}' to '{1}'", plInputManager::GetInputSlotDisplayName(szInputSlot), sName.GetData());

  plInputManager::SetInputSlotDisplayName(szInputSlot, sName.GetData());
}

void plStandardInputDevice::LocalizeButtonDisplayNames()
{
  PLASMA_LOG_BLOCK("plStandardInputDevice::LocalizeButtonDisplayNames");

  SetKeyNameForScanCode(1, false, plInputSlot_KeyEscape);
  SetKeyNameForScanCode(2, false, plInputSlot_Key1);
  SetKeyNameForScanCode(3, false, plInputSlot_Key2);
  SetKeyNameForScanCode(4, false, plInputSlot_Key3);
  SetKeyNameForScanCode(5, false, plInputSlot_Key4);
  SetKeyNameForScanCode(6, false, plInputSlot_Key5);
  SetKeyNameForScanCode(7, false, plInputSlot_Key6);
  SetKeyNameForScanCode(8, false, plInputSlot_Key7);
  SetKeyNameForScanCode(9, false, plInputSlot_Key8);
  SetKeyNameForScanCode(10, false, plInputSlot_Key9);
  SetKeyNameForScanCode(11, false, plInputSlot_Key0);

  SetKeyNameForScanCode(12, false, plInputSlot_KeyHyphen);
  SetKeyNameForScanCode(13, false, plInputSlot_KeyEquals);
  SetKeyNameForScanCode(14, false, plInputSlot_KeyBackspace);

  SetKeyNameForScanCode(15, false, plInputSlot_KeyTab);
  SetKeyNameForScanCode(16, false, plInputSlot_KeyQ);
  SetKeyNameForScanCode(17, false, plInputSlot_KeyW);
  SetKeyNameForScanCode(18, false, plInputSlot_KeyE);
  SetKeyNameForScanCode(19, false, plInputSlot_KeyR);
  SetKeyNameForScanCode(20, false, plInputSlot_KeyT);
  SetKeyNameForScanCode(21, false, plInputSlot_KeyY);
  SetKeyNameForScanCode(22, false, plInputSlot_KeyU);
  SetKeyNameForScanCode(23, false, plInputSlot_KeyI);
  SetKeyNameForScanCode(24, false, plInputSlot_KeyO);
  SetKeyNameForScanCode(25, false, plInputSlot_KeyP);
  SetKeyNameForScanCode(26, false, plInputSlot_KeyBracketOpen);
  SetKeyNameForScanCode(27, false, plInputSlot_KeyBracketClose);
  SetKeyNameForScanCode(28, false, plInputSlot_KeyReturn);

  SetKeyNameForScanCode(29, false, plInputSlot_KeyLeftCtrl);
  SetKeyNameForScanCode(30, false, plInputSlot_KeyA);
  SetKeyNameForScanCode(31, false, plInputSlot_KeyS);
  SetKeyNameForScanCode(32, false, plInputSlot_KeyD);
  SetKeyNameForScanCode(33, false, plInputSlot_KeyF);
  SetKeyNameForScanCode(34, false, plInputSlot_KeyG);
  SetKeyNameForScanCode(35, false, plInputSlot_KeyH);
  SetKeyNameForScanCode(36, false, plInputSlot_KeyJ);
  SetKeyNameForScanCode(37, false, plInputSlot_KeyK);
  SetKeyNameForScanCode(38, false, plInputSlot_KeyL);
  SetKeyNameForScanCode(39, false, plInputSlot_KeySemicolon);
  SetKeyNameForScanCode(40, false, plInputSlot_KeyApostrophe);

  SetKeyNameForScanCode(41, false, plInputSlot_KeyTilde);
  SetKeyNameForScanCode(42, false, plInputSlot_KeyLeftShift);
  SetKeyNameForScanCode(43, false, plInputSlot_KeyBackslash);

  SetKeyNameForScanCode(44, false, plInputSlot_KeyZ);
  SetKeyNameForScanCode(45, false, plInputSlot_KeyX);
  SetKeyNameForScanCode(46, false, plInputSlot_KeyC);
  SetKeyNameForScanCode(47, false, plInputSlot_KeyV);
  SetKeyNameForScanCode(48, false, plInputSlot_KeyB);
  SetKeyNameForScanCode(49, false, plInputSlot_KeyN);
  SetKeyNameForScanCode(50, false, plInputSlot_KeyM);
  SetKeyNameForScanCode(51, false, plInputSlot_KeyComma);
  SetKeyNameForScanCode(52, false, plInputSlot_KeyPeriod);
  SetKeyNameForScanCode(53, false, plInputSlot_KeySlash);
  SetKeyNameForScanCode(54, false, plInputSlot_KeyRightShift);

  SetKeyNameForScanCode(55, false, plInputSlot_KeyNumpadStar); // Overlaps with Print

  SetKeyNameForScanCode(56, false, plInputSlot_KeyLeftAlt);
  SetKeyNameForScanCode(57, false, plInputSlot_KeySpace);
  SetKeyNameForScanCode(58, false, plInputSlot_KeyCapsLock);

  SetKeyNameForScanCode(59, false, plInputSlot_KeyF1);
  SetKeyNameForScanCode(60, false, plInputSlot_KeyF2);
  SetKeyNameForScanCode(61, false, plInputSlot_KeyF3);
  SetKeyNameForScanCode(62, false, plInputSlot_KeyF4);
  SetKeyNameForScanCode(63, false, plInputSlot_KeyF5);
  SetKeyNameForScanCode(64, false, plInputSlot_KeyF6);
  SetKeyNameForScanCode(65, false, plInputSlot_KeyF7);
  SetKeyNameForScanCode(66, false, plInputSlot_KeyF8);
  SetKeyNameForScanCode(67, false, plInputSlot_KeyF9);
  SetKeyNameForScanCode(68, false, plInputSlot_KeyF10);

  SetKeyNameForScanCode(69, true, plInputSlot_KeyNumLock); // Prints 'Pause' if it is not 'extended'
  SetKeyNameForScanCode(70, false, plInputSlot_KeyScroll); // This overlaps with Pause

  SetKeyNameForScanCode(71, false, plInputSlot_KeyNumpad7); // This overlaps with Home
  SetKeyNameForScanCode(72, false, plInputSlot_KeyNumpad8); // This overlaps with Arrow Up
  SetKeyNameForScanCode(73, false, plInputSlot_KeyNumpad9); // This overlaps with Page Up
  SetKeyNameForScanCode(74, false, plInputSlot_KeyNumpadMinus);

  SetKeyNameForScanCode(75, false, plInputSlot_KeyNumpad4); // This overlaps with Arrow Left
  SetKeyNameForScanCode(76, false, plInputSlot_KeyNumpad5);
  SetKeyNameForScanCode(77, false, plInputSlot_KeyNumpad6); // This overlaps with Arrow Right
  SetKeyNameForScanCode(78, false, plInputSlot_KeyNumpadPlus);

  SetKeyNameForScanCode(79, false, plInputSlot_KeyNumpad1);      // This overlaps with End
  SetKeyNameForScanCode(80, false, plInputSlot_KeyNumpad2);      // This overlaps with Arrow Down
  SetKeyNameForScanCode(81, false, plInputSlot_KeyNumpad3);      // This overlaps with Page Down
  SetKeyNameForScanCode(82, false, plInputSlot_KeyNumpad0);      // This overlaps with Insert
  SetKeyNameForScanCode(83, false, plInputSlot_KeyNumpadPeriod); // This overlaps with Insert

  SetKeyNameForScanCode(86, false, plInputSlot_KeyPipe);

  SetKeyNameForScanCode(87, false, "keyboard_f11");
  SetKeyNameForScanCode(88, false, "keyboard_f12");

  SetKeyNameForScanCode(91, true, plInputSlot_KeyLeftWin);  // Prints '' if it is not 'extended'
  SetKeyNameForScanCode(92, true, plInputSlot_KeyRightWin); // Prints '' if it is not 'extended'
  SetKeyNameForScanCode(93, true, plInputSlot_KeyApps);     // Prints '' if it is not 'extended'

  // 'Extended' keys
  SetKeyNameForScanCode(28, true, plInputSlot_KeyNumpadEnter);
  SetKeyNameForScanCode(29, true, plInputSlot_KeyRightCtrl);
  SetKeyNameForScanCode(53, true, plInputSlot_KeyNumpadSlash);
  SetKeyNameForScanCode(55, true, plInputSlot_KeyPrint);
  SetKeyNameForScanCode(56, true, plInputSlot_KeyRightAlt);
  SetKeyNameForScanCode(70, true, plInputSlot_KeyPause);
  SetKeyNameForScanCode(71, true, plInputSlot_KeyHome);
  SetKeyNameForScanCode(72, true, plInputSlot_KeyUp);
  SetKeyNameForScanCode(73, true, plInputSlot_KeyPageUp);

  SetKeyNameForScanCode(75, true, plInputSlot_KeyLeft);
  SetKeyNameForScanCode(77, true, plInputSlot_KeyRight);

  SetKeyNameForScanCode(79, true, plInputSlot_KeyEnd);
  SetKeyNameForScanCode(80, true, plInputSlot_KeyDown);
  SetKeyNameForScanCode(81, true, plInputSlot_KeyPageDown);
  SetKeyNameForScanCode(82, true, plInputSlot_KeyInsert);
  SetKeyNameForScanCode(83, true, plInputSlot_KeyDelete);
}

void plStandardInputDevice::SetShowMouseCursor(bool bShow)
{
  if (m_bShowCursor == bShow)
    return;

  m_bShowCursor = bShow;
  ShowCursor(m_bShowCursor);
}

bool plStandardInputDevice::GetShowMouseCursor() const
{
  return m_bShowCursor;
}

void plStandardInputDevice::OnFocusLost(plMinWindows::HWND hWnd)
{
  m_bApplyClipRect = true;
  ApplyClipRect(plMouseCursorClipMode::NoClip, hWnd);

  auto it = m_InputSlotValues.GetIterator();

  while (it.IsValid())
  {
    it.Value() = 0.0f;
    it.Next();
  }


  const char* slotDown[5] = {plInputSlot_MouseButton0, plInputSlot_MouseButton1, plInputSlot_MouseButton2, plInputSlot_MouseButton3, plInputSlot_MouseButton4};

  static_assert(PLASMA_ARRAY_SIZE(m_uiMouseButtonReceivedDown) == PLASMA_ARRAY_SIZE(slotDown));

  for (int i = 0; i < PLASMA_ARRAY_SIZE(m_uiMouseButtonReceivedDown); ++i)
  {
    m_uiMouseButtonReceivedDown[i] = 0;
    m_uiMouseButtonReceivedUp[i] = 0;

    m_InputSlotValues[slotDown[i]] = 0;
  }
}

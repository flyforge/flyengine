#include <Core/Input/InputManager.h>
#include <Core/System/Implementation/uwp/InputDevice_uwp.h>
#include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringConversion.h>
#include <wrl/event.h>

using namespace ABI::Windows::UI::Core;

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plStandardInputDevice, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStandardInputDevice::plStandardInputDevice(ICoreWindow* coreWindow)
  : m_coreWindow(coreWindow)
{
  // TODO
  m_ClipCursorMode = plMouseCursorClipMode::NoClip;
  m_bShowCursor = true;
}

plStandardInputDevice::~plStandardInputDevice()
{
  if (m_coreWindow)
  {
    m_coreWindow->remove_KeyDown(m_eventRegistration_keyDown);
    m_coreWindow->remove_KeyUp(m_eventRegistration_keyUp);
    m_coreWindow->remove_CharacterReceived(m_eventRegistration_characterReceived);
    m_coreWindow->remove_PointerMoved(m_eventRegistration_pointerMoved);
    m_coreWindow->remove_PointerEntered(m_eventRegistration_pointerEntered);
    m_coreWindow->remove_PointerExited(m_eventRegistration_pointerExited);
    m_coreWindow->remove_PointerCaptureLost(m_eventRegistration_pointerCaptureLost);
    m_coreWindow->remove_PointerPressed(m_eventRegistration_pointerPressed);
    m_coreWindow->remove_PointerReleased(m_eventRegistration_pointerReleased);
    m_coreWindow->remove_PointerWheelChanged(m_eventRegistration_pointerWheelChanged);
  }

  if (m_mouseDevice)
  {
    m_mouseDevice->remove_MouseMoved(m_eventRegistration_mouseMoved);
  }
}

void plStandardInputDevice::InitializeDevice()
{
  using KeyHandler = __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CKeyEventArgs;
  using CharacterReceivedHandler =
    __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCharacterReceivedEventArgs;
  using PointerHander = __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs;

  // Keyboard
  m_coreWindow->add_KeyDown(Callback<KeyHandler>(this, &plStandardInputDevice::OnKeyEvent).Get(), &m_eventRegistration_keyDown);
  m_coreWindow->add_KeyUp(Callback<KeyHandler>(this, &plStandardInputDevice::OnKeyEvent).Get(), &m_eventRegistration_keyUp);
  m_coreWindow->add_CharacterReceived(Callback<CharacterReceivedHandler>(this, &plStandardInputDevice::OnCharacterReceived).Get(),
    &m_eventRegistration_characterReceived);

  // Pointer
  // Note that a pointer may be mouse, pen/stylus or touch!
  // We bundle move/press/enter all in a single callback to update all pointer state - all these cases have in common that pen/touch is
  // pressed now.
  m_coreWindow->add_PointerMoved(Callback<PointerHander>(this, &plStandardInputDevice::OnPointerMovePressEnter).Get(),
    &m_eventRegistration_pointerMoved);
  m_coreWindow->add_PointerEntered(Callback<PointerHander>(this, &plStandardInputDevice::OnPointerMovePressEnter).Get(),
    &m_eventRegistration_pointerEntered);
  m_coreWindow->add_PointerPressed(Callback<PointerHander>(this, &plStandardInputDevice::OnPointerMovePressEnter).Get(),
    &m_eventRegistration_pointerPressed);
  // Changes in the pointer wheel:
  m_coreWindow->add_PointerWheelChanged(Callback<PointerHander>(this, &plStandardInputDevice::OnPointerWheelChange).Get(),
    &m_eventRegistration_pointerWheelChanged);
  // Exit for touch or stylus means that we no longer have a press.
  // However, we presserve mouse button presses.
  m_coreWindow->add_PointerExited(Callback<PointerHander>(this, &plStandardInputDevice::OnPointerReleasedOrExited).Get(),
    &m_eventRegistration_pointerExited);
  m_coreWindow->add_PointerReleased(Callback<PointerHander>(this, &plStandardInputDevice::OnPointerReleasedOrExited).Get(),
    &m_eventRegistration_pointerReleased);
  // Capture loss.
  // From documentation "Occurs when a pointer moves to another app. This event is raised after PointerExited and is the final event
  // received by the app for this pointer." If this happens we want to release all mouse buttons as well.
  m_coreWindow->add_PointerCaptureLost(Callback<PointerHander>(this, &plStandardInputDevice::OnPointerCaptureLost).Get(),
    &m_eventRegistration_pointerCaptureLost);


  // Mouse
  // The only thing that we get from the MouseDevice class is mouse moved which gives us unfiltered relative mouse position.
  // Everything else is done by WinRt's "Pointer"
  // https://docs.microsoft.com/uwp/api/windows.devices.input.mousedevice
  // Relevant article for mouse move:
  // https://docs.microsoft.com/windows/uwp/gaming/relative-mouse-movement
  {
    ComPtr<ABI::Windows::Devices::Input::IMouseDeviceStatics> mouseDeviceStatics;
    if (SUCCEEDED(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_Devices_Input_MouseDevice).Get(),
          &mouseDeviceStatics)))
    {
      if (SUCCEEDED(mouseDeviceStatics->GetForCurrentView(&m_mouseDevice)))
      {
        using MouseMovedHandler =
          __FITypedEventHandler_2_Windows__CDevices__CInput__CMouseDevice_Windows__CDevices__CInput__CMouseEventArgs;
        m_mouseDevice->add_MouseMoved(Callback<MouseMovedHandler>(this, &plStandardInputDevice::OnMouseMoved).Get(),
          &m_eventRegistration_mouseMoved);
      }
    }
  }
}

HRESULT plStandardInputDevice::OnKeyEvent(ICoreWindow* coreWindow, IKeyEventArgs* args)
{
  // Closely related to the RawInput implementation in Win32/InputDevice_win32.inl

  CorePhysicalKeyStatus keyStatus;
  PL_SUCCEED_OR_RETURN(args->get_KeyStatus(&keyStatus));

  static bool bWasStupidLeftShift = false;

  if (keyStatus.ScanCode == 42 && keyStatus.IsExtendedKey) // 42 has to be special I guess
  {
    bWasStupidLeftShift = true;
    return S_OK;
  }

  plStringView sInputSlotName = plInputManager::ConvertScanCodeToEngineName(static_cast<plUInt8>(keyStatus.ScanCode), keyStatus.IsExtendedKey == TRUE);
  if (sInputSlotName.IsEmpty())
    return S_OK;


  // Don't know yet how to handle this in UWP:

  // On Windows this only happens with the Pause key, but it will actually send the 'Right Ctrl' key value
  // so we need to fix this manually
  // if (raw->data.keyboard.Flags & RI_KEY_E1)
  //{
  //  szInputSlotName = plInputSlot_KeyPause;
  //  bIgnoreNext = true;
  //}


  // The Print key is sent as a two key sequence, first an 'extended left shift' and then the Numpad* key is sent
  // we ignore the first stupid shift key entirely and then modify the following Numpad* key
  // Note that the 'stupid shift' is sent along with several other keys as well (e.g. left/right/up/down arrows)
  // in these cases we can ignore them entirely, as the following key will have an unambiguous key code
  if (sInputSlotName == plInputSlot_KeyNumpadStar && bWasStupidLeftShift)
    sInputSlotName = plInputSlot_KeyPrint;

  bWasStupidLeftShift = false;

  m_InputSlotValues[sInputSlotName] = keyStatus.IsKeyReleased ? 0.0f : 1.0f;

  return S_OK;
}

HRESULT plStandardInputDevice::OnCharacterReceived(ICoreWindow* coreWindow, ICharacterReceivedEventArgs* args)
{
  UINT32 keyCode = 0;
  PL_SUCCEED_OR_RETURN(args->get_KeyCode(&keyCode));
  m_uiLastCharacter = keyCode;

  return S_OK;
}

HRESULT plStandardInputDevice::OnPointerMovePressEnter(ICoreWindow* coreWindow, IPointerEventArgs* args)
{
  using namespace ABI::Windows::Devices::Input;

  ComPtr<ABI::Windows::UI::Input::IPointerPoint> pointerPoint;
  PL_SUCCEED_OR_RETURN(args->get_CurrentPoint(&pointerPoint));
  ComPtr<IPointerDevice> pointerDevice;
  PL_SUCCEED_OR_RETURN(pointerPoint->get_PointerDevice(&pointerDevice));
  PointerDeviceType deviceType;
  PL_SUCCEED_OR_RETURN(pointerDevice->get_PointerDeviceType(&deviceType));

  // Pointer position.
  // From the documentation: "The position of the pointer in device-independent pixel (DIP)."
  // Note also, that there is "raw position" which may be free of pointer prediction etc.
  ABI::Windows::Foundation::Point pointerPosition;
  PL_SUCCEED_OR_RETURN(pointerPoint->get_Position(&pointerPosition));
  ABI::Windows::Foundation::Rect windowRectangle;
  PL_SUCCEED_OR_RETURN(coreWindow->get_Bounds(&windowRectangle)); // Bounds are in DIP as well!

  float relativePosX = static_cast<float>(pointerPosition.X) / windowRectangle.Width;
  float relativePosY = static_cast<float>(pointerPosition.Y) / windowRectangle.Height;

  if (deviceType == PointerDeviceType_Mouse)
  {
    // TODO
    // RegisterInputSlot(plInputSlot_MouseDblClick0, "Left Double Click", plInputSlotFlags::IsDoubleClick);
    // RegisterInputSlot(plInputSlot_MouseDblClick1, "Right Double Click", plInputSlotFlags::IsDoubleClick);
    // RegisterInputSlot(plInputSlot_MouseDblClick2, "Middle Double Click", plInputSlotFlags::IsDoubleClick);

    s_iMouseIsOverWindowNumber = 0;
    m_InputSlotValues[plInputSlot_MousePositionX] = relativePosX;
    m_InputSlotValues[plInputSlot_MousePositionY] = relativePosY;

    PL_SUCCEED_OR_RETURN(UpdateMouseButtonStates(pointerPoint.Get()));
  }
  else // Touch AND Pen
  {
    // WinRT treats each touch point as unique pointer.
    UINT32 pointerId;
    PL_SUCCEED_OR_RETURN(pointerPoint->get_PointerId(&pointerId));
    if (pointerId > 9)
      return S_OK;

    // All callbacks we subscribed this event to imply that a touch occurs right now.
    m_InputSlotValues[plInputManager::GetInputSlotTouchPoint(pointerId)] = 1.0f; // Touch strength?
    m_InputSlotValues[plInputManager::GetInputSlotTouchPointPositionX(pointerId)] = relativePosX;
    m_InputSlotValues[plInputManager::GetInputSlotTouchPointPositionY(pointerId)] = relativePosY;
  }

  return S_OK;
}

HRESULT plStandardInputDevice::OnPointerWheelChange(ICoreWindow* coreWindow, IPointerEventArgs* args)
{
  using namespace ABI::Windows::Devices::Input;

  ComPtr<ABI::Windows::UI::Input::IPointerPoint> pointerPoint;
  PL_SUCCEED_OR_RETURN(args->get_CurrentPoint(&pointerPoint));
  ComPtr<IPointerDevice> pointerDevice;
  PL_SUCCEED_OR_RETURN(pointerPoint->get_PointerDevice(&pointerDevice));

  // Only interested in mouse devices.
  PointerDeviceType deviceType;
  PL_SUCCEED_OR_RETURN(pointerDevice->get_PointerDeviceType(&deviceType));
  if (deviceType == PointerDeviceType_Mouse)
  {
    ComPtr<ABI::Windows::UI::Input::IPointerPointProperties> properties;
    PL_SUCCEED_OR_RETURN(pointerPoint->get_Properties(&properties));

    // .. and only vertical wheels.
    boolean isHorizontalWheel;
    PL_SUCCEED_OR_RETURN(properties->get_IsHorizontalMouseWheel(&isHorizontalWheel));
    if (!isHorizontalWheel)
    {
      INT32 delta;
      PL_SUCCEED_OR_RETURN(properties->get_MouseWheelDelta(&delta));

      if (delta > 0)
        m_InputSlotValues[plInputSlot_MouseWheelUp] = delta / 120.0f;
      else
        m_InputSlotValues[plInputSlot_MouseWheelDown] = -delta / 120.0f;
    }
  }

  return S_OK;
}

HRESULT plStandardInputDevice::OnPointerReleasedOrExited(ICoreWindow* coreWindow, IPointerEventArgs* args)
{
  using namespace ABI::Windows::Devices::Input;

  ComPtr<ABI::Windows::UI::Input::IPointerPoint> pointerPoint;
  PL_SUCCEED_OR_RETURN(args->get_CurrentPoint(&pointerPoint));
  ComPtr<IPointerDevice> pointerDevice;
  PL_SUCCEED_OR_RETURN(pointerPoint->get_PointerDevice(&pointerDevice));
  PointerDeviceType deviceType;
  PL_SUCCEED_OR_RETURN(pointerDevice->get_PointerDeviceType(&deviceType));

  if (deviceType == PointerDeviceType_Mouse)
  {
    // Note that the relased event is only fired if the last mouse button is released according to documentation.
    // However, we're also subscribing to exit and depending on the mouse capture this may or may not be a button release.
    PL_SUCCEED_OR_RETURN(UpdateMouseButtonStates(pointerPoint.Get()));
  }
  else // Touch AND Pen
  {
    // WinRT treats each touch point as unique pointer.
    UINT32 pointerId;
    PL_SUCCEED_OR_RETURN(pointerPoint->get_PointerId(&pointerId));
    if (pointerId > 9)
      return S_OK;

    m_InputSlotValues[plInputManager::GetInputSlotTouchPoint(pointerId)] = 0.0f;
  }

  return S_OK;
}

HRESULT plStandardInputDevice::OnPointerCaptureLost(ICoreWindow* coreWindow, IPointerEventArgs* args)
{
  using namespace ABI::Windows::Devices::Input;

  ComPtr<ABI::Windows::UI::Input::IPointerPoint> pointerPoint;
  PL_SUCCEED_OR_RETURN(args->get_CurrentPoint(&pointerPoint));
  ComPtr<IPointerDevice> pointerDevice;
  PL_SUCCEED_OR_RETURN(pointerPoint->get_PointerDevice(&pointerDevice));
  PointerDeviceType deviceType;
  PL_SUCCEED_OR_RETURN(pointerDevice->get_PointerDeviceType(&deviceType));

  if (deviceType == PointerDeviceType_Mouse)
  {
    m_InputSlotValues[plInputSlot_MouseButton0] = 0.0f;
    m_InputSlotValues[plInputSlot_MouseButton1] = 0.0f;
    m_InputSlotValues[plInputSlot_MouseButton2] = 0.0f;
    m_InputSlotValues[plInputSlot_MouseButton3] = 0.0f;
    m_InputSlotValues[plInputSlot_MouseButton4] = 0.0f;
  }
  else // Touch AND Pen
  {
    // WinRT treats each touch point as unique pointer.
    UINT32 pointerId;
    PL_SUCCEED_OR_RETURN(pointerPoint->get_PointerId(&pointerId));
    if (pointerId > 9)
      return S_OK;

    m_InputSlotValues[plInputManager::GetInputSlotTouchPoint(pointerId)] = 0.0f;
  }

  return S_OK;
}

HRESULT plStandardInputDevice::OnMouseMoved(ABI::Windows::Devices::Input::IMouseDevice* mouseDevice,
  ABI::Windows::Devices::Input::IMouseEventArgs* args)
{
  ABI::Windows::Devices::Input::MouseDelta mouseDelta;
  PL_SUCCEED_OR_RETURN(args->get_MouseDelta(&mouseDelta));

  m_InputSlotValues[plInputSlot_MouseMoveNegX] += ((mouseDelta.X < 0) ? static_cast<float>(-mouseDelta.X) : 0.0f) * GetMouseSpeed().x;
  m_InputSlotValues[plInputSlot_MouseMovePosX] += ((mouseDelta.X > 0) ? static_cast<float>(mouseDelta.X) : 0.0f) * GetMouseSpeed().x;
  m_InputSlotValues[plInputSlot_MouseMoveNegY] += ((mouseDelta.Y < 0) ? static_cast<float>(-mouseDelta.Y) : 0.0f) * GetMouseSpeed().y;
  m_InputSlotValues[plInputSlot_MouseMovePosY] += ((mouseDelta.Y > 0) ? static_cast<float>(mouseDelta.Y) : 0.0f) * GetMouseSpeed().y;

  return S_OK;
}

HRESULT plStandardInputDevice::UpdateMouseButtonStates(ABI::Windows::UI::Input::IPointerPoint* pointerPoint)
{
  ComPtr<ABI::Windows::UI::Input::IPointerPointProperties> properties;
  PL_SUCCEED_OR_RETURN(pointerPoint->get_Properties(&properties));

  boolean isPressed;
  PL_SUCCEED_OR_RETURN(properties->get_IsLeftButtonPressed(&isPressed));
  m_InputSlotValues[plInputSlot_MouseButton0] = isPressed ? 1.0f : 0.0f;
  PL_SUCCEED_OR_RETURN(properties->get_IsRightButtonPressed(&isPressed));
  m_InputSlotValues[plInputSlot_MouseButton1] = isPressed ? 1.0f : 0.0f;
  PL_SUCCEED_OR_RETURN(properties->get_IsMiddleButtonPressed(&isPressed));
  m_InputSlotValues[plInputSlot_MouseButton2] = isPressed ? 1.0f : 0.0f;
  PL_SUCCEED_OR_RETURN(properties->get_IsXButton1Pressed(&isPressed));
  m_InputSlotValues[plInputSlot_MouseButton3] = isPressed ? 1.0f : 0.0f;
  PL_SUCCEED_OR_RETURN(properties->get_IsXButton2Pressed(&isPressed));
  m_InputSlotValues[plInputSlot_MouseButton4] = isPressed ? 1.0f : 0.0f;

  return S_OK;
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

  // RegisterInputSlot(plInputSlot_KeyPrevTrack, "Previous Track", plInputSlotFlags::IsButton);
  // RegisterInputSlot(plInputSlot_KeyNextTrack, "Next Track", plInputSlotFlags::IsButton);
  // RegisterInputSlot(plInputSlot_KeyPlayPause, "Play / Pause", plInputSlotFlags::IsButton);
  // RegisterInputSlot(plInputSlot_KeyStop, "Stop", plInputSlotFlags::IsButton);
  // RegisterInputSlot(plInputSlot_KeyVolumeUp, "Volume Up", plInputSlotFlags::IsButton);
  // RegisterInputSlot(plInputSlot_KeyVolumeDown, "Volume Down", plInputSlotFlags::IsButton);
  // RegisterInputSlot(plInputSlot_KeyMute, "Mute", plInputSlotFlags::IsButton);

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


  // Not yet supported
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

void SetClipRect(bool bClip, HWND hWnd)
{
  // NOT IMPLEMENTED. TODO
}

void plStandardInputDevice::SetClipMouseCursor(plMouseCursorClipMode::Enum mode)
{
  if (m_ClipCursorMode == mode)
    return;

  if (mode != plMouseCursorClipMode::NoClip)
    m_coreWindow->SetPointerCapture();
  else
    m_coreWindow->ReleasePointerCapture();

  m_ClipCursorMode = mode;
}

void plStandardInputDevice::SetShowMouseCursor(bool bShow)
{
  if (m_bShowCursor == bShow)
    return;

  // Hide
  if (!bShow)
  {
    // Save cursor to reinstantiate it.
    m_coreWindow->get_PointerCursor(&m_cursorBeforeHide);
    m_coreWindow->put_PointerCursor(nullptr);
  }

  // Show
  else
  {
    PL_ASSERT_DEV(m_cursorBeforeHide, "There should be a ICoreCursor backup that can be put back.");
    m_coreWindow->put_PointerCursor(m_cursorBeforeHide.Get());
  }

  m_bShowCursor = bShow;
}

bool plStandardInputDevice::GetShowMouseCursor() const
{
  return m_bShowCursor;
}

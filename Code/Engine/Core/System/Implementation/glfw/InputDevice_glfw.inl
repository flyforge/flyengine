#include <Core/System/Implementation/glfw/InputDevice_glfw.h>
#include <GLFW/glfw3.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plStandardInputDevice, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  const char* ConvertGLFWKeyToEngineName(int key)
  {
    switch (key)
    {
      case GLFW_KEY_LEFT:
        return plInputSlot_KeyLeft;
      case GLFW_KEY_RIGHT:
        return plInputSlot_KeyRight;
      case GLFW_KEY_UP:
        return plInputSlot_KeyUp;
      case GLFW_KEY_DOWN:
        return plInputSlot_KeyDown;
      case GLFW_KEY_ESCAPE:
        return plInputSlot_KeyEscape;
      case GLFW_KEY_SPACE:
        return plInputSlot_KeySpace;
      case GLFW_KEY_BACKSPACE:
        return plInputSlot_KeyBackspace;
      case GLFW_KEY_ENTER:
        return plInputSlot_KeyReturn;
      case GLFW_KEY_TAB:
        return plInputSlot_KeyTab;
      case GLFW_KEY_LEFT_SHIFT:
        return plInputSlot_KeyLeftShift;
      case GLFW_KEY_RIGHT_SHIFT:
        return plInputSlot_KeyRightShift;
      case GLFW_KEY_LEFT_CONTROL:
        return plInputSlot_KeyLeftCtrl;
      case GLFW_KEY_RIGHT_CONTROL:
        return plInputSlot_KeyRightCtrl;
      case GLFW_KEY_LEFT_ALT:
        return plInputSlot_KeyLeftAlt;
      case GLFW_KEY_RIGHT_ALT:
        return plInputSlot_KeyRightAlt;
      case GLFW_KEY_LEFT_SUPER:
        return plInputSlot_KeyLeftWin;
      case GLFW_KEY_RIGHT_SUPER:
        return plInputSlot_KeyRightWin;
      case GLFW_KEY_MENU:
        return plInputSlot_KeyApps;
      case GLFW_KEY_LEFT_BRACKET:
        return plInputSlot_KeyBracketOpen;
      case GLFW_KEY_RIGHT_BRACKET:
        return plInputSlot_KeyBracketClose;
      case GLFW_KEY_SEMICOLON:
        return plInputSlot_KeySemicolon;
      case GLFW_KEY_APOSTROPHE:
        return plInputSlot_KeyApostrophe;
      case GLFW_KEY_SLASH:
        return plInputSlot_KeySlash;
      case GLFW_KEY_EQUAL:
        return plInputSlot_KeyEquals;
      case GLFW_KEY_GRAVE_ACCENT:
        return plInputSlot_KeyTilde;
      case GLFW_KEY_MINUS:
        return plInputSlot_KeyHyphen;
      case GLFW_KEY_COMMA:
        return plInputSlot_KeyComma;
      case GLFW_KEY_PERIOD:
        return plInputSlot_KeyPeriod;
      case GLFW_KEY_BACKSLASH:
        return plInputSlot_KeyBackslash;
      case GLFW_KEY_WORLD_1:
        return plInputSlot_KeyPipe;
      case GLFW_KEY_1:
        return plInputSlot_Key1;
      case GLFW_KEY_2:
        return plInputSlot_Key2;
      case GLFW_KEY_3:
        return plInputSlot_Key3;
      case GLFW_KEY_4:
        return plInputSlot_Key4;
      case GLFW_KEY_5:
        return plInputSlot_Key5;
      case GLFW_KEY_6:
        return plInputSlot_Key6;
      case GLFW_KEY_7:
        return plInputSlot_Key7;
      case GLFW_KEY_8:
        return plInputSlot_Key8;
      case GLFW_KEY_9:
        return plInputSlot_Key9;
      case GLFW_KEY_0:
        return plInputSlot_Key0;
      case GLFW_KEY_KP_1:
        return plInputSlot_KeyNumpad1;
      case GLFW_KEY_KP_2:
        return plInputSlot_KeyNumpad2;
      case GLFW_KEY_KP_3:
        return plInputSlot_KeyNumpad3;
      case GLFW_KEY_KP_4:
        return plInputSlot_KeyNumpad4;
      case GLFW_KEY_KP_5:
        return plInputSlot_KeyNumpad5;
      case GLFW_KEY_KP_6:
        return plInputSlot_KeyNumpad6;
      case GLFW_KEY_KP_7:
        return plInputSlot_KeyNumpad7;
      case GLFW_KEY_KP_8:
        return plInputSlot_KeyNumpad8;
      case GLFW_KEY_KP_9:
        return plInputSlot_KeyNumpad9;
      case GLFW_KEY_KP_0:
        return plInputSlot_KeyNumpad0;
      case GLFW_KEY_A:
        return plInputSlot_KeyA;
      case GLFW_KEY_B:
        return plInputSlot_KeyB;
      case GLFW_KEY_C:
        return plInputSlot_KeyC;
      case GLFW_KEY_D:
        return plInputSlot_KeyD;
      case GLFW_KEY_E:
        return plInputSlot_KeyE;
      case GLFW_KEY_F:
        return plInputSlot_KeyF;
      case GLFW_KEY_G:
        return plInputSlot_KeyG;
      case GLFW_KEY_H:
        return plInputSlot_KeyH;
      case GLFW_KEY_I:
        return plInputSlot_KeyI;
      case GLFW_KEY_J:
        return plInputSlot_KeyJ;
      case GLFW_KEY_K:
        return plInputSlot_KeyK;
      case GLFW_KEY_L:
        return plInputSlot_KeyL;
      case GLFW_KEY_M:
        return plInputSlot_KeyM;
      case GLFW_KEY_N:
        return plInputSlot_KeyN;
      case GLFW_KEY_O:
        return plInputSlot_KeyO;
      case GLFW_KEY_P:
        return plInputSlot_KeyP;
      case GLFW_KEY_Q:
        return plInputSlot_KeyQ;
      case GLFW_KEY_R:
        return plInputSlot_KeyR;
      case GLFW_KEY_S:
        return plInputSlot_KeyS;
      case GLFW_KEY_T:
        return plInputSlot_KeyT;
      case GLFW_KEY_U:
        return plInputSlot_KeyU;
      case GLFW_KEY_V:
        return plInputSlot_KeyV;
      case GLFW_KEY_W:
        return plInputSlot_KeyW;
      case GLFW_KEY_X:
        return plInputSlot_KeyX;
      case GLFW_KEY_Y:
        return plInputSlot_KeyY;
      case GLFW_KEY_Z:
        return plInputSlot_KeyZ;
      case GLFW_KEY_F1:
        return plInputSlot_KeyF1;
      case GLFW_KEY_F2:
        return plInputSlot_KeyF2;
      case GLFW_KEY_F3:
        return plInputSlot_KeyF3;
      case GLFW_KEY_F4:
        return plInputSlot_KeyF4;
      case GLFW_KEY_F5:
        return plInputSlot_KeyF5;
      case GLFW_KEY_F6:
        return plInputSlot_KeyF6;
      case GLFW_KEY_F7:
        return plInputSlot_KeyF7;
      case GLFW_KEY_F8:
        return plInputSlot_KeyF8;
      case GLFW_KEY_F9:
        return plInputSlot_KeyF9;
      case GLFW_KEY_F10:
        return plInputSlot_KeyF10;
      case GLFW_KEY_F11:
        return plInputSlot_KeyF11;
      case GLFW_KEY_F12:
        return plInputSlot_KeyF12;
      case GLFW_KEY_HOME:
        return plInputSlot_KeyHome;
      case GLFW_KEY_END:
        return plInputSlot_KeyEnd;
      case GLFW_KEY_DELETE:
        return plInputSlot_KeyDelete;
      case GLFW_KEY_INSERT:
        return plInputSlot_KeyInsert;
      case GLFW_KEY_PAGE_UP:
        return plInputSlot_KeyPageUp;
      case GLFW_KEY_PAGE_DOWN:
        return plInputSlot_KeyPageDown;
      case GLFW_KEY_NUM_LOCK:
        return plInputSlot_KeyNumLock;
      case GLFW_KEY_KP_ADD:
        return plInputSlot_KeyNumpadPlus;
      case GLFW_KEY_KP_SUBTRACT:
        return plInputSlot_KeyNumpadMinus;
      case GLFW_KEY_KP_MULTIPLY:
        return plInputSlot_KeyNumpadStar;
      case GLFW_KEY_KP_DIVIDE:
        return plInputSlot_KeyNumpadSlash;
      case GLFW_KEY_KP_DECIMAL:
        return plInputSlot_KeyNumpadPeriod;
      case GLFW_KEY_KP_ENTER:
        return plInputSlot_KeyNumpadEnter;
      case GLFW_KEY_CAPS_LOCK:
        return plInputSlot_KeyCapsLock;
      case GLFW_KEY_PRINT_SCREEN:
        return plInputSlot_KeyPrint;
      case GLFW_KEY_SCROLL_LOCK:
        return plInputSlot_KeyScroll;
      case GLFW_KEY_PAUSE:
        return plInputSlot_KeyPause;
      // TODO plInputSlot_KeyPrevTrack
      // TODO plInputSlot_KeyNextTrack
      // TODO plInputSlot_KeyPlayPause
      // TODO plInputSlot_KeyStop
      // TODO plInputSlot_KeyVolumeUp
      // TODO plInputSlot_KeyVolumeDown
      // TODO plInputSlot_KeyMute
      default:
        return nullptr;
    }
  }
} // namespace

plStandardInputDevice::plStandardInputDevice(plUInt32 uiWindowNumber, GLFWwindow* windowHandle)
  : m_uiWindowNumber(uiWindowNumber)
  , m_pWindow(windowHandle)
{
}

plStandardInputDevice::~plStandardInputDevice()
{
}

void plStandardInputDevice::SetShowMouseCursor(bool bShow)
{
  glfwSetInputMode(m_pWindow, GLFW_CURSOR, bShow ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

bool plStandardInputDevice::GetShowMouseCursor() const
{
  return (glfwGetInputMode(m_pWindow, GLFW_CURSOR) != GLFW_CURSOR_DISABLED);
}

void plStandardInputDevice::SetClipMouseCursor(plMouseCursorClipMode::Enum mode)
{
}

plMouseCursorClipMode::Enum plStandardInputDevice::GetClipMouseCursor() const
{
  return plMouseCursorClipMode::Default;
}

void plStandardInputDevice::InitializeDevice() {}

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
  // TODO RegisterInputSlot(plInputSlot_KeyTilde, "~", plInputSlotFlags::IsButton);
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

  /* TODO
  RegisterInputSlot(plInputSlot_KeyPrevTrack, "Previous Track", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyNextTrack, "Next Track", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyPlayPause, "Play / Pause", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyStop, "Stop", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyVolumeUp, "Volume Up", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyVolumeDown, "Volume Down", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_KeyMute, "Mute", plInputSlotFlags::IsButton);
  */

  RegisterInputSlot(plInputSlot_MousePositionX, "Mouse Position X", plInputSlotFlags::IsMouseAxisPosition);
  RegisterInputSlot(plInputSlot_MousePositionY, "Mouse Position Y", plInputSlotFlags::IsMouseAxisPosition);

  RegisterInputSlot(plInputSlot_MouseMoveNegX, "Mouse Move Left", plInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(plInputSlot_MouseMovePosX, "Mouse Move Right", plInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(plInputSlot_MouseMoveNegY, "Mouse Move Down", plInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(plInputSlot_MouseMovePosY, "Mouse Move Up", plInputSlotFlags::IsMouseAxisMove);

  RegisterInputSlot(plInputSlot_MouseButton0, "Mousebutton 0", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_MouseButton1, "Mousebutton 1", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_MouseButton2, "Mousebutton 2", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_MouseButton3, "Mousebutton 3", plInputSlotFlags::IsButton);
  RegisterInputSlot(plInputSlot_MouseButton4, "Mousebutton 4", plInputSlotFlags::IsButton);

  RegisterInputSlot(plInputSlot_MouseWheelUp, "Mousewheel Up", plInputSlotFlags::IsMouseWheel);
  RegisterInputSlot(plInputSlot_MouseWheelDown, "Mousewheel Down", plInputSlotFlags::IsMouseWheel);
}

void plStandardInputDevice::ResetInputSlotValues()
{
  m_InputSlotValues[plInputSlot_MouseWheelUp] = 0;
  m_InputSlotValues[plInputSlot_MouseWheelDown] = 0;
  m_InputSlotValues[plInputSlot_MouseMoveNegX] = 0;
  m_InputSlotValues[plInputSlot_MouseMovePosX] = 0;
  m_InputSlotValues[plInputSlot_MouseMoveNegY] = 0;
  m_InputSlotValues[plInputSlot_MouseMovePosY] = 0;
}

void plStandardInputDevice::OnKey(int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_BACKSPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
  {
    m_uiLastCharacter = 0x00000008;
  }

  const char* szInputSlotName = ConvertGLFWKeyToEngineName(key);
  if (szInputSlotName)
  {
    m_InputSlotValues[szInputSlotName] = (action == GLFW_RELEASE) ? 0.0f : 1.0f;
  }
  else
  {
    plLog::Warning("Unhandeled glfw keyboard key {} {}", key, (action == GLFW_RELEASE) ? "released" : "pressed");
  }
}

void plStandardInputDevice::OnCharacter(unsigned int codepoint)
{
  m_uiLastCharacter = codepoint;
}

void plStandardInputDevice::OnCursorPosition(double xpos, double ypos)
{
  s_iMouseIsOverWindowNumber = m_uiWindowNumber;

  int width;
  int height;
  glfwGetWindowSize(m_pWindow, &width, &height);

  m_InputSlotValues[plInputSlot_MousePositionX] = static_cast<float>(xpos / width);
  m_InputSlotValues[plInputSlot_MousePositionY] = static_cast<float>(ypos / height);

  if (m_LastPos.x != plMath::MaxValue<double>())
  {
    plVec2d diff = plVec2d(xpos, ypos) - m_LastPos;

    m_InputSlotValues[plInputSlot_MouseMoveNegX] += ((diff.x < 0) ? (float)-diff.x : 0.0f) * GetMouseSpeed().x;
    m_InputSlotValues[plInputSlot_MouseMovePosX] += ((diff.x > 0) ? (float)diff.x : 0.0f) * GetMouseSpeed().x;
    m_InputSlotValues[plInputSlot_MouseMoveNegY] += ((diff.y < 0) ? (float)-diff.y : 0.0f) * GetMouseSpeed().y;
    m_InputSlotValues[plInputSlot_MouseMovePosY] += ((diff.y > 0) ? (float)diff.y : 0.0f) * GetMouseSpeed().y;
  }
  m_LastPos = plVec2d(xpos, ypos);
}

void plStandardInputDevice::OnMouseButton(int button, int action, int mods)
{
  const char* inputSlot = nullptr;
  switch (button)
  {
    case GLFW_MOUSE_BUTTON_1:
      inputSlot = plInputSlot_MouseButton0;
      break;
    case GLFW_MOUSE_BUTTON_2:
      inputSlot = plInputSlot_MouseButton1;
      break;
    case GLFW_MOUSE_BUTTON_3:
      inputSlot = plInputSlot_MouseButton2;
      break;
    case GLFW_MOUSE_BUTTON_4:
      inputSlot = plInputSlot_MouseButton3;
      break;
    case GLFW_MOUSE_BUTTON_5:
      inputSlot = plInputSlot_MouseButton4;
      break;
  }

  if (inputSlot)
  {
    m_InputSlotValues[inputSlot] = (action == GLFW_PRESS) ? 1.0f : 0.0f;
  }
}

void plStandardInputDevice::OnScroll(double xoffset, double yoffset)
{
  if (yoffset > 0)
  {
    m_InputSlotValues[plInputSlot_MouseWheelUp] = static_cast<float>(yoffset);
  }
  else
  {
    m_InputSlotValues[plInputSlot_MouseWheelDown] = static_cast<float>(-yoffset);
  }
}

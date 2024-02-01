#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

plStringView plInputManager::ConvertScanCodeToEngineName(plUInt8 uiScanCode, bool bIsExtendedKey)
{
  const plUInt8 uiFinalScanCode = bIsExtendedKey ? (uiScanCode + 128) : uiScanCode;

  switch (uiFinalScanCode)
  {
    case 1:
      return plInputSlot_KeyEscape;
    case 2:
      return plInputSlot_Key1;
    case 3:
      return plInputSlot_Key2;
    case 4:
      return plInputSlot_Key3;
    case 5:
      return plInputSlot_Key4;
    case 6:
      return plInputSlot_Key5;
    case 7:
      return plInputSlot_Key6;
    case 8:
      return plInputSlot_Key7;
    case 9:
      return plInputSlot_Key8;
    case 10:
      return plInputSlot_Key9;
    case 11:
      return plInputSlot_Key0;
    case 12:
      return plInputSlot_KeyHyphen;
    case 13:
      return plInputSlot_KeyEquals;
    case 14:
      return plInputSlot_KeyBackspace;
    case 15:
      return plInputSlot_KeyTab;
    case 16:
      return plInputSlot_KeyQ;
    case 17:
      return plInputSlot_KeyW;
    case 18:
      return plInputSlot_KeyE;
    case 19:
      return plInputSlot_KeyR;
    case 20:
      return plInputSlot_KeyT;
    case 21:
      return plInputSlot_KeyY;
    case 22:
      return plInputSlot_KeyU;
    case 23:
      return plInputSlot_KeyI;
    case 24:
      return plInputSlot_KeyO;
    case 25:
      return plInputSlot_KeyP;
    case 26:
      return plInputSlot_KeyBracketOpen;
    case 27:
      return plInputSlot_KeyBracketClose;
    case 28:
      return plInputSlot_KeyReturn;
    case 29:
      return plInputSlot_KeyLeftCtrl;
    case 30:
      return plInputSlot_KeyA;
    case 31:
      return plInputSlot_KeyS;
    case 32:
      return plInputSlot_KeyD;
    case 33:
      return plInputSlot_KeyF;
    case 34:
      return plInputSlot_KeyG;
    case 35:
      return plInputSlot_KeyH;
    case 36:
      return plInputSlot_KeyJ;
    case 37:
      return plInputSlot_KeyK;
    case 38:
      return plInputSlot_KeyL;
    case 39:
      return plInputSlot_KeySemicolon;
    case 40:
      return plInputSlot_KeyApostrophe;
    case 41:
      return plInputSlot_KeyTilde;
    case 42:
      return plInputSlot_KeyLeftShift;
    case 43:
      return plInputSlot_KeyBackslash;
    case 44:
      return plInputSlot_KeyZ;
    case 45:
      return plInputSlot_KeyX;
    case 46:
      return plInputSlot_KeyC;
    case 47:
      return plInputSlot_KeyV;
    case 48:
      return plInputSlot_KeyB;
    case 49:
      return plInputSlot_KeyN;
    case 50:
      return plInputSlot_KeyM;
    case 51:
      return plInputSlot_KeyComma;
    case 52:
      return plInputSlot_KeyPeriod;
    case 53:
      return plInputSlot_KeySlash;
    case 54:
      return plInputSlot_KeyRightShift;
    case 55:
      return plInputSlot_KeyNumpadStar;
    case 56:
      return plInputSlot_KeyLeftAlt;
    case 57:
      return plInputSlot_KeySpace;
    case 58:
      return plInputSlot_KeyCapsLock;
    case 59:
      return plInputSlot_KeyF1;
    case 60:
      return plInputSlot_KeyF2;
    case 61:
      return plInputSlot_KeyF3;
    case 62:
      return plInputSlot_KeyF4;
    case 63:
      return plInputSlot_KeyF5;
    case 64:
      return plInputSlot_KeyF6;
    case 65:
      return plInputSlot_KeyF7;
    case 66:
      return plInputSlot_KeyF8;
    case 67:
      return plInputSlot_KeyF9;
    case 68:
      return plInputSlot_KeyF10;
    case 69:
      return plInputSlot_KeyNumLock;
    case 70:
      return plInputSlot_KeyScroll;
    case 71:
      return plInputSlot_KeyNumpad7;
    case 72:
      return plInputSlot_KeyNumpad8;
    case 73:
      return plInputSlot_KeyNumpad9;
    case 74:
      return plInputSlot_KeyNumpadMinus;
    case 75:
      return plInputSlot_KeyNumpad4;
    case 76:
      return plInputSlot_KeyNumpad5;
    case 77:
      return plInputSlot_KeyNumpad6;
    case 78:
      return plInputSlot_KeyNumpadPlus;
    case 79:
      return plInputSlot_KeyNumpad1;
    case 80:
      return plInputSlot_KeyNumpad2;
    case 81:
      return plInputSlot_KeyNumpad3;
    case 82:
      return plInputSlot_KeyNumpad0;
    case 83:
      return plInputSlot_KeyNumpadPeriod;


    case 86:
      return plInputSlot_KeyPipe;
    case 87:
      return plInputSlot_KeyF11;
    case 88:
      return plInputSlot_KeyF12;


    case 91:
      return plInputSlot_KeyLeftWin;
    case 92:
      return plInputSlot_KeyRightWin;
    case 93:
      return plInputSlot_KeyApps;



    case 128 + 16:
      return plInputSlot_KeyPrevTrack;
    case 128 + 25:
      return plInputSlot_KeyNextTrack;
    case 128 + 28:
      return plInputSlot_KeyNumpadEnter;
    case 128 + 29:
      return plInputSlot_KeyRightCtrl;
    case 128 + 32:
      return plInputSlot_KeyMute;
    case 128 + 34:
      return plInputSlot_KeyPlayPause;
    case 128 + 36:
      return plInputSlot_KeyStop;
    case 128 + 46:
      return plInputSlot_KeyVolumeDown;
    case 128 + 48:
      return plInputSlot_KeyVolumeUp;
    case 128 + 53:
      return plInputSlot_KeyNumpadSlash;
    case 128 + 55:
      return plInputSlot_KeyPrint;
    case 128 + 56:
      return plInputSlot_KeyRightAlt;
    case 128 + 70:
      return plInputSlot_KeyPause;
    case 128 + 71:
      return plInputSlot_KeyHome;
    case 128 + 72:
      return plInputSlot_KeyUp;
    case 128 + 73:
      return plInputSlot_KeyPageUp;
    case 128 + 75:
      return plInputSlot_KeyLeft;
    case 128 + 77:
      return plInputSlot_KeyRight;
    case 128 + 79:
      return plInputSlot_KeyEnd;
    case 128 + 80:
      return plInputSlot_KeyDown;
    case 128 + 81:
      return plInputSlot_KeyPageDown;
    case 128 + 82:
      return plInputSlot_KeyInsert;
    case 128 + 83:
      return plInputSlot_KeyDelete;

    default:

      // for extended keys fall back to the non-extended name
      if (bIsExtendedKey)
        return ConvertScanCodeToEngineName(uiScanCode, false);

      break;
  }

  return "unknown_key";
}



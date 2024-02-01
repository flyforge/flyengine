#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Foundation/Basics/Platform/Win/MinWindows.h>

class PL_CORE_DLL plStandardInputDevice : public plInputDeviceMouseKeyboard
{
  PL_ADD_DYNAMIC_REFLECTION(plStandardInputDevice, plInputDeviceMouseKeyboard);

public:
  plStandardInputDevice(plUInt32 uiWindowNumber);
  ~plStandardInputDevice();

  /// \brief This function needs to be called by all Windows functions, to pass the input information through to this input device.
  void WindowMessage(plMinWindows::HWND hWnd, plMinWindows::UINT msg, plMinWindows::WPARAM wparam, plMinWindows::LPARAM lparam);

  /// \brief Calling this function will 'translate' most key names from English to the OS language, by querying that information
  /// from the OS.
  ///
  /// The OS translation might not always be perfect for all keys. The translation can change when the user changes the keyboard layout.
  /// So if he switches from an English layout to a German layout, LocalizeButtonDisplayNames() should be called again, to update
  /// the display names, if that is required.
  static void LocalizeButtonDisplayNames();

  virtual void SetClipMouseCursor(plMouseCursorClipMode::Enum mode) override;
  virtual plMouseCursorClipMode::Enum GetClipMouseCursor() const override { return m_ClipCursorMode; }

  virtual void SetShowMouseCursor(bool bShow) override;
  virtual bool GetShowMouseCursor() const override;

protected:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;
  virtual void UpdateInputSlotValues() override;

private:
  void ApplyClipRect(plMouseCursorClipMode::Enum mode, plMinWindows::HWND hWnd);
  void OnFocusLost(plMinWindows::HWND hWnd);

  static bool s_bMainWindowUsed;
  plUInt32 m_uiWindowNumber = 0;
  bool m_bShowCursor = true;
  plMouseCursorClipMode::Enum m_ClipCursorMode = plMouseCursorClipMode::NoClip;
  bool m_bApplyClipRect = false;
  plUInt8 m_uiMouseButtonReceivedDown[5] = {0, 0, 0, 0, 0};
  plUInt8 m_uiMouseButtonReceivedUp[5] = {0, 0, 0, 0, 0};
};

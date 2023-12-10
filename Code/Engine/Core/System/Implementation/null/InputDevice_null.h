#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>

class PLASMA_CORE_DLL plStandardInputDevice : public plInputDeviceMouseKeyboard
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStandardInputDevice, plInputDeviceMouseKeyboard);

public:
  plStandardInputDevice(plUInt32 uiWindowNumber);
  ~plStandardInputDevice();

  virtual void SetShowMouseCursor(bool bShow) override;
  virtual void SetClipMouseCursor(plMouseCursorClipMode::Enum mode) override;
  virtual plMouseCursorClipMode::Enum GetClipMouseCursor() const override;

private:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
};

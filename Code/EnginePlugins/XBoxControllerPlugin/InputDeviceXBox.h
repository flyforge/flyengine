#pragma once

#include <Core/Input/DeviceTypes/Controller.h>
#include <XBoxControllerPlugin/XBoxControllerDLL.h>

/// \brief An implementation of plInputDeviceController that handles XBox 360 controllers.
///
/// Works on all platforms that provide the XINPUT API.
class PL_XBOXCONTROLLER_DLL plInputDeviceXBox360 : public plInputDeviceController
{
  PL_ADD_DYNAMIC_REFLECTION(plInputDeviceXBox360, plInputDeviceController);

public:
  plInputDeviceXBox360();
  ~plInputDeviceXBox360();

  /// \brief Returns an plInputDeviceXBox360 device.
  static plInputDeviceXBox360* GetDevice();

  /// \brief Destroys all devices of this type. Automatically called at engine shutdown.
  static void DestroyAllDevices();

  virtual bool IsControllerConnected(plUInt8 uiPhysical) const override;

private:
  virtual void ApplyVibration(plUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength) override;

  bool m_bControllerConnected[4];

  virtual void InitializeDevice() override {}
  virtual void UpdateInputSlotValues() override;
  virtual void RegisterInputSlots() override;
  virtual void UpdateHardwareState(plTime tTimeDifference) override;

  void SetValue(plInt32 iController, const char* szButton, float fValue);

  static void RegisterControllerButton(const char* szButton, const char* szName, plBitflags<plInputSlotFlags> SlotFlags);
  static void SetDeadZone(const char* szButton);
};

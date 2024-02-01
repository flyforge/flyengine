#pragma once

#include <Core/CoreDLL.h>

class plInputDeviceController;

class PL_CORE_DLL plControllerInput
{
public:
  // \brief Returns if a global controller input device exists.
  static bool HasDevice();

  // \brief Returns the global controller input device. May be nullptr.
  static plInputDeviceController* GetDevice();

  // \brief Set the global controller input device.
  static void SetDevice(plInputDeviceController* pDevice);
};

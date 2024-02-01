#include <Core/CorePCH.h>

#include <Core/System/ControllerInput.h>
#include <Foundation/System/PlatformFeatures.h>

namespace
{
  plInputDeviceController* g_pInputDeviceController = nullptr;
}

bool plControllerInput::HasDevice()
{
  return g_pInputDeviceController != nullptr;
}

plInputDeviceController* plControllerInput::GetDevice()
{
  return g_pInputDeviceController;
}

void plControllerInput::SetDevice(plInputDeviceController* pDevice)
{
  g_pInputDeviceController = pDevice;
}

#if PL_ENABLED(PL_SUPPORTS_GLFW)
#  include <Core/System/Implementation/glfw/ControllerInput_glfw.inl>
#endif



#include <Core/System/ControllerInput.h>
#include <Foundation/Configuration/Startup.h>
#include <XBoxControllerPlugin/InputDeviceXBox.h>

static plInputDeviceXBox360* g_InputDeviceXBox360 = nullptr;

plInputDeviceXBox360* plInputDeviceXBox360::GetDevice()
{
  if (g_InputDeviceXBox360 == nullptr)
    g_InputDeviceXBox360 = PL_DEFAULT_NEW(plInputDeviceXBox360);

  return g_InputDeviceXBox360;
}

void plInputDeviceXBox360::DestroyAllDevices()
{
  PL_DEFAULT_DELETE(g_InputDeviceXBox360);
}

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(InputDevices, InputDeviceXBox360)
 
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation", 
    "InputManager"
    
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }
 
  ON_CORESYSTEMS_SHUTDOWN
  {
    plInputDeviceXBox360::DestroyAllDevices();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    plInputDeviceXBox360* pDevice = plInputDeviceXBox360::GetDevice();
    if(plControllerInput::GetDevice() == nullptr)
    {
      plControllerInput::SetDevice(pDevice);
    }
  }
 
  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if(plControllerInput::GetDevice() == g_InputDeviceXBox360)
    {
      plControllerInput::SetDevice(nullptr);
    }
    plInputDeviceXBox360::DestroyAllDevices();
  }
 
PL_END_SUBSYSTEM_DECLARATION;
// clang-format on


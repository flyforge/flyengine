#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/XRInputDevice.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plXRInputDevice, 1, plRTTINoAllocator);
// no properties or message handlers
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRInputDevice);

#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/XRInputDevice.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plXRInputDevice, 1, plRTTINoAllocator);
// no properties or message handlers
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PL_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRInputDevice);

#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/XRInterface.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plXRStageSpace, 1)
  PLASMA_BITFLAGS_CONSTANTS(plXRStageSpace::Seated, plXRStageSpace::Standing)
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRInterface);

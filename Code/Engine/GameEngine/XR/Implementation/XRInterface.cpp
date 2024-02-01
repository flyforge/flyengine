#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/XRInterface.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plXRStageSpace, 1)
  PL_BITFLAGS_CONSTANTS(plXRStageSpace::Seated, plXRStageSpace::Standing)
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

PL_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRInterface);

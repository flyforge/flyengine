#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/Startup.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Core, InputManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plInputManager::DeallocateInternals();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

PLASMA_STATICLINK_FILE(Core, Core_Input_Implementation_Startup);

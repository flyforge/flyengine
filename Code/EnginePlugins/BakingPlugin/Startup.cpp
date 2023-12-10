#include <BakingPlugin/BakingPluginPCH.h>

#include <BakingPlugin/BakingScene.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>

static plUniquePtr<plBaking> s_Baking;

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Baking, BakingPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  { 
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    s_Baking = PLASMA_DEFAULT_NEW(plBaking);
    s_Baking->Startup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    s_Baking->Shutdown();
    s_Baking = nullptr;
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

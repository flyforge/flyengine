#include <FMODAudioPlugin/FMODAudioPluginPCH.h>

#include <FMODAudioPlugin/FMODAudioSingleton.h>

#include <AudioSystemPlugin/Core/AudioSystem.h>

#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <GameEngine/GameApplication/GameApplication.h>

static plFMOD* s_pfmodSingleton = nullptr;

PLASMA_BEGIN_SUBSYSTEM_DECLARATION(FMOD, FMODAudioPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "AudioSystem"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    s_pfmodSingleton = PLASMA_AUDIOSYSTEM_NEW(plFMOD);

    plAudioSystem::GetSingleton()->Startup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    plAudioSystem::GetSingleton()->Shutdown();

    PLASMA_AUDIOSYSTEM_DELETE(s_pfmodSingleton);
  }

PLASMA_END_SUBSYSTEM_DECLARATION;

PLASMA_STATICLINK_FILE(FMODAudioPlugin, FMODAudioPlugin_FMODAudioPluginStartup);

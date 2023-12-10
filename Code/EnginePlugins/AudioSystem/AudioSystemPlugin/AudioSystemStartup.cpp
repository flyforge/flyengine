#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>

#include <Foundation/Configuration/Startup.h>

static plAudioSystemAllocator* s_pAudioSystemAllocator = nullptr;
static plAudioMiddlewareAllocator* s_pAudioMiddlewareAllocator = nullptr;
static plAudioSystem* s_pAudioSystemSingleton = nullptr;

PLASMA_BEGIN_SUBSYSTEM_DECLARATION(AudioSystem, AudioSystemPlugin)

  // clang-format off
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES
  // clang-format on

  ON_BASESYSTEMS_STARTUP
  {
    s_pAudioSystemAllocator = PLASMA_DEFAULT_NEW(plAudioSystemAllocator);
    s_pAudioMiddlewareAllocator = PLASMA_DEFAULT_NEW(plAudioMiddlewareAllocator, s_pAudioSystemAllocator);
    s_pAudioSystemSingleton = PLASMA_DEFAULT_NEW(plAudioSystem);
  }

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    PLASMA_DEFAULT_DELETE(s_pAudioSystemSingleton);
    PLASMA_DEFAULT_DELETE(s_pAudioMiddlewareAllocator);
    PLASMA_DEFAULT_DELETE(s_pAudioSystemAllocator);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(&plAudioSystem::GameApplicationEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(&plAudioSystem::GameApplicationEventHandler);
  }

PLASMA_END_SUBSYSTEM_DECLARATION;

PLASMA_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_AudioSystemStartup);

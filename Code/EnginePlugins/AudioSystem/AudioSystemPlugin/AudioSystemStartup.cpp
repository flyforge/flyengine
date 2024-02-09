#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>

#include <Foundation/Configuration/Startup.h>

static plAudioSystemAllocator* s_pAudioSystemAllocator = nullptr;
static plAudioMiddlewareAllocator* s_pAudioMiddlewareAllocator = nullptr;
static plAudioSystem* s_pAudioSystemSingleton = nullptr;

PL_BEGIN_SUBSYSTEM_DECLARATION(AudioSystem, AudioSystemPlugin)

  // clang-format off
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES
  // clang-format on

  ON_BASESYSTEMS_STARTUP
  {
    s_pAudioSystemAllocator = PL_DEFAULT_NEW(plAudioSystemAllocator);
    s_pAudioMiddlewareAllocator = PL_DEFAULT_NEW(plAudioMiddlewareAllocator, s_pAudioSystemAllocator);
    s_pAudioSystemSingleton = PL_DEFAULT_NEW(plAudioSystem);
  }

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    PL_DEFAULT_DELETE(s_pAudioSystemSingleton);
    PL_DEFAULT_DELETE(s_pAudioMiddlewareAllocator);
    PL_DEFAULT_DELETE(s_pAudioSystemAllocator);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(&plAudioSystem::GameApplicationEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(&plAudioSystem::GameApplicationEventHandler);
  }

PL_END_SUBSYSTEM_DECLARATION;

PL_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_AudioSystemStartup);

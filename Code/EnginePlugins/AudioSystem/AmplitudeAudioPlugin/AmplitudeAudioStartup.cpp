#include <AmplitudeAudioPlugin/AmplitudeAudioPluginPCH.h>

#include <AmplitudeAudioPlugin/AmplitudeAudioSingleton.h>
#include <AmplitudeAudioPlugin/Resources/AudioControlCollectionResource.h>

#include <AudioSystemPlugin/Core/AudioSystem.h>

#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <GameEngine/GameApplication/GameApplication.h>

static plAmplitude* s_pAmplitudeSingleton = nullptr;

PL_BEGIN_SUBSYSTEM_DECLARATION(SparkyStudios, AmplitudeAudioPlugin)

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
    s_pAmplitudeSingleton = PL_AUDIOSYSTEM_NEW(plAmplitude);

    // Audio Control Collection Resources
    {
      plResourceManager::RegisterResourceForAssetType("Audio Control Collection", plGetStaticRTTI<plAmplitudeAudioControlCollectionResource>());

      plAmplitudeAudioControlCollectionResourceDescriptor desc;
      const auto hResource = plResourceManager::CreateResource<plAmplitudeAudioControlCollectionResource>("AudioControlCollectionMissing", std::move(desc), "Fallback for missing audio control collections.");
      plResourceManager::SetResourceTypeMissingFallback<plAmplitudeAudioControlCollectionResource>(hResource);
    }

    plAudioSystem::GetSingleton()->Startup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    plAudioSystem::GetSingleton()->Shutdown();

    plAmplitudeAudioControlCollectionResource::CleanupDynamicPluginReferences();

    PL_AUDIOSYSTEM_DELETE(s_pAmplitudeSingleton);
  }

PL_END_SUBSYSTEM_DECLARATION;

#if defined(AM_WINDOWS_VERSION)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

PL_STATICLINK_FILE(AmplitudeAudioPlugin, AmplitudeAudioPlugin_AmplitudeAudioPluginStartup);

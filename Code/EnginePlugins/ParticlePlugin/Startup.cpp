#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Particle, ParticlePlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plResourceManager::RegisterResourceForAssetType("Particle Effect", plGetStaticRTTI<plParticleEffectResource>());

    plParticleEffectResourceDescriptor desc;
    plParticleEffectResourceHandle hEffect = plResourceManager::CreateResource<plParticleEffectResource>("ParticleEffectMissing", std::move(desc), "Fallback for missing Particle Effects");
    plResourceManager::SetResourceTypeMissingFallback<plParticleEffectResource>(hEffect);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plParticleEffectResource::CleanupDynamicPluginReferences();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on


PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Startup);

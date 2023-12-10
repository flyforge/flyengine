#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(ProcGen, ProcGenPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plResourceManager::RegisterResourceForAssetType("ProcGen Graph", plGetStaticRTTI<plProcGenGraphResource>());

    plProcGenGraphResourceDescriptor desc;
    plProcGenGraphResourceHandle hResource = plResourceManager::CreateResource<plProcGenGraphResource>("ProcGenGraphMissing", std::move(desc), "Fallback for missing ProcGen Graph Resource");
    plResourceManager::SetResourceTypeMissingFallback<plProcGenGraphResource>(hResource);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plProcGenGraphResource::CleanupDynamicPluginReferences();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

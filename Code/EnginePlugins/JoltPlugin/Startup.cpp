#include <JoltPlugin/JoltPluginPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/System/JoltCore.h>

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(Jolt, JoltPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plResourceManager::RegisterResourceForAssetType("Jolt_Colmesh_Triangle", plGetStaticRTTI<plJoltMeshResource>());
    plResourceManager::RegisterResourceForAssetType("Jolt_Colmesh_Convex", plGetStaticRTTI<plJoltMeshResource>());

    plJoltMeshResourceDescriptor desc;
    plJoltMeshResourceHandle hResource = plResourceManager::CreateResource<plJoltMeshResource>("Missing Jolt Mesh", std::move(desc), "Empty collision mesh");
    plResourceManager::SetResourceTypeMissingFallback<plJoltMeshResource>(hResource);

    plJoltCore::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plResourceManager::SetResourceTypeMissingFallback<plJoltMeshResource>(plJoltMeshResourceHandle());
    plJoltCore::Shutdown();

    plJoltMeshResource::CleanupDynamicPluginReferences();
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on


PL_STATICLINK_FILE(JoltPlugin, JoltPlugin_Startup);


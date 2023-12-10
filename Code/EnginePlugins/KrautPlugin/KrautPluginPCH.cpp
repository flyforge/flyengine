#include <KrautPlugin/KrautPluginPCH.h>

#include <KrautPlugin/KrautDeclarations.h>

#include <Foundation/Configuration/Startup.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Kraut, KrautPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plResourceManager::RegisterResourceForAssetType("Kraut Tree", plGetStaticRTTI<plKrautGeneratorResource>());

    plResourceManager::AllowResourceTypeAcquireDuringUpdateContent<plKrautTreeResource, plMaterialResource>();
    plResourceManager::AllowResourceTypeAcquireDuringUpdateContent<plKrautTreeResource, plMeshResource>();

    {
      plKrautTreeResourceDescriptor desc;
      desc.m_Details.m_Bounds = plBoundingBoxSphere::MakeInvalid();

        plKrautTreeResourceHandle hResource = plResourceManager::CreateResource<plKrautTreeResource>("Missing Kraut Tree Mesh", std::move(desc), "Empty Kraut Tree Mesh");
      plResourceManager::SetResourceTypeMissingFallback<plKrautTreeResource>(hResource);
    }

    //{
    //  plKrautGeneratorResourceHandle hResource = plResourceManager::LoadResource<plKrautGeneratorResource>("Kraut/KrautFallback.tree");
    //  plResourceManager::SetResourceTypeMissingFallback<plKrautGeneratorResource>(hResource);
    //}
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plResourceManager::SetResourceTypeMissingFallback<plKrautTreeResource>(plKrautTreeResourceHandle());
    plResourceManager::SetResourceTypeMissingFallback<plKrautGeneratorResource>(plKrautGeneratorResourceHandle());

    plKrautTreeResource::CleanupDynamicPluginReferences();
    plKrautGeneratorResource::CleanupDynamicPluginReferences();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

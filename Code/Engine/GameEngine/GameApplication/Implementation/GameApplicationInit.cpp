#include <GameEngine/GameEnginePCH.h>

#include <Core/Collection/CollectionResource.h>
#include <Core/Curves/ColorGradientResource.h>
#include <Core/Curves/Curve1DResource.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Decals/DecalResource.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/DeviceFactory.h>

#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
constexpr const char* szDefaultRenderer = "Vulkan";
#else
constexpr const char* szDefaultRenderer = "DX11";
#endif

plCommandLineOptionString opt_Renderer("app", "-renderer", "The renderer implementation to use.", szDefaultRenderer);

void plGameApplication::Init_ConfigureAssetManagement()
{
  const plStringBuilder sAssetRedirFile("AssetCache/", m_PlatformProfile.m_sName, ".plAidlt");

  // which redirection table to search
  plDataDirectory::FolderType::s_sRedirectionFile = sAssetRedirFile;

  // which platform assets to use
  plDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/";

  plResourceManager::RegisterResourceForAssetType("Collection", plGetStaticRTTI<plCollectionResource>());
  plResourceManager::RegisterResourceForAssetType("Material", plGetStaticRTTI<plMaterialResource>());
  plResourceManager::RegisterResourceForAssetType("Mesh", plGetStaticRTTI<plMeshResource>());
  plResourceManager::RegisterResourceForAssetType("Animated Mesh", plGetStaticRTTI<plMeshResource>());
  plResourceManager::RegisterResourceForAssetType("Prefab", plGetStaticRTTI<plPrefabResource>());
  plResourceManager::RegisterResourceForAssetType("RenderPipeline", plGetStaticRTTI<plRenderPipelineResource>());
  plResourceManager::RegisterResourceForAssetType("Surface", plGetStaticRTTI<plSurfaceResource>());
  plResourceManager::RegisterResourceForAssetType("Texture 2D", plGetStaticRTTI<plTexture2DResource>());
  plResourceManager::RegisterResourceForAssetType("Render Target", plGetStaticRTTI<plTexture2DResource>());
  plResourceManager::RegisterResourceForAssetType("Texture Cube", plGetStaticRTTI<plTextureCubeResource>());
  plResourceManager::RegisterResourceForAssetType("Color Gradient", plGetStaticRTTI<plColorGradientResource>());
  plResourceManager::RegisterResourceForAssetType("Curve1D", plGetStaticRTTI<plCurve1DResource>());
  plResourceManager::RegisterResourceForAssetType("Skeleton", plGetStaticRTTI<plSkeletonResource>());
  plResourceManager::RegisterResourceForAssetType("Animation Clip", plGetStaticRTTI<plAnimationClipResource>());
  plResourceManager::RegisterResourceForAssetType("Animation Graph", plGetStaticRTTI<plAnimGraphResource>());
  plResourceManager::RegisterResourceForAssetType("Image Data", plGetStaticRTTI<plImageDataResource>());
  plResourceManager::RegisterResourceForAssetType("PropertyAnim", plGetStaticRTTI<plPropertyAnimResource>());
  plResourceManager::RegisterResourceForAssetType("Decal", plGetStaticRTTI<plDecalResource>());
  plResourceManager::RegisterResourceForAssetType("LUT", plGetStaticRTTI<plTexture3DResource>());
  plResourceManager::RegisterResourceForAssetType("BlackboardTemplate", plGetStaticRTTI<plBlackboardTemplateResource>());
  plResourceManager::RegisterResourceForAssetType("Substance Texture", plGetStaticRTTI<plTexture2DResource>());
}

void plGameApplication::Init_SetupDefaultResources()
{
  SUPER::Init_SetupDefaultResources();

  plResourceManager::SetIncrementalUnloadForResourceType<plShaderPermutationResource>(false);

  // Shaders
  {
    plShaderResourceDescriptor desc;
    plShaderResourceHandle hFallbackShader = plResourceManager::CreateResource<plShaderResource>("FallbackShaderResource", std::move(desc), "FallbackShaderResource");
    plShaderResourceHandle hMissingShader = plResourceManager::CreateResource<plShaderResource>("MissingShaderResource", std::move(desc), "MissingShaderResource");

    plResourceManager::SetResourceTypeLoadingFallback<plShaderResource>(hFallbackShader);
    plResourceManager::SetResourceTypeMissingFallback<plShaderResource>(hMissingShader);
  }

  // Shader Permutation
  {
    plShaderPermutationResourceDescriptor desc;
    plShaderPermutationResourceHandle hFallbackShaderPermutation = plResourceManager::CreateResource<plShaderPermutationResource>("FallbackShaderPermutationResource", std::move(desc), "FallbackShaderPermutationResource");

    plResourceManager::SetResourceTypeLoadingFallback<plShaderPermutationResource>(hFallbackShaderPermutation);
  }

  // 2D Textures
  {
    plTexture2DResourceHandle hFallbackTexture = plResourceManager::LoadResource<plTexture2DResource>("Textures/LoadingTexture_D.dds");
    plTexture2DResourceHandle hMissingTexture = plResourceManager::LoadResource<plTexture2DResource>("Textures/MissingTexture_D.dds");

    plResourceManager::SetResourceTypeLoadingFallback<plTexture2DResource>(hFallbackTexture);
    plResourceManager::SetResourceTypeMissingFallback<plTexture2DResource>(hMissingTexture);
  }

  // Render to 2D Textures
  {
    plRenderToTexture2DResourceDescriptor desc;
    desc.m_uiWidth = 128;
    desc.m_uiHeight = 128;

    plRenderToTexture2DResourceHandle hMissingTexture = plResourceManager::CreateResource<plRenderToTexture2DResource>("R22DT_Missing", std::move(desc));

    plResourceManager::SetResourceTypeMissingFallback<plRenderToTexture2DResource>(hMissingTexture);
  }

  // Cube Textures
  {
    /// \todo Loading Cubemap Texture

    plTextureCubeResourceHandle hFallbackTexture = plResourceManager::LoadResource<plTextureCubeResource>("Textures/MissingCubeMap.dds");
    plTextureCubeResourceHandle hMissingTexture = plResourceManager::LoadResource<plTextureCubeResource>("Textures/MissingCubeMap.dds");

    plResourceManager::SetResourceTypeLoadingFallback<plTextureCubeResource>(hFallbackTexture);
    plResourceManager::SetResourceTypeMissingFallback<plTextureCubeResource>(hMissingTexture);
  }

  // Materials
  {
    plResourceManager::AllowResourceTypeAcquireDuringUpdateContent<plMaterialResource, plMaterialResource>();

    plMaterialResourceHandle hMissingMaterial = plResourceManager::LoadResource<plMaterialResource>("Materials/Common/MissingMaterial.plMaterial");
    plMaterialResourceHandle hFallbackMaterial = plResourceManager::LoadResource<plMaterialResource>("Materials/Common/LoadingMaterial.plMaterial");

    plResourceManager::SetResourceTypeLoadingFallback<plMaterialResource>(hFallbackMaterial);
    plResourceManager::SetResourceTypeMissingFallback<plMaterialResource>(hMissingMaterial);
  }

  // Meshes
  {
    plResourceManager::AllowResourceTypeAcquireDuringUpdateContent<plMeshResource, plMeshBufferResource>();

    plMeshResourceHandle hMissingMesh = plResourceManager::LoadResource<plMeshResource>("Meshes/MissingMesh.plMesh");
    plResourceManager::SetResourceTypeMissingFallback<plMeshResource>(hMissingMesh);
  }

  // Prefabs
  {
    // plPrefabResourceDescriptor emptyPrefab;
    // plPrefabResourceHandle hMissingPrefab = plResourceManager::CreateResource<plPrefabResource>("MissingPrefabResource", emptyPrefab,
    // "MissingPrefabResource");

    plPrefabResourceHandle hMissingPrefab = plResourceManager::LoadResource<plPrefabResource>("Prefabs/MissingPrefab.plObjectGraph");
    plResourceManager::SetResourceTypeMissingFallback<plPrefabResource>(hMissingPrefab);
  }

  // Collections
  {
    plCollectionResourceDescriptor desc;
    plCollectionResourceHandle hMissingCollection = plResourceManager::CreateResource<plCollectionResource>("MissingCollectionResource", std::move(desc), "MissingCollectionResource");

    plResourceManager::SetResourceTypeMissingFallback<plCollectionResource>(hMissingCollection);
  }

  // Render Pipelines
  {
    plRenderPipelineResourceHandle hMissingRenderPipeline = plRenderPipelineResource::CreateMissingPipeline();
    plResourceManager::SetResourceTypeMissingFallback<plRenderPipelineResource>(hMissingRenderPipeline);
  }

  // Color Gradient
  {
    plColorGradientResourceDescriptor cg;
    cg.m_Gradient.AddColorControlPoint(0, plColor::RebeccaPurple);
    cg.m_Gradient.AddColorControlPoint(1, plColor::LawnGreen);
    cg.m_Gradient.SortControlPoints();

    plColorGradientResourceHandle hResource = plResourceManager::CreateResource<plColorGradientResource>("MissingColorGradient", std::move(cg), "Missing Color Gradient Resource");
    plResourceManager::SetResourceTypeMissingFallback<plColorGradientResource>(hResource);
  }

  // 1D Curve
  {
    plCurve1DResourceDescriptor cd;
    auto& curve = cd.m_Curves.ExpandAndGetRef();
    curve.AddControlPoint(0);
    curve.AddControlPoint(1);
    curve.CreateLinearApproximation();

    plCurve1DResourceHandle hResource = plResourceManager::CreateResource<plCurve1DResource>("MissingCurve1D", std::move(cd), "Missing Curve1D Resource");
    plResourceManager::SetResourceTypeMissingFallback<plCurve1DResource>(hResource);
  }
  
  // Property Animations
  {
    plPropertyAnimResourceDescriptor desc;
    desc.m_AnimationDuration = plTime::Seconds(0.1);

    plPropertyAnimResourceHandle hResource = plResourceManager::CreateResource<plPropertyAnimResource>("MissingPropertyAnim", std::move(desc), "Missing Property Animation Resource");
    plResourceManager::SetResourceTypeMissingFallback<plPropertyAnimResource>(hResource);
  }

  // Animation Skeleton
  {
    plSkeletonResourceDescriptor desc;

    plSkeletonResourceHandle hResource = plResourceManager::CreateResource<plSkeletonResource>("MissingSkeleton", std::move(desc), "Missing Skeleton Resource");
    plResourceManager::SetResourceTypeMissingFallback<plSkeletonResource>(hResource);
  }

  // Animation Clip
  {
    plAnimationClipResourceDescriptor desc;

    plAnimationClipResourceHandle hResource = plResourceManager::CreateResource<plAnimationClipResource>("MissingAnimationClip", std::move(desc), "Missing Animation Clip Resource");
    plResourceManager::SetResourceTypeMissingFallback<plAnimationClipResource>(hResource);
  }

  // Decal Atlas
  {
    plResourceManager::AllowResourceTypeAcquireDuringUpdateContent<plDecalAtlasResource, plTexture2DResource>();
  }
}

plStringView GetRendererNameFromCommandLine()
{
  return opt_Renderer.GetOptionValue(plCommandLineOption::LogMode::FirstTimeIfSpecified);
}

plStringView plGameApplication::GetActiveRenderer()
{
  return GetRendererNameFromCommandLine();
}

void plGameApplication::Init_SetupGraphicsDevice()
{
  plGALDeviceCreationDescription DeviceInit;

//#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  DeviceInit.m_bDebugDevice = true;
//#endif

  {
    plGALDevice* pDevice = nullptr;

    if (s_DefaultDeviceCreator.IsValid())
    {
      pDevice = s_DefaultDeviceCreator(DeviceInit);
    }
    else
    {
      plStringView sRendererName = GetRendererNameFromCommandLine();
      pDevice = plGALDeviceFactory::CreateDevice(sRendererName, plFoundation::GetDefaultAllocator(), DeviceInit);
      PLASMA_ASSERT_DEV(pDevice != nullptr, "Device implemention for '{}' not found", sRendererName);
    }

    PLASMA_VERIFY(pDevice->Init() == PLASMA_SUCCESS, "Graphics device creation failed!");
    plGALDevice::SetDefaultDevice(pDevice);
  }

  // Create GPU resource pool
  plGPUResourcePool* pResourcePool = PLASMA_DEFAULT_NEW(plGPUResourcePool);
  plGPUResourcePool::SetDefaultInstance(pResourcePool);
}

void plGameApplication::Init_LoadRequiredPlugins()
{
  plPlugin::InitializeStaticallyLinkedPlugins();

  plStringView sRendererName = GetRendererNameFromCommandLine();
  const char* szShaderModel = "";
  const char* szShaderCompiler = "";
  plGALDeviceFactory::GetShaderModelAndCompiler(sRendererName, szShaderModel, szShaderCompiler);
  plShaderManager::Configure(szShaderModel, true);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  plPlugin::LoadPlugin("plasmaInspectorPlugin").IgnoreResult();

  // on sandboxed platforms, we can only load data through fileserve, so enforce use of this plugin
#  if PLASMA_DISABLED(PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  plPlugin::LoadPlugin("plasmaFileservePlugin").IgnoreResult(); // don't care if it fails to load
#  endif

#endif

  PLASMA_VERIFY(plPlugin::LoadPlugin(szShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", szShaderCompiler);
}

void plGameApplication::Deinit_ShutdownGraphicsDevice()
{
  if (!plGALDevice::HasDefaultDevice())
    return;

  // Cleanup resource pool
  plGPUResourcePool::SetDefaultInstance(nullptr);

  plResourceManager::FreeAllUnusedResources();

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  pDevice->Shutdown().IgnoreResult();
  PLASMA_DEFAULT_DELETE(pDevice);
  plGALDevice::SetDefaultDevice(nullptr);
}


PLASMA_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_GameApplicationInit);

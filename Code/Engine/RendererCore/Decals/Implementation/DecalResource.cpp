#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererCore/Decals/DecalResource.h>

static plDecalResourceLoader s_DecalResourceLoader;

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DecalResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core",
  "TextureResource"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plResourceManager::SetResourceTypeLoader<plDecalResource>(&s_DecalResourceLoader);

    plDecalResourceDescriptor desc;
    plDecalResourceHandle hFallback = plResourceManager::CreateResource<plDecalResource>("Fallback Decal", std::move(desc), "Empty Decal for loading and missing decals");

    plResourceManager::SetResourceTypeLoadingFallback<plDecalResource>(hFallback);
    plResourceManager::SetResourceTypeMissingFallback<plDecalResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plResourceManager::SetResourceTypeLoader<plDecalResource>(nullptr);

    plResourceManager::SetResourceTypeLoadingFallback<plDecalResource>(plDecalResourceHandle());
    plResourceManager::SetResourceTypeMissingFallback<plDecalResource>(plDecalResourceHandle());
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDecalResource, 1, plRTTIDefaultAllocator<plDecalResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plDecalResource);
// clang-format on

plDecalResource::plDecalResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

plResourceLoadDesc plDecalResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

plResourceLoadDesc plDecalResource::UpdateContent(plStreamReader* Stream)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}

void plDecalResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plDecalResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

PL_RESOURCE_IMPLEMENT_CREATEABLE(plDecalResource, plDecalResourceDescriptor)
{
  plResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;
  ret.m_State = plResourceState::Loaded;

  return ret;
}

//////////////////////////////////////////////////////////////////////////

plResourceLoadData plDecalResourceLoader::OpenDataStream(const plResource* pResource)
{
  // nothing to load, decals are solely identified by their id (name)
  // the rest of the information is in the decal atlas resource

  plResourceLoadData res;
  return res;
}

void plDecalResourceLoader::CloseDataStream(const plResource* pResource, const plResourceLoadData& loaderData)
{
  // nothing to do
}

bool plDecalResourceLoader::IsResourceOutdated(const plResource* pResource) const
{
  // decals are never outdated
  return false;
}



PL_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalResource);

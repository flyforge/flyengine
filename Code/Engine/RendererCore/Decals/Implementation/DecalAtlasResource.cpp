#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DecalAtlasResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core",
  "TextureResource"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plDecalAtlasResourceDescriptor desc;
    plDecalAtlasResourceHandle hFallback = plResourceManager::CreateResource<plDecalAtlasResource>("Fallback Decal Atlas", std::move(desc), "Empty Decal Atlas for loading and missing decals");

    plResourceManager::SetResourceTypeLoadingFallback<plDecalAtlasResource>(hFallback);
    plResourceManager::SetResourceTypeMissingFallback<plDecalAtlasResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plResourceManager::SetResourceTypeLoadingFallback<plDecalAtlasResource>(plDecalAtlasResourceHandle());
    plResourceManager::SetResourceTypeMissingFallback<plDecalAtlasResource>(plDecalAtlasResourceHandle());
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
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDecalAtlasResource, 1, plRTTIDefaultAllocator<plDecalAtlasResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plDecalAtlasResource);
// clang-format on

plUInt32 plDecalAtlasResource::s_uiDecalAtlasResources = 0;

plDecalAtlasResource::plDecalAtlasResource()
  : plResource(DoUpdate::OnAnyThread, 1)
  , m_vBaseColorSize(plVec2U32::MakeZero())
  , m_vNormalSize(plVec2U32::MakeZero())
{
}

plDecalAtlasResourceHandle plDecalAtlasResource::GetDecalAtlasResource()
{
  return plResourceManager::LoadResource<plDecalAtlasResource>("{ ProjectDecalAtlas }");
}

plResourceLoadDesc plDecalAtlasResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

plResourceLoadDesc plDecalAtlasResource::UpdateContent(plStreamReader* Stream)
{
  PL_LOG_BLOCK("plDecalAtlasResource::UpdateContent", GetResourceIdOrDescription());

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::LoadedResourceMissing;

  if (Stream == nullptr)
    return res;

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    plStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset header
  {
    plAssetFileHeader header;
    header.Read(*Stream).IgnoreResult();
  }

  {
    plUInt8 uiVersion = 0;
    *Stream >> uiVersion;
    PL_ASSERT_DEV(uiVersion <= 3, "Invalid decal atlas version {0}", uiVersion);

    // this version is now incompatible
    if (uiVersion < 3)
      return res;
  }

  // read the textures
  {
    plDdsFileFormat dds;
    plImage baseColor, normal, orm;

    if (dds.ReadImage(*Stream, baseColor, "dds").Failed())
    {
      plLog::Error("Failed to load baseColor image for decal atlas");
      return res;
    }

    if (dds.ReadImage(*Stream, normal, "dds").Failed())
    {
      plLog::Error("Failed to load normal image for decal atlas");
      return res;
    }

    if (dds.ReadImage(*Stream, orm, "dds").Failed())
    {
      plLog::Error("Failed to load normal image for decal atlas");
      return res;
    }

    CreateLayerTexture(baseColor, true, m_hBaseColor);
    CreateLayerTexture(normal, false, m_hNormal);
    CreateLayerTexture(orm, false, m_hORM);

    m_vBaseColorSize = plVec2U32(baseColor.GetWidth(), baseColor.GetHeight());
    m_vNormalSize = plVec2U32(normal.GetWidth(), normal.GetHeight());
    m_vORMSize = plVec2U32(orm.GetWidth(), orm.GetHeight());
  }

  ReadDecalInfo(Stream);

  res.m_State = plResourceState::Loaded;
  return res;
}

void plDecalAtlasResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plDecalAtlasResource) + (plUInt32)m_Atlas.m_Items.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

PL_RESOURCE_IMPLEMENT_CREATEABLE(plDecalAtlasResource, plDecalAtlasResourceDescriptor)
{
  plResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;
  ret.m_State = plResourceState::Loaded;

  m_Atlas.Clear();

  return ret;
}

void plDecalAtlasResource::CreateLayerTexture(const plImage& img, bool bSRGB, plTexture2DResourceHandle& out_hTexture)
{
  plTexture2DResourceDescriptor td;
  td.m_SamplerDesc.m_AddressU = plImageAddressMode::Clamp;
  td.m_SamplerDesc.m_AddressV = plImageAddressMode::Clamp;
  td.m_SamplerDesc.m_AddressW = plImageAddressMode::Clamp;

  plUInt32 uiMemory;
  plHybridArray<plGALSystemMemoryDescription, 32> initData;
  plTexture2DResource::FillOutDescriptor(td, &img, bSRGB, img.GetNumMipLevels(), uiMemory, initData);
  plTextureUtils::ConfigureSampler(plTextureFilterSetting::HighQuality, td.m_SamplerDesc);

  plStringBuilder sTexId;
  sTexId.SetFormat("{0}_Tex{1}", GetResourceID(), s_uiDecalAtlasResources);
  ++s_uiDecalAtlasResources;

  out_hTexture = plResourceManager::CreateResource<plTexture2DResource>(sTexId, std::move(td));
}

void plDecalAtlasResource::ReadDecalInfo(plStreamReader* Stream)
{
  m_Atlas.Deserialize(*Stream).IgnoreResult();
}

void plDecalAtlasResource::ReportResourceIsMissing()
{
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  // normal during development, don't care much
  plLog::Debug("Decal Atlas Resource is missing: '{0}' ('{1}')", GetResourceID(), GetResourceDescription());
#else
  // should probably exist for shipped applications, report this
  plLog::Warning("Decal Atlas Resource is missing: '{0}' ('{1}')", GetResourceID(), GetResourceDescription());
#endif
}



PL_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalAtlasResource);

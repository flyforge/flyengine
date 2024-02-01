#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plRmlUiScaleMode, 1)
  PL_ENUM_CONSTANTS(plRmlUiScaleMode::Fixed, plRmlUiScaleMode::WithScreenSize)
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

static plTypeVersion s_RmlUiDescVersion = 1;

plResult plRmlUiResourceDescriptor::Save(plStreamWriter& inout_stream)
{
  // write this at the beginning so that the file can be read as an plDependencyFile
  m_DependencyFile.StoreCurrentTimeStamp();
  PL_SUCCEED_OR_RETURN(m_DependencyFile.WriteDependencyFile(inout_stream));

  inout_stream.WriteVersion(s_RmlUiDescVersion);

  inout_stream << m_sRmlFile;
  inout_stream << m_ScaleMode;
  inout_stream << m_ReferenceResolution;

  return PL_SUCCESS;
}

plResult plRmlUiResourceDescriptor::Load(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(m_DependencyFile.ReadDependencyFile(inout_stream));

  plTypeVersion uiVersion = inout_stream.ReadVersion(s_RmlUiDescVersion);
  PL_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_sRmlFile;
  inout_stream >> m_ScaleMode;
  inout_stream >> m_ReferenceResolution;

  return PL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiResource, 1, plRTTIDefaultAllocator<plRmlUiResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plRmlUiResource);
// clang-format on

plRmlUiResource::plRmlUiResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

plResourceLoadDesc plRmlUiResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

plResourceLoadDesc plRmlUiResource::UpdateContent(plStreamReader* Stream)
{
  plRmlUiResourceDescriptor desc;
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  plStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // Direct loading of rml file
  if (sAbsFilePath.GetFileExtension() == "rml")
  {
    m_sRmlFile = sAbsFilePath;

    res.m_State = plResourceState::Loaded;
    return res;
  }

  plAssetFileHeader assetHeader;
  assetHeader.Read(*Stream).IgnoreResult();

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(desc));
}

void plRmlUiResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

PL_RESOURCE_IMPLEMENT_CREATEABLE(plRmlUiResource, plRmlUiResourceDescriptor)
{
  m_sRmlFile = descriptor.m_sRmlFile;
  m_ScaleMode = descriptor.m_ScaleMode;
  m_vReferenceResolution = descriptor.m_ReferenceResolution;

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}

//////////////////////////////////////////////////////////////////////////

bool plRmlUiResourceLoader::IsResourceOutdated(const plResource* pResource) const
{
  if (plResourceLoaderFromFile::IsResourceOutdated(pResource))
    return true;

  plStringBuilder sId = pResource->GetResourceID();
  if (sId.GetFileExtension() == "rml")
    return false;

  plFileReader stream;
  if (stream.Open(pResource->GetResourceID()).Failed())
    return false;

  // skip asset header
  plAssetFileHeader assetHeader;
  assetHeader.Read(stream).IgnoreResult();

  plDependencyFile dep;
  if (dep.ReadDependencyFile(stream).Failed())
    return true;

  return dep.HasAnyFileChanged();
}

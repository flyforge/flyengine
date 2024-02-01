#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>

plProbeTreeSectorResourceDescriptor::plProbeTreeSectorResourceDescriptor() = default;
plProbeTreeSectorResourceDescriptor::~plProbeTreeSectorResourceDescriptor() = default;
plProbeTreeSectorResourceDescriptor& plProbeTreeSectorResourceDescriptor::operator=(plProbeTreeSectorResourceDescriptor&& other) = default;

void plProbeTreeSectorResourceDescriptor::Clear()
{
  m_ProbePositions.Clear();
  m_SkyVisibility.Clear();
}

plUInt64 plProbeTreeSectorResourceDescriptor::GetHeapMemoryUsage() const
{
  plUInt64 uiMemUsage = 0;
  uiMemUsage += m_ProbePositions.GetHeapMemoryUsage();
  uiMemUsage += m_SkyVisibility.GetHeapMemoryUsage();
  return uiMemUsage;
}

static plTypeVersion s_ProbeTreeResourceDescriptorVersion = 1;
plResult plProbeTreeSectorResourceDescriptor::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_ProbeTreeResourceDescriptorVersion);

  inout_stream << m_vGridOrigin;
  inout_stream << m_vProbeSpacing;
  inout_stream << m_vProbeCount;

  PL_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_ProbePositions));
  PL_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_SkyVisibility));

  return PL_SUCCESS;
}

plResult plProbeTreeSectorResourceDescriptor::Deserialize(plStreamReader& inout_stream)
{
  Clear();

  const plTypeVersion version = inout_stream.ReadVersion(s_ProbeTreeResourceDescriptorVersion);
  PL_IGNORE_UNUSED(version);

  inout_stream >> m_vGridOrigin;
  inout_stream >> m_vProbeSpacing;
  inout_stream >> m_vProbeCount;

  PL_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_ProbePositions));
  PL_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_SkyVisibility));

  return PL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProbeTreeSectorResource, 1, plRTTIDefaultAllocator<plProbeTreeSectorResource>);
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plProbeTreeSectorResource);
// clang-format on

plProbeTreeSectorResource::plProbeTreeSectorResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

plProbeTreeSectorResource::~plProbeTreeSectorResource() = default;

plResourceLoadDesc plProbeTreeSectorResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  m_Desc.Clear();

  return res;
}

plResourceLoadDesc plProbeTreeSectorResource::UpdateContent(plStreamReader* Stream)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    plString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  plAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  plProbeTreeSectorResourceDescriptor descriptor;
  if (descriptor.Deserialize(*Stream).Failed())
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(descriptor));
}

void plProbeTreeSectorResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plProbeTreeSectorResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_Desc.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

plResourceLoadDesc plProbeTreeSectorResource::CreateResource(plProbeTreeSectorResourceDescriptor&& descriptor)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  m_Desc = std::move(descriptor);

  return res;
}


PL_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_ProbeTreeSectorResource);

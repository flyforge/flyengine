#include <Core/CorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Core/Curves/ColorGradientResource.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plColorGradientResource, 1, plRTTIDefaultAllocator<plColorGradientResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plColorGradientResource);

plColorGradientResource::plColorGradientResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

PL_RESOURCE_IMPLEMENT_CREATEABLE(plColorGradientResource, plColorGradientResourceDescriptor)
{
  m_Descriptor = descriptor;

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}

plResourceLoadDesc plColorGradientResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  m_Descriptor.m_Gradient.Clear();

  return res;
}

plResourceLoadDesc plColorGradientResource::UpdateContent(plStreamReader* Stream)
{
  PL_LOG_BLOCK("plColorGradientResource::UpdateContent", GetResourceIdOrDescription());

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
    plStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  plAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_Descriptor.Load(*Stream);

  res.m_State = plResourceState::Loaded;
  return res;
}

void plColorGradientResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<plUInt32>(m_Descriptor.m_Gradient.GetHeapMemoryUsage()) + static_cast<plUInt32>(sizeof(m_Descriptor));
}

void plColorGradientResourceDescriptor::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  m_Gradient.Save(inout_stream);
}

void plColorGradientResourceDescriptor::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion == 1, "Invalid file version {0}", uiVersion);

  m_Gradient.Load(inout_stream);
}



PL_STATICLINK_FILE(Core, Core_Curves_Implementation_ColorGradientResource);

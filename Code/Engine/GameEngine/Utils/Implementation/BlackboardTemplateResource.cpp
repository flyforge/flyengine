#include <GameEngine/GameEnginePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plBlackboardTemplateResource, 1, plRTTIDefaultAllocator<plBlackboardTemplateResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plBlackboardTemplateResource);
// clang-format on

plBlackboardTemplateResource::plBlackboardTemplateResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

plBlackboardTemplateResource::~plBlackboardTemplateResource() = default;

plResourceLoadDesc plBlackboardTemplateResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

plResourceLoadDesc plBlackboardTemplateResource::UpdateContent(plStreamReader* Stream)
{
  PLASMA_LOG_BLOCK("plBlackboardTemplateResource::UpdateContent", GetResourceIdOrDescription());

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  plStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // skip the asset file header at the start of the file
  plAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  plBlackboardTemplateResourceDescriptor desc;
  if (desc.Deserialize(*Stream).Failed())
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  CreateResource(std::move(desc));

  res.m_State = plResourceState::Loaded;
  return res;
}

void plBlackboardTemplateResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plBlackboardTemplateResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_Descriptor.m_Entries.GetHeapMemoryUsage();
}

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plBlackboardTemplateResource, plBlackboardTemplateResourceDescriptor)
{
  m_Descriptor = std::move(descriptor);

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}

plResult plBlackboardTemplateResourceDescriptor::Serialize(plStreamWriter& inout_stream) const
{
  PLASMA_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Entries));
  return PLASMA_SUCCESS;
}

plResult plBlackboardTemplateResourceDescriptor::Deserialize(plStreamReader& inout_stream)
{
  PLASMA_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Entries));
  return PLASMA_SUCCESS;
}

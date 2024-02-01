#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <TypeScriptPlugin/Resources/ScriptCompendiumResource.h>

// clang-format off
PL_RESOURCE_IMPLEMENT_COMMON_CODE(plScriptCompendiumResource);

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plScriptCompendiumResource, 1, plRTTIDefaultAllocator<plScriptCompendiumResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plScriptCompendiumResource::plScriptCompendiumResource()
  : plResource(plResource::DoUpdate::OnAnyThread, 1)
{
}

plScriptCompendiumResource::~plScriptCompendiumResource() = default;

plResourceLoadDesc plScriptCompendiumResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc ld;
  ld.m_State = plResourceState::Unloaded;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  m_Desc.m_PathToSource.Clear();

  return ld;
}

plResourceLoadDesc plScriptCompendiumResource::UpdateContent(plStreamReader* pStream)
{
  plResourceLoadDesc ld;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  if (pStream == nullptr)
  {
    ld.m_State = plResourceState::LoadedResourceMissing;
    return ld;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    plString sAbsFilePath;
    (*pStream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  plAssetFileHeader AssetHash;
  AssetHash.Read(*pStream).IgnoreResult();

  m_Desc.Deserialize(*pStream).IgnoreResult();

  ld.m_State = plResourceState::Loaded;

  return ld;
}

void plScriptCompendiumResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = (plUInt32)sizeof(plScriptCompendiumResource) + (plUInt32)m_Desc.m_PathToSource.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

//////////////////////////////////////////////////////////////////////////

plResult plScriptCompendiumResourceDesc::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(2);

  PL_SUCCEED_OR_RETURN(inout_stream.WriteMap(m_PathToSource));
  PL_SUCCEED_OR_RETURN(inout_stream.WriteMap(m_AssetGuidToInfo));

  return PL_SUCCESS;
}

plResult plScriptCompendiumResourceDesc::Deserialize(plStreamReader& inout_stream)
{
  plTypeVersion version = inout_stream.ReadVersion(2);

  PL_SUCCEED_OR_RETURN(inout_stream.ReadMap(m_PathToSource));

  if (version >= 2)
  {
    PL_SUCCEED_OR_RETURN(inout_stream.ReadMap(m_AssetGuidToInfo));
  }

  return PL_SUCCESS;
}

plResult plScriptCompendiumResourceDesc::ComponentTypeInfo::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(inout_stream.WriteString(m_sComponentTypeName));
  PL_SUCCEED_OR_RETURN(inout_stream.WriteString(m_sComponentFilePath));
  return PL_SUCCESS;
}

plResult plScriptCompendiumResourceDesc::ComponentTypeInfo::Deserialize(plStreamReader& inout_stream)
{
  plTypeVersion version = inout_stream.ReadVersion(1);
  PL_IGNORE_UNUSED(version);

  PL_SUCCEED_OR_RETURN(inout_stream.ReadString(m_sComponentTypeName));
  PL_SUCCEED_OR_RETURN(inout_stream.ReadString(m_sComponentFilePath));
  return PL_SUCCESS;
}

#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <TypeScriptPlugin/Resources/ScriptCompendiumResource.h>

// clang-format off
PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plScriptCompendiumResource);

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plScriptCompendiumResource, 1, plRTTIDefaultAllocator<plScriptCompendiumResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
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

plResult plScriptCompendiumResourceDesc::Serialize(plStreamWriter& stream) const
{
  stream.WriteVersion(2);

  PLASMA_SUCCEED_OR_RETURN(stream.WriteMap(m_PathToSource));
  PLASMA_SUCCEED_OR_RETURN(stream.WriteMap(m_AssetGuidToInfo));

  return PLASMA_SUCCESS;
}

plResult plScriptCompendiumResourceDesc::Deserialize(plStreamReader& stream)
{
  plTypeVersion version = stream.ReadVersion(2);

  PLASMA_SUCCEED_OR_RETURN(stream.ReadMap(m_PathToSource));

  if (version >= 2)
  {
    PLASMA_SUCCEED_OR_RETURN(stream.ReadMap(m_AssetGuidToInfo));
  }

  return PLASMA_SUCCESS;
}

plResult plScriptCompendiumResourceDesc::ComponentTypeInfo::Serialize(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(stream.WriteString(m_sComponentTypeName));
  PLASMA_SUCCEED_OR_RETURN(stream.WriteString(m_sComponentFilePath));
  return PLASMA_SUCCESS;
}

plResult plScriptCompendiumResourceDesc::ComponentTypeInfo::Deserialize(plStreamReader& stream)
{
  plTypeVersion version = stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(stream.ReadString(m_sComponentTypeName));
  PLASMA_SUCCEED_OR_RETURN(stream.ReadString(m_sComponentFilePath));
  return PLASMA_SUCCESS;
}

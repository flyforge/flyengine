#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plShaderResource, 1, plRTTIDefaultAllocator<plShaderResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plShaderResource);
// clang-format on

plShaderResource::plShaderResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
  m_bShaderResourceIsValid = false;
}

plResourceLoadDesc plShaderResource::UnloadData(Unload WhatToUnload)
{
  m_bShaderResourceIsValid = false;
  m_PermutationVarsUsed.Clear();

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

plResourceLoadDesc plShaderResource::UpdateContent(plStreamReader* stream)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  m_bShaderResourceIsValid = false;

  if (stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    plStringBuilder sAbsFilePath;
    (*stream) >> sAbsFilePath;
  }

  plHybridArray<plPermutationVar, 16> fixedPermVars; // ignored here
  plShaderParser::ParsePermutationSection(*stream, m_PermutationVarsUsed, fixedPermVars);

  res.m_State = plResourceState::Loaded;
  m_bShaderResourceIsValid = true;

  return res;
}

void plShaderResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plShaderResource) + (plUInt32)m_PermutationVarsUsed.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plShaderResource, plShaderResourceDescriptor)
{
  plResourceLoadDesc ret;
  ret.m_State = plResourceState::Loaded;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;

  m_bShaderResourceIsValid = false;

  return ret;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderResource);

#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Shader/Shader.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plShaderPermutationResource, 1, plRTTIDefaultAllocator<plShaderPermutationResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plShaderPermutationResource);
// clang-format on

static plShaderPermutationResourceLoader g_PermutationResourceLoader;

plShaderPermutationResource::plShaderPermutationResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
  m_bShaderPermutationValid = false;

  for (plUInt32 stage = 0; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    m_pShaderStageBinaries[stage] = nullptr;
  }
}

plResourceLoadDesc plShaderPermutationResource::UnloadData(Unload WhatToUnload)
{
  m_bShaderPermutationValid = false;

  auto pDevice = plGALDevice::GetDefaultDevice();

  if (!m_hShader.IsInvalidated())
  {
    pDevice->DestroyShader(m_hShader);
    m_hShader.Invalidate();
  }

  if (!m_hBlendState.IsInvalidated())
  {
    pDevice->DestroyBlendState(m_hBlendState);
    m_hBlendState.Invalidate();
  }

  if (!m_hDepthStencilState.IsInvalidated())
  {
    pDevice->DestroyDepthStencilState(m_hDepthStencilState);
    m_hDepthStencilState.Invalidate();
  }

  if (!m_hRasterizerState.IsInvalidated())
  {
    pDevice->DestroyRasterizerState(m_hRasterizerState);
    m_hRasterizerState.Invalidate();
  }


  plResourceLoadDesc res;
  res.m_State = plResourceState::Unloaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  return res;
}

plResourceLoadDesc plShaderPermutationResource::UpdateContent(plStreamReader* Stream)
{
  plUInt32 uiGPUMem = 0;
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  m_bShaderPermutationValid = false;

  plResourceLoadDesc res;
  res.m_State = plResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    plLog::Error("Shader Permutation '{0}': Data is not available", GetResourceID());
    return res;
  }

  plShaderPermutationBinary PermutationBinary;

  bool bOldVersion = false;
  if (PermutationBinary.Read(*Stream, bOldVersion).Failed())
  {
    plLog::Error("Shader Permutation '{0}': Could not read shader permutation binary", GetResourceID());
    return res;
  }

  auto pDevice = plGALDevice::GetDefaultDevice();

  // get the shader render state object
  {
    m_hBlendState = pDevice->CreateBlendState(PermutationBinary.m_StateDescriptor.m_BlendDesc);
    m_hDepthStencilState = pDevice->CreateDepthStencilState(PermutationBinary.m_StateDescriptor.m_DepthStencilDesc);
    m_hRasterizerState = pDevice->CreateRasterizerState(PermutationBinary.m_StateDescriptor.m_RasterizerDesc);
  }

  plGALShaderCreationDescription ShaderDesc;

  // iterate over all shader stages, add them to the descriptor
  for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    const plUInt32 uiStageHash = PermutationBinary.m_uiShaderStageHashes[stage];

    if (uiStageHash == 0) // not used
      continue;

    plShaderStageBinary* pStageBin = plShaderStageBinary::LoadStageBinary((plGALShaderStage::Enum)stage, uiStageHash);

    if (pStageBin == nullptr)
    {
      plLog::Error("Shader Permutation '{0}': Stage '{1}' could not be loaded", GetResourceID(), plGALShaderStage::Names[stage]);
      return res;
    }

    // store not only the hash but also the pointer to the stage binary
    // since it contains other useful information (resource bindings), that we need for shader binding
    m_pShaderStageBinaries[stage] = pStageBin;

    PLASMA_ASSERT_DEV(pStageBin->m_Stage == stage, "Invalid shader stage! Expected stage '{0}', but loaded data is for stage '{1}'", plGALShaderStage::Names[stage], plGALShaderStage::Names[pStageBin->m_Stage]);

    ShaderDesc.m_ByteCodes[stage] = pStageBin->m_GALByteCode;

    uiGPUMem += pStageBin->m_ByteCode.GetCount();
  }

  m_hShader = pDevice->CreateShader(ShaderDesc);

  if (m_hShader.IsInvalidated())
  {
    plLog::Error("Shader Permutation '{0}': Shader program creation failed", GetResourceID());
    return res;
  }

  pDevice->GetShader(m_hShader)->SetDebugName(GetResourceID());

  m_PermutationVars = PermutationBinary.m_PermutationVars;

  m_bShaderPermutationValid = true;

  ModifyMemoryUsage().m_uiMemoryGPU = uiGPUMem;

  return res;
}

void plShaderPermutationResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plShaderPermutationResource);
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plShaderPermutationResource, plShaderPermutationResourceDescriptor)
{
  plResourceLoadDesc ret;
  ret.m_State = plResourceState::Loaded;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;

  return ret;
}

plResourceTypeLoader* plShaderPermutationResource::GetDefaultResourceTypeLoader() const
{
  return &g_PermutationResourceLoader;
}

struct ShaderPermutationResourceLoadData
{
  ShaderPermutationResourceLoadData()
    : m_Reader(&m_Storage)
  {
  }

  plContiguousMemoryStreamStorage m_Storage;
  plMemoryStreamReader m_Reader;
};

plResult plShaderPermutationResourceLoader::RunCompiler(const plResource* pResource, plShaderPermutationBinary& BinaryInfo, bool bForce)
{
  if (plShaderManager::IsRuntimeCompilationEnabled())
  {
    if (!bForce)
    {
      // check whether any dependent file has changed, and trigger a recompilation if necessary
      if (BinaryInfo.m_DependencyFile.HasAnyFileChanged())
      {
        bForce = true;
      }
    }

    if (!bForce) // no recompilation necessary
      return PLASMA_SUCCESS;

    plStringBuilder sPermutationFile = pResource->GetResourceID();

    sPermutationFile.ChangeFileExtension("");
    sPermutationFile.Shrink(plShaderManager::GetCacheDirectory().GetCharacterCount() + plShaderManager::GetActivePlatform().GetCharacterCount() + 2, 1);

    sPermutationFile.Shrink(0, 9); // remove underscore and the hash at the end
    sPermutationFile.Append(".plShader");

    plArrayPtr<const plPermutationVar> permutationVars = static_cast<const plShaderPermutationResource*>(pResource)->GetPermutationVars();

    plShaderCompiler sc;
    return sc.CompileShaderPermutationForPlatforms(sPermutationFile, permutationVars, plLog::GetThreadLocalLogSystem(), plShaderManager::GetActivePlatform());
  }
  else
  {
    if (bForce)
    {
      plLog::Error("Shader was forced to be compiled, but runtime shader compilation is not available");
      return PLASMA_FAILURE;
    }
  }

  return PLASMA_SUCCESS;
}

bool plShaderPermutationResourceLoader::IsResourceOutdated(const plResource* pResource) const
{
  // don't try to reload a file that cannot be found
  plStringBuilder sAbs;
  if (plFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
    return false;

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS)
  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    plFileStats stat;
    if (plFileSystem::GetFileStats(pResource->GetResourceID(), stat).Failed())
      return false;

    if (!stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), plTimestamp::CompareMode::FileTimeEqual))
      return true;
  }

#endif

  plDependencyFile dep;
  if (dep.ReadDependencyFile(pResource->GetResourceID()).Failed())
    return true;

  return dep.HasAnyFileChanged();
}

plResourceLoadData plShaderPermutationResourceLoader::OpenDataStream(const plResource* pResource)
{
  plResourceLoadData res;

  plShaderPermutationBinary permutationBinary;

  bool bNeedsCompilation = true;
  bool bOldVersion = false;

  {
    plFileReader File;
    if (File.Open(pResource->GetResourceID().GetData()).Failed())
    {
      plLog::Debug("Shader Permutation '{0}' does not exist, triggering recompile.", pResource->GetResourceID());

      bNeedsCompilation = false;
      if (RunCompiler(pResource, permutationBinary, true).Failed())
        return res;

      // try again
      if (File.Open(pResource->GetResourceID().GetData()).Failed())
      {
        plLog::Debug("Shader Permutation '{0}' still does not exist after recompile.", pResource->GetResourceID());
        return res;
      }
    }

    res.m_sResourceDescription = File.GetFilePathRelative().GetData();

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS)
    plFileStats stat;
    if (plFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
    {
      res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
    }
#endif

    if (permutationBinary.Read(File, bOldVersion).Failed())
    {
      plLog::Error("Shader Permutation '{0}': Could not read shader permutation binary", pResource->GetResourceID());

      bNeedsCompilation = true;
    }

    if (bOldVersion)
    {
      plLog::Dev("Shader Permutation Binary version is outdated, recompiling shader.");
      bNeedsCompilation = true;
    }
  }

  if (bNeedsCompilation)
  {
    if (RunCompiler(pResource, permutationBinary, false).Failed())
      return res;

    plFileReader File;

    if (File.Open(pResource->GetResourceID().GetData()).Failed())
    {
      plLog::Error("Shader Permutation '{0}': Failed to open the file", pResource->GetResourceID());
      return res;
    }

    if (permutationBinary.Read(File, bOldVersion).Failed())
    {
      plLog::Error("Shader Permutation '{0}': Binary data could not be read", pResource->GetResourceID());
      return res;
    }

    File.Close();
  }



  ShaderPermutationResourceLoadData* pData = PLASMA_DEFAULT_NEW(ShaderPermutationResourceLoadData);

  plMemoryStreamWriter w(&pData->m_Storage);

  // preload the files that are referenced in the .plPermutation file
  {
    // write the permutation file info back to the output stream, so that the resource can read it as well
    permutationBinary.Write(w).IgnoreResult();

    for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
    {
      const plUInt32 uiStageHash = permutationBinary.m_uiShaderStageHashes[stage];

      if (uiStageHash == 0) // not used
        continue;

      // this is where the preloading happens
      plShaderStageBinary::LoadStageBinary((plGALShaderStage::Enum)stage, uiStageHash);
    }
  }

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void plShaderPermutationResourceLoader::CloseDataStream(const plResource* pResource, const plResourceLoadData& loaderData)
{
  ShaderPermutationResourceLoadData* pData = static_cast<ShaderPermutationResourceLoadData*>(loaderData.m_pCustomLoaderData);

  PLASMA_DEFAULT_DELETE(pData);
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderPermutationResource);

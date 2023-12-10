#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/Passes/SourcePass.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>

void plRenderPipelineResourceDescriptor::CreateFromRenderPipeline(const plRenderPipeline* pPipeline)
{
  plRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(pPipeline, *this);
}

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRenderPipelineResource, 1, plRTTIDefaultAllocator<plRenderPipelineResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plRenderPipelineResource);
// clang-format on

plRenderPipelineResource::plRenderPipelineResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

plInternal::NewInstance<plRenderPipeline> plRenderPipelineResource::CreateRenderPipeline() const
{
  if (GetLoadingState() != plResourceState::Loaded)
  {
    plLog::Error("Can't create render pipeline '{0}', the resource is not loaded!", GetResourceID());
    return plInternal::NewInstance<plRenderPipeline>(nullptr, nullptr);
  }

  return plRenderPipelineResourceLoader::CreateRenderPipeline(m_Desc);
}

// static
plRenderPipelineResourceHandle plRenderPipelineResource::CreateMissingPipeline()
{
  plUniquePtr<plRenderPipeline> pRenderPipeline = PLASMA_DEFAULT_NEW(plRenderPipeline);

  plSourcePass* pColorSourcePass = nullptr;
  {
    plUniquePtr<plSourcePass> pPass = PLASMA_DEFAULT_NEW(plSourcePass, "ColorSource");
    pColorSourcePass = pPass.Borrow();
    pRenderPipeline->AddPass(std::move(pPass));
  }

  plSimpleRenderPass* pSimplePass = nullptr;
  {
    plUniquePtr<plSimpleRenderPass> pPass = PLASMA_DEFAULT_NEW(plSimpleRenderPass);
    pSimplePass = pPass.Borrow();
    pSimplePass->SetMessage("Render pipeline resource is missing. Ensure that the corresponding asset has been transformed.");
    pRenderPipeline->AddPass(std::move(pPass));
  }

  plTargetPass* pTargetPass = nullptr;
  {
    plUniquePtr<plTargetPass> pPass = PLASMA_DEFAULT_NEW(plTargetPass);
    pTargetPass = pPass.Borrow();
    pRenderPipeline->AddPass(std::move(pPass));
  }

  PLASMA_VERIFY(pRenderPipeline->Connect(pColorSourcePass, "Output", pSimplePass, "Color"), "Connect failed!");
  PLASMA_VERIFY(pRenderPipeline->Connect(pSimplePass, "Color", pTargetPass, "Color0"), "Connect failed!");

  plRenderPipelineResourceDescriptor desc;
  plRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(pRenderPipeline.Borrow(), desc);

  return plResourceManager::CreateResource<plRenderPipelineResource>("MissingRenderPipeline", std::move(desc), "MissingRenderPipeline");
}

plResourceLoadDesc plRenderPipelineResource::UnloadData(Unload WhatToUnload)
{
  m_Desc.Clear();

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

plResourceLoadDesc plRenderPipelineResource::UpdateContent(plStreamReader* Stream)
{
  m_Desc.Clear();

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  if (Stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  plStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  if (sAbsFilePath.HasExtension("plRenderPipelineBin"))
  {
    plStringBuilder sTemp, sTemp2;

    plAssetFileHeader AssetHash;
    AssetHash.Read(*Stream).IgnoreResult();

    plUInt8 uiVersion = 0;
    (*Stream) >> uiVersion;

    // Version 1 was using old tooling serialization. Code path removed.
    if (uiVersion == 1)
    {
      res.m_State = plResourceState::LoadedResourceMissing;
      plLog::Error("Failed to load old plRenderPipelineResource '{}'. Needs re-transform.", sAbsFilePath);
      return res;
    }
    PLASMA_ASSERT_DEV(uiVersion == 2, "Unknown plRenderPipelineBin version {0}", uiVersion);

    plUInt32 uiSize = 0;
    (*Stream) >> uiSize;

    m_Desc.m_SerializedPipeline.SetCountUninitialized(uiSize);
    Stream->ReadBytes(m_Desc.m_SerializedPipeline.GetData(), uiSize);

    PLASMA_ASSERT_DEV(uiSize > 0, "RenderPipeline resourse contains no pipeline data!");
  }
  else
  {
    PLASMA_REPORT_FAILURE("The file '{0}' is unsupported, only '.plRenderPipelineBin' files can be loaded as plRenderPipelineResource", sAbsFilePath);
  }

  return res;
}

void plRenderPipelineResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plRenderPipelineResource) + (plUInt32)(m_Desc.m_SerializedPipeline.GetCount());

  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plRenderPipelineResource, plRenderPipelineResourceDescriptor)
{
  m_Desc = descriptor;

  plResourceLoadDesc res;
  res.m_State = plResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  return res;
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineResource);

#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/Implementation/MeshRendererUtils.h>
#include <RendererCore/Meshes/InstancedMeshComponent.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMeshRenderer, 1, plRTTIDefaultAllocator<plMeshRenderer>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plMeshRenderer::plMeshRenderer() = default;
plMeshRenderer::~plMeshRenderer() = default;

void plMeshRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(plGetStaticRTTI<plMeshRenderData>());
  ref_types.PushBack(plGetStaticRTTI<plInstancedMeshRenderData>());
}

void plMeshRenderer::GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(plDefaultRenderDataCategories::Sky);
  ref_categories.PushBack(plDefaultRenderDataCategories::LitOpaque);
  ref_categories.PushBack(plDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(plDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(plDefaultRenderDataCategories::LitForeground);
  ref_categories.PushBack(plDefaultRenderDataCategories::SimpleOpaque);
  ref_categories.PushBack(plDefaultRenderDataCategories::SimpleTransparent);
  ref_categories.PushBack(plDefaultRenderDataCategories::SimpleForeground);
  ref_categories.PushBack(plDefaultRenderDataCategories::Selection);
  ref_categories.PushBack(plDefaultRenderDataCategories::GUI);
}

void plMeshRenderer::RenderBatch(const plRenderViewContext& renderViewContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const
{
  plRenderContext* pContext = renderViewContext.m_pRenderContext;

  const plMeshRenderData* pRenderData = batch.GetFirstData<plMeshRenderData>();

  const plMeshResourceHandle& hMesh = pRenderData->m_hMesh;
  const plMaterialResourceHandle& hMaterial = pRenderData->m_hMaterial;
  const plUInt32 uiPartIndex = pRenderData->m_uiSubMeshIndex;
  const bool bHasExplicitInstanceData = pRenderData->IsInstanceOf<plInstancedMeshRenderData>();

  plResourceLock<plMeshResource> pMesh(hMesh, plResourceAcquireMode::AllowLoadingFallback);

  // This can happen when the resource has been reloaded and now has fewer submeshes.
  const auto& subMeshes = pMesh->GetSubMeshes();
  if (subMeshes.GetCount() <= uiPartIndex)
  {
    return;
  }

  plInstanceData* pInstanceData = bHasExplicitInstanceData ? static_cast<const plInstancedMeshRenderData*>(pRenderData)->m_pExplicitInstanceData : pPass->GetPipeline()->GetFrameDataProvider<plInstanceDataProvider>()->GetData(renderViewContext);

  pInstanceData->BindResources(pContext);

  if (pRenderData->m_uiFlipWinding)
  {
    pContext->SetShaderPermutationVariable("FLIP_WINDING", "TRUE");
  }
  else
  {
    pContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  }

  pContext->BindMaterial(hMaterial);
  pContext->BindMeshBuffer(pMesh->GetMeshBuffer());

  SetAdditionalData(renderViewContext, pRenderData);

  if (!bHasExplicitInstanceData)
  {
    plUInt32 uiStartIndex = 0;
    while (uiStartIndex < batch.GetCount())
    {
      const plUInt32 uiRemainingInstances = batch.GetCount() - uiStartIndex;

      plUInt32 uiInstanceDataOffset = 0;
      plArrayPtr<plPerInstanceData> instanceData = pInstanceData->GetInstanceData(uiRemainingInstances, uiInstanceDataOffset);

      plUInt32 uiFilteredCount = 0;
      FillPerInstanceData(instanceData, batch, uiStartIndex, uiFilteredCount);

      if (uiFilteredCount > 0) // Instance data might be empty if all render data was filtered.
      {
        pInstanceData->UpdateInstanceData(pContext, uiFilteredCount);

        const plMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];

        if (pContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive, uiFilteredCount).Failed())
        {
          for (auto it = batch.GetIterator<plMeshRenderData>(uiStartIndex, instanceData.GetCount()); it.IsValid(); ++it)
          {
            pRenderData = it;

            // draw bounding box instead
            if (pRenderData->m_GlobalBounds.IsValid())
            {
              plDebugRenderer::DrawLineBox(*renderViewContext.m_pViewDebugContext, pRenderData->m_GlobalBounds.GetBox(), plColor::Magenta);
            }
          }
        }
      }

      uiStartIndex += instanceData.GetCount();
    }
  }
  else
  {
    plUInt32 uiInstanceCount = static_cast<const plInstancedMeshRenderData*>(pRenderData)->m_uiExplicitInstanceCount;

    const plMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];

    pContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive, uiInstanceCount).IgnoreResult();
  }
}

void plMeshRenderer::SetAdditionalData(const plRenderViewContext& renderViewContext, const plMeshRenderData* pRenderData) const
{
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");
}

void plMeshRenderer::FillPerInstanceData(plArrayPtr<plPerInstanceData> instanceData, const plRenderDataBatch& batch, plUInt32 uiStartIndex, plUInt32& out_uiFilteredCount) const
{
  plUInt32 uiCount = plMath::Min<plUInt32>(instanceData.GetCount(), batch.GetCount() - uiStartIndex);
  plUInt32 uiCurrentIndex = 0;

  for (auto it = batch.GetIterator<plMeshRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    plInternal::FillPerInstanceData(instanceData[uiCurrentIndex], it);

    ++uiCurrentIndex;
  }

  out_uiFilteredCount = uiCurrentIndex;
}

PL_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshRenderer);

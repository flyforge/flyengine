#include <KrautPlugin/KrautPluginPCH.h>

#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <KrautPlugin/Renderer/KrautRenderer.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plKrautRenderData, 1, plRTTIDefaultAllocator<plKrautRenderData>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plKrautRenderer, 1, plRTTIDefaultAllocator<plKrautRenderer>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plKrautRenderer::plKrautRenderer() = default;
plKrautRenderer::~plKrautRenderer() = default;

void plKrautRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(plGetStaticRTTI<plKrautRenderData>());
}

void plKrautRenderer::GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(plDefaultRenderDataCategories::LitOpaque);
  ref_categories.PushBack(plDefaultRenderDataCategories::Selection);
}

void plKrautRenderer::RenderBatch(const plRenderViewContext& renderViewContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const
{
  plRenderContext* pRenderContext = renderViewContext.m_pRenderContext;

  const plKrautRenderData* pRenderData = batch.GetFirstData<plKrautRenderData>();

  plResourceLock<plMeshResource> pMesh(pRenderData->m_hMesh, plResourceAcquireMode::AllowLoadingFallback);

  // This can happen when the resource has been reloaded and now has fewer sub-meshes.
  if (pMesh->GetSubMeshes().GetCount() <= pRenderData->m_uiSubMeshIndex)
    return;

  TempTreeCB treeConstants(pRenderContext);

  const auto& subMesh = pMesh->GetSubMeshes()[pRenderData->m_uiSubMeshIndex];

  plInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<plInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pRenderContext);

  // inverted trees are not allowed
  pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  // no skinning atm
  pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");

  pRenderContext->BindMaterial(pMesh->GetMaterials()[subMesh.m_uiMaterialIndex]);
  pRenderContext->BindMeshBuffer(pMesh->GetMeshBuffer());

  treeConstants.SetTreeData(pRenderData->m_vLeafCenter, renderViewContext.m_pViewData->m_CameraUsageHint == plCameraUsageHint::Shadow ? 1.0f : 0.0f);

  const plVec3 vLodCamPos = renderViewContext.m_pLodCamera->GetPosition();

  const bool bIsShadowView = renderViewContext.m_pViewData->m_CameraUsageHint == plCameraUsageHint::Shadow;

  for (plUInt32 uiStartIndex = 0; uiStartIndex < batch.GetCount(); /**/)
  {
    const plUInt32 uiRemainingInstances = batch.GetCount() - uiStartIndex;

    plUInt32 uiInstanceDataOffset = 0;
    plArrayPtr<plPerInstanceData> instanceData = pInstanceData->GetInstanceData(uiRemainingInstances, uiInstanceDataOffset);

    plUInt32 uiFilteredCount = 0;
    FillPerInstanceData(vLodCamPos, instanceData, batch, bIsShadowView, uiStartIndex, uiFilteredCount);

    if (uiFilteredCount > 0) // Instance data might be empty if all render data was filtered.
    {
      pInstanceData->UpdateInstanceData(pRenderContext, uiFilteredCount);

      if (pRenderContext->DrawMeshBuffer(subMesh.m_uiPrimitiveCount, subMesh.m_uiFirstPrimitive, uiFilteredCount).Failed())
      {
        for (auto it = batch.GetIterator<plKrautRenderData>(uiStartIndex, instanceData.GetCount()); it.IsValid(); ++it)
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

void plKrautRenderer::FillPerInstanceData(const plVec3& vLodCamPos, plArrayPtr<plPerInstanceData> instanceData, const plRenderDataBatch& batch, bool bIsShadowView, plUInt32 uiStartIndex, plUInt32& out_uiFilteredCount) const
{
  plUInt32 uiCount = plMath::Min<plUInt32>(instanceData.GetCount(), batch.GetCount() - uiStartIndex);
  plUInt32 uiCurrentIndex = 0;

  for (auto it = batch.GetIterator<plKrautRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    const plKrautRenderData* pRenderData = it;

    const float fDistanceSQR = (pRenderData->m_GlobalTransform.m_vPosition - vLodCamPos).GetLengthSquared();

    if (fDistanceSQR < pRenderData->m_fLodDistanceMinSQR || fDistanceSQR >= pRenderData->m_fLodDistanceMaxSQR)
      continue;

    if (bIsShadowView && !pRenderData->m_bCastShadows)
      continue;

    const plMat4 objectToWorld = pRenderData->m_GlobalTransform.GetAsMat4();

    auto& perInstanceData = instanceData[uiCurrentIndex];
    perInstanceData.ObjectToWorld = objectToWorld;

    // always assumes uniform-scale only
    perInstanceData.ObjectToWorldNormal = objectToWorld;
    perInstanceData.GameObjectID = pRenderData->m_uiUniqueID;
    perInstanceData.Color = plColor(pRenderData->m_vWindTrunk.x, pRenderData->m_vWindTrunk.y, pRenderData->m_vWindTrunk.z, pRenderData->m_vWindTrunk.GetLength());

    ++uiCurrentIndex;
  }

  out_uiFilteredCount = uiCurrentIndex;
}

plKrautRenderer::TempTreeCB::TempTreeCB(plRenderContext* pRenderContext)
{
  // TODO This pattern looks like it is inefficient. Should it use the GPU pool instead somehow?
  m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage(m_pConstants);

  pRenderContext->BindConstantBuffer("plKrautTreeConstants", m_hConstantBuffer);
}

plKrautRenderer::TempTreeCB::~TempTreeCB()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void plKrautRenderer::TempTreeCB::SetTreeData(const plVec3& vTreeCenter, float fLeafShadowOffset)
{
  plKrautTreeConstants& cb = m_pConstants->GetDataForWriting();
  cb.LeafCenter = vTreeCenter;
  cb.LeafShadowOffset = fLeafShadowOffset;
}

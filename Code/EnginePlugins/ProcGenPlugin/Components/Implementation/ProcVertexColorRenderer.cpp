#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <ProcGenPlugin/Components/ProcVertexColorComponent.h>
#include <ProcGenPlugin/Components/ProcVertexColorRenderer.h>
#include <RendererCore/Meshes/Implementation/MeshRendererUtils.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcVertexColorRenderer, 1, plRTTIDefaultAllocator<plProcVertexColorRenderer>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plProcVertexColorRenderer::plProcVertexColorRenderer() = default;
plProcVertexColorRenderer::~plProcVertexColorRenderer() = default;

void plProcVertexColorRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(plGetStaticRTTI<plProcVertexColorRenderData>());
}

void plProcVertexColorRenderer::SetAdditionalData(const plRenderViewContext& renderViewContext, const plMeshRenderData* pRenderData) const
{
  SUPER::SetAdditionalData(renderViewContext, pRenderData);

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plRenderContext* pContext = renderViewContext.m_pRenderContext;

  auto pProcVertexColorRenderData = static_cast<const plProcVertexColorRenderData*>(pRenderData);

  pContext->BindBuffer("perInstanceVertexColors", pDevice->GetDefaultResourceView(pProcVertexColorRenderData->m_hVertexColorBuffer));
}

void plProcVertexColorRenderer::FillPerInstanceData(
  plArrayPtr<plPerInstanceData> instanceData, const plRenderDataBatch& batch, plUInt32 uiStartIndex, plUInt32& out_uiFilteredCount) const
{
  plUInt32 uiCount = plMath::Min<plUInt32>(instanceData.GetCount(), batch.GetCount() - uiStartIndex);
  plUInt32 uiCurrentIndex = 0;

  for (auto it = batch.GetIterator<plProcVertexColorRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    auto& perInstanceData = instanceData[uiCurrentIndex];

    plInternal::FillPerInstanceData(perInstanceData, it);
    perInstanceData.VertexColorAccessData = it->m_uiBufferAccessData;

    ++uiCurrentIndex;
  }

  out_uiFilteredCount = uiCurrentIndex;
}

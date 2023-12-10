#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <RendererCore/Meshes/SkinnedMeshRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSkinnedMeshRenderer, 1, plRTTIDefaultAllocator<plSkinnedMeshRenderer>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plUInt32 plSkinnedMeshRenderer::s_uiSkinningBufferUpdates = 0;

plSkinnedMeshRenderer::plSkinnedMeshRenderer() = default;
plSkinnedMeshRenderer::~plSkinnedMeshRenderer() = default;

void plSkinnedMeshRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(plGetStaticRTTI<plSkinnedMeshRenderData>());
}

void plSkinnedMeshRenderer::SetAdditionalData(const plRenderViewContext& renderViewContext, const plMeshRenderData* pRenderData) const
{
  // Don't call base class implementation here since the state will be overwritten in this method anyways.

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plRenderContext* pContext = renderViewContext.m_pRenderContext;

  auto pSkinnedRenderData = static_cast<const plSkinnedMeshRenderData*>(pRenderData);

  if (pSkinnedRenderData->m_hSkinningTransforms.IsInvalidated())
  {
    pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");
  }
  else
  {
    pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "TRUE");

    if (pSkinnedRenderData->m_bTransformsUpdated != nullptr && *pSkinnedRenderData->m_bTransformsUpdated == false)
    {
      // if this is the first renderer that is supposed to actually render the skinned mesh, upload the skinning matrices
      *pSkinnedRenderData->m_bTransformsUpdated = true;
      pContext->GetCommandEncoder()->UpdateBuffer(pSkinnedRenderData->m_hSkinningTransforms, 0, pSkinnedRenderData->m_pNewSkinningTransformData);

      // TODO: could expose this somewhere (plStats?)
      s_uiSkinningBufferUpdates++;
    }

    pContext->BindBuffer("skinningTransforms", pDevice->GetDefaultResourceView(pSkinnedRenderData->m_hSkinningTransforms));
  }
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_SkinnedMeshRenderer);

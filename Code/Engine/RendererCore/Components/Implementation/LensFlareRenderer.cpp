#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Components/LensFlareComponent.h>
#include <RendererCore/Components/LensFlareRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <Shaders/Materials/LensFlareData.h>
PLASMA_CHECK_AT_COMPILETIME(sizeof(plPerLensFlareData) == 48);

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLensFlareRenderer, 1, plRTTIDefaultAllocator<plLensFlareRenderer>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plLensFlareRenderer::plLensFlareRenderer()
{
  m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Materials/LensFlareMaterial.plShader");
}

plLensFlareRenderer::~plLensFlareRenderer() = default;

void plLensFlareRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(plGetStaticRTTI<plLensFlareRenderData>());
}

void plLensFlareRenderer::GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(plDefaultRenderDataCategories::LitTransparent);
}

void plLensFlareRenderer::RenderBatch(const plRenderViewContext& renderViewContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plRenderContext* pContext = renderViewContext.m_pRenderContext;

  const plLensFlareRenderData* pRenderData = batch.GetFirstData<plLensFlareRenderData>();

  const plUInt32 uiBufferSize = plMath::RoundUp(batch.GetCount(), 128u);
  plGALBufferHandle hLensFlareData = CreateLensFlareDataBuffer(uiBufferSize);
  PLASMA_SCOPE_EXIT(DeleteLensFlareDataBuffer(hLensFlareData));

  pContext->BindShader(m_hShader);
  pContext->BindBuffer("lensFlareData", pDevice->GetDefaultResourceView(hLensFlareData));
  pContext->BindTexture2D("LensFlareTexture", pRenderData->m_hTexture);

  FillLensFlareData(batch);

  if (m_LensFlareData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
  {
    pContext->GetCommandEncoder()->UpdateBuffer(hLensFlareData, 0, m_LensFlareData.GetByteArrayPtr());

    pContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, m_LensFlareData.GetCount() * 2);
    pContext->DrawMeshBuffer().IgnoreResult();
  }
}

plGALBufferHandle plLensFlareRenderer::CreateLensFlareDataBuffer(plUInt32 uiBufferSize) const
{
  plGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(plPerLensFlareData);
  desc.m_uiTotalSize = desc.m_uiStructSize * uiBufferSize;
  desc.m_BufferType = plGALBufferType::Generic;
  desc.m_bUseAsStructuredBuffer = true;
  desc.m_bAllowShaderResourceView = true;
  desc.m_ResourceAccess.m_bImmutable = false;

  return plGPUResourcePool::GetDefaultInstance()->GetBuffer(desc);
}

void plLensFlareRenderer::DeleteLensFlareDataBuffer(plGALBufferHandle hBuffer) const
{
  plGPUResourcePool::GetDefaultInstance()->ReturnBuffer(hBuffer);
}

void plLensFlareRenderer::FillLensFlareData(const plRenderDataBatch& batch) const
{
  m_LensFlareData.Clear();
  m_LensFlareData.Reserve(batch.GetCount());

  for (auto it = batch.GetIterator<plLensFlareRenderData>(); it.IsValid(); ++it)
  {
    const plLensFlareRenderData* pRenderData = it;

    auto& LensFlareData = m_LensFlareData.ExpandAndGetRef();
    LensFlareData.WorldSpacePosition = pRenderData->m_GlobalTransform.m_vPosition;
    LensFlareData.Size = pRenderData->m_fSize;
    LensFlareData.MaxScreenSize = pRenderData->m_fMaxScreenSize;
    LensFlareData.OcclusionRadius = pRenderData->m_fOcclusionSampleRadius;
    LensFlareData.OcclusionSpread = pRenderData->m_fOcclusionSampleSpread;
    LensFlareData.DepthOffset = pRenderData->m_fOcclusionDepthOffset;
    LensFlareData.AspectRatioAndShift = plShaderUtils::Float2ToRG16F(plVec2(pRenderData->m_fAspectRatio, pRenderData->m_fShiftToCenter));
    LensFlareData.ColorRG = plShaderUtils::PackFloat16intoUint(pRenderData->m_Color.x, pRenderData->m_Color.y);
    LensFlareData.ColorBA = plShaderUtils::PackFloat16intoUint(pRenderData->m_Color.z, pRenderData->m_Color.w);
    LensFlareData.Flags = (pRenderData->m_bInverseTonemap ? LENS_FLARE_INVERSE_TONEMAP : 0) |
                          (pRenderData->m_bGreyscaleTexture ? LENS_FLARE_GREYSCALE_TEXTURE : 0) |
                          (pRenderData->m_bApplyFog ? LENS_FLARE_APPLY_FOG : 0);
  }
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_LensFlareRenderer);

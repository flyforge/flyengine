#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleRenderer, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleRenderer::TempSystemCB::TempSystemCB(plRenderContext* pRenderContext)
{
  // TODO This pattern looks like it is inefficient. Should it use the GPU pool instead somehow?
  m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage(m_pConstants);

  pRenderContext->BindConstantBuffer("plParticleSystemConstants", m_hConstantBuffer);
}

plParticleRenderer::TempSystemCB::~TempSystemCB()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void plParticleRenderer::TempSystemCB::SetGenericData(bool bApplyObjectTransform, const plTransform& ObjectTransform, plTime effectLifeTime,
  plUInt8 uiNumVariationsX, plUInt8 uiNumVariationsY, plUInt8 uiNumFlipbookAnimsX, plUInt8 uiNumFlipbookAnimsY, float fDistortionStrength /*= 0*/)
{
  plParticleSystemConstants& cb = m_pConstants->GetDataForWriting();
  cb.TextureAtlasVariationFramesX = uiNumVariationsX;
  cb.TextureAtlasVariationFramesY = uiNumVariationsY;
  cb.TextureAtlasFlipbookFramesX = uiNumFlipbookAnimsX;
  cb.TextureAtlasFlipbookFramesY = uiNumFlipbookAnimsY;
  cb.DistortionStrength = fDistortionStrength;
  cb.TotalEffectLifeTime = effectLifeTime.AsFloatInSeconds();

  if (bApplyObjectTransform)
    cb.ObjectToWorldMatrix = ObjectTransform.GetAsMat4();
  else
    cb.ObjectToWorldMatrix.SetIdentity();
}


void plParticleRenderer::TempSystemCB::SetTrailData(float fSnapshotFraction, plInt32 iNumUsedTrailPoints)
{
  plParticleSystemConstants& cb = m_pConstants->GetDataForWriting();
  cb.SnapshotFraction = fSnapshotFraction;
  cb.NumUsedTrailPoints = iNumUsedTrailPoints;
}

plParticleRenderer::plParticleRenderer() = default;
plParticleRenderer::~plParticleRenderer() = default;

void plParticleRenderer::GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& categories) const
{
  categories.PushBack(plDefaultRenderDataCategories::LitTransparent);
}

void plParticleRenderer::CreateParticleDataBuffer(plGALBufferHandle& inout_hBuffer, plUInt32 uiDataTypeSize, plUInt32 uiNumParticlesPerBatch)
{
  if (inout_hBuffer.IsInvalidated())
  {
    plGALBufferCreationDescription desc;
    desc.m_uiStructSize = uiDataTypeSize;
    desc.m_uiTotalSize = uiNumParticlesPerBatch * desc.m_uiStructSize;
    desc.m_BufferType = plGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    inout_hBuffer = plGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }
}


void plParticleRenderer::DestroyParticleDataBuffer(plGALBufferHandle& inout_hBuffer)
{
  if (!inout_hBuffer.IsInvalidated())
  {
    plGALDevice::GetDefaultDevice()->DestroyBuffer(inout_hBuffer);
    inout_hBuffer.Invalidate();
  }
}

void plParticleRenderer::BindParticleShader(plRenderContext* pRenderContext, const char* szShader) const
{
  if (!m_hShader.IsValid())
  {
    // m_hShader = plResourceManager::LoadResource<plShaderResource>(szShader);
  }

  pRenderContext->BindShader(m_hShader);
}

PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Renderer_ParticleRenderer);

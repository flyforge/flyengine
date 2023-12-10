#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>
#include <ParticlePlugin/Type/Trail/TrailRenderer.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>

//clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTrailRenderData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTrailRenderer, 1, plRTTIDefaultAllocator<plParticleTrailRenderer>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleTrailRenderer::plParticleTrailRenderer()
{
  CreateParticleDataBuffer(m_hBaseDataBuffer, sizeof(plBaseParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hTrailDataBuffer, sizeof(plTrailParticleShaderData), s_uiParticlesPerBatch);

  // this is kinda stupid, apparently due to stride enforcement I cannot reuse the same buffer for different sizes
  // and instead have to create one buffer with every size ...

  CreateParticleDataBuffer(m_hTrailPointsDataBuffer8, sizeof(plTrailParticlePointsData8), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hTrailPointsDataBuffer16, sizeof(plTrailParticlePointsData16), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hTrailPointsDataBuffer32, sizeof(plTrailParticlePointsData32), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hTrailPointsDataBuffer64, sizeof(plTrailParticlePointsData64), s_uiParticlesPerBatch);

  m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Particles/Trail.plShader");
}

plParticleTrailRenderer::~plParticleTrailRenderer()
{
  DestroyParticleDataBuffer(m_hBaseDataBuffer);
  DestroyParticleDataBuffer(m_hTrailDataBuffer);
  DestroyParticleDataBuffer(m_hTrailPointsDataBuffer8);
  DestroyParticleDataBuffer(m_hTrailPointsDataBuffer16);
  DestroyParticleDataBuffer(m_hTrailPointsDataBuffer32);
  DestroyParticleDataBuffer(m_hTrailPointsDataBuffer64);
}

void plParticleTrailRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& types) const
{
  types.PushBack(plGetStaticRTTI<plParticleTrailRenderData>());
}

void plParticleTrailRenderer::RenderBatch(const plRenderViewContext& renderViewContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const
{
  plRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  plGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  TempSystemCB systemConstants(pRenderContext);

  pRenderContext->BindShader(m_hShader);

  // make sure our structured buffer is allocated and bound
  {
    pRenderContext->BindBuffer("particleBaseData", plGALDevice::GetDefaultDevice()->GetDefaultResourceView(m_hBaseDataBuffer));
    pRenderContext->BindBuffer("particleTrailData", plGALDevice::GetDefaultDevice()->GetDefaultResourceView(m_hTrailDataBuffer));
  }

  // now render all particle effects of type Trail
  for (auto it = batch.GetIterator<plParticleTrailRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const plParticleTrailRenderData* pRenderData = it;

    if (!ConfigureShader(pRenderData, renderViewContext))
      continue;

    const plUInt32 uiBucketSize = plParticleTypeTrail::ComputeTrailPointBucketSize(pRenderData->m_uiMaxTrailPoints);
    const plUInt32 uiMaxTrailSegments = uiBucketSize - 1;
    const plUInt32 uiPrimFactor = 2;
    const plUInt32 uiMaxPrimitivesToRender = s_uiParticlesPerBatch * uiMaxTrailSegments * uiPrimFactor;


    pRenderContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, uiMaxPrimitivesToRender);

    const plBaseParticleShaderData* pParticleBaseData = pRenderData->m_BaseParticleData.GetPtr();
    const plTrailParticleShaderData* pParticleTrailData = pRenderData->m_TrailParticleData.GetPtr();


    const plVec4* pParticlePointsData = pRenderData->m_TrailPointsShared.GetPtr();

    pRenderContext->BindTexture2D("ParticleTexture", pRenderData->m_hTexture);

    systemConstants.SetGenericData(
      pRenderData->m_bApplyObjectTransform, pRenderData->m_GlobalTransform, pRenderData->m_TotalEffectLifeTime, pRenderData->m_uiNumVariationsX, pRenderData->m_uiNumVariationsY, pRenderData->m_uiNumFlipbookAnimationsX, pRenderData->m_uiNumFlipbookAnimationsY, pRenderData->m_fDistortionStrength);
    systemConstants.SetTrailData(pRenderData->m_fSnapshotFraction, pRenderData->m_uiMaxTrailPoints);

    plUInt32 uiNumParticles = pRenderData->m_BaseParticleData.GetCount();
    while (uiNumParticles > 0)
    {
      // upload this batch of particle data
      const plUInt32 uiNumParticlesInBatch = plMath::Min<plUInt32>(uiNumParticles, s_uiParticlesPerBatch);
      uiNumParticles -= uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(m_hBaseDataBuffer, 0, plMakeArrayPtr(pParticleBaseData, uiNumParticlesInBatch).ToByteArray());
      pParticleBaseData += uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(m_hTrailDataBuffer, 0, plMakeArrayPtr(pParticleTrailData, uiNumParticlesInBatch).ToByteArray());
      pParticleTrailData += uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(m_hActiveTrailPointsDataBuffer, 0, plMakeArrayPtr(pParticlePointsData, uiNumParticlesInBatch * uiBucketSize).ToByteArray());
      pParticlePointsData += uiNumParticlesInBatch * uiBucketSize;

      // do one drawcall
      pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch * uiMaxTrailSegments * uiPrimFactor).IgnoreResult();
    }
  }
}

bool plParticleTrailRenderer::ConfigureShader(const plParticleTrailRenderData* pRenderData, const plRenderViewContext& renderViewContext) const
{
  auto pRenderContext = renderViewContext.m_pRenderContext;

  switch (pRenderData->m_RenderMode)
  {
    case plParticleTypeRenderMode::Additive:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_ADDITIVE");
      break;
    case plParticleTypeRenderMode::Blended:
    case plParticleTypeRenderMode::BlendedForeground:
    case plParticleTypeRenderMode::BlendedBackground:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_BLENDED");
      break;
    case plParticleTypeRenderMode::Opaque:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_OPAQUE");
      break;
    case plParticleTypeRenderMode::Distortion:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_DISTORTION");
      pRenderContext->BindTexture2D("ParticleDistortionTexture", pRenderData->m_hDistortionTexture);
      break;
    case plParticleTypeRenderMode::BlendAdd:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_BLENDADD");
      break;
  }

  switch (plParticleTypeTrail::ComputeTrailPointBucketSize(pRenderData->m_uiMaxTrailPoints))
  {
    case 8:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT8");
      m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer8;
      break;
    case 16:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT16");
      m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer16;
      break;
    case 32:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT32");
      m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer32;
      break;
    case 64:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT64");
      m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer64;
      break;

    default:
      return false;
  }

  renderViewContext.m_pRenderContext->BindBuffer("particlePointsData", plGALDevice::GetDefaultDevice()->GetDefaultResourceView(m_hActiveTrailPointsDataBuffer));
  return true;
}

PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Trail_TrailRenderer);

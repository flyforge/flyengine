#pragma once

#include <ParticlePlugin/ParticlePluginDLL.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/BillboardQuadParticleShaderData.h>

class PL_PARTICLEPLUGIN_DLL plParticlePointRenderData final : public plRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plParticlePointRenderData, plRenderData);

public:
  plArrayPtr<plBaseParticleShaderData> m_BaseParticleData;
  plArrayPtr<plBillboardQuadParticleShaderData> m_BillboardParticleData;
  bool m_bApplyObjectTransform = true;
  plTime m_TotalEffectLifeTime;
};

/// \brief Implements rendering of particle systems
class PL_PARTICLEPLUGIN_DLL plParticlePointRenderer final : public plParticleRenderer
{
  PL_ADD_DYNAMIC_REFLECTION(plParticlePointRenderer, plParticleRenderer);
  PL_DISALLOW_COPY_AND_ASSIGN(plParticlePointRenderer);

public:
  plParticlePointRenderer();
  ~plParticlePointRenderer();

  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const override;
  virtual void RenderBatch(
    const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;

protected:
  static const plUInt32 s_uiParticlesPerBatch = 1024;
  plGALBufferHandle m_hBaseDataBuffer;
  plGALBufferHandle m_hBillboardDataBuffer;
};

#pragma once

#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/TrailShaderData.h>

class PL_PARTICLEPLUGIN_DLL plParticleTrailRenderData final : public plRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleTrailRenderData, plRenderData);

public:
  plTexture2DResourceHandle m_hTexture;
  plUInt16 m_uiMaxTrailPoints;
  float m_fSnapshotFraction;
  plArrayPtr<plBaseParticleShaderData> m_BaseParticleData;
  plArrayPtr<plTrailParticleShaderData> m_TrailParticleData;
  plArrayPtr<plVec4> m_TrailPointsShared;
  plEnum<plParticleTypeRenderMode> m_RenderMode;
  bool m_bApplyObjectTransform = true;
  plTime m_TotalEffectLifeTime;
  plUInt8 m_uiNumVariationsX = 1;
  plUInt8 m_uiNumVariationsY = 1;
  plUInt8 m_uiNumFlipbookAnimationsX = 1;
  plUInt8 m_uiNumFlipbookAnimationsY = 1;
  plTexture2DResourceHandle m_hDistortionTexture;
  float m_fDistortionStrength = 0;
};

/// \brief Implements rendering of a trail particle systems
class PL_PARTICLEPLUGIN_DLL plParticleTrailRenderer final : public plParticleRenderer
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleTrailRenderer, plParticleRenderer);
  PL_DISALLOW_COPY_AND_ASSIGN(plParticleTrailRenderer);

public:
  plParticleTrailRenderer();
  ~plParticleTrailRenderer();

  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const override;
  virtual void RenderBatch(
    const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;

protected:
  bool ConfigureShader(const plParticleTrailRenderData* pRenderData, const plRenderViewContext& renderViewContext) const;

  static const plUInt32 s_uiParticlesPerBatch = 512;
  plGALBufferHandle m_hBaseDataBuffer;
  plGALBufferHandle m_hTrailDataBuffer;
  plGALBufferHandle m_hTrailPointsDataBuffer8;
  plGALBufferHandle m_hTrailPointsDataBuffer16;
  plGALBufferHandle m_hTrailPointsDataBuffer32;
  plGALBufferHandle m_hTrailPointsDataBuffer64;

  mutable plGALBufferHandle m_hActiveTrailPointsDataBuffer;
};

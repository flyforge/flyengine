#pragma once

#include <Foundation/Math/Color16f.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>

class plRenderContext;

/// \brief Implements rendering of particle systems
class PLASMA_PARTICLEPLUGIN_DLL plParticleRenderer : public plRenderer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleRenderer, plRenderer);
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plParticleRenderer);

public:
  plParticleRenderer();
  ~plParticleRenderer();

  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& categories) const override;

protected:
  struct TempSystemCB
  {
    TempSystemCB(plRenderContext* pRenderContext);
    ~TempSystemCB();

    void SetGenericData(bool bApplyObjectTransform, const plTransform& ObjectTransform, plTime effectLifeTime, plUInt8 uiNumVariationsX,
      plUInt8 uiNumVariationsY, plUInt8 uiNumFlipbookAnimsX, plUInt8 uiNumFlipbookAnimsY, float fDistortionStrength = 0);
    void SetTrailData(float fSnapshotFraction, plInt32 iNumUsedTrailPoints);

    plConstantBufferStorage<plParticleSystemConstants>* m_pConstants;
    plConstantBufferStorageHandle m_hConstantBuffer;
  };

  void CreateParticleDataBuffer(plGALBufferHandle& inout_hBuffer, plUInt32 uiDataTypeSize, plUInt32 uiNumParticlesPerBatch);
  void DestroyParticleDataBuffer(plGALBufferHandle& inout_hBuffer);
  void BindParticleShader(plRenderContext* pRenderContext, const char* szShader) const;

protected:
  plShaderResourceHandle m_hShader;
};

#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

struct PLASMA_RENDERERCORE_DLL plClusteredDataGPU
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plClusteredDataGPU);

public:
  plClusteredDataGPU();
  ~plClusteredDataGPU();

  plUInt32 m_uiSkyIrradianceIndex = 0;
  plEnum<plCameraUsageHint> m_cameraUsageHint = plCameraUsageHint::Default;

  plGALBufferHandle m_hLightDataBuffer;
  plGALBufferHandle m_hDecalDataBuffer;
  plGALBufferHandle m_hReflectionProbeDataBuffer;
  plGALBufferHandle m_hClusterDataBuffer;
  plGALBufferHandle m_hClusterItemBuffer;

  plConstantBufferStorageHandle m_hConstantBuffer;

  plGALSamplerStateHandle m_hShadowSampler;

  plDecalAtlasResourceHandle m_hDecalAtlas;
  plGALSamplerStateHandle m_hDecalAtlasSampler;

  void BindResources(plRenderContext* pRenderContext);
};

class PLASMA_RENDERERCORE_DLL plClusteredDataProvider : public plFrameDataProvider<plClusteredDataGPU>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plClusteredDataProvider, plFrameDataProviderBase);

public:
  plClusteredDataProvider();
  ~plClusteredDataProvider();

private:
  virtual void* UpdateData(const plRenderViewContext& renderViewContext, const plExtractedRenderData& extractedData) override;

  plClusteredDataGPU m_Data;
};

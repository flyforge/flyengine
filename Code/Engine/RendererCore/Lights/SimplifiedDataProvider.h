#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

struct PL_RENDERERCORE_DLL plSimplifiedDataGPU
{
  PL_DISALLOW_COPY_AND_ASSIGN(plSimplifiedDataGPU);

public:
  plSimplifiedDataGPU();
  ~plSimplifiedDataGPU();

  plUInt32 m_uiSkyIrradianceIndex = 0;
  plEnum<plCameraUsageHint> m_cameraUsageHint = plCameraUsageHint::Default;
  plConstantBufferStorageHandle m_hConstantBuffer;

  void BindResources(plRenderContext* pRenderContext);
};

class PL_RENDERERCORE_DLL plSimplifiedDataProvider : public plFrameDataProvider<plSimplifiedDataGPU>
{
  PL_ADD_DYNAMIC_REFLECTION(plSimplifiedDataProvider, plFrameDataProviderBase);

public:
  plSimplifiedDataProvider();
  ~plSimplifiedDataProvider();

private:
  virtual void* UpdateData(const plRenderViewContext& renderViewContext, const plExtractedRenderData& extractedData) override;

  plSimplifiedDataGPU m_Data;
};

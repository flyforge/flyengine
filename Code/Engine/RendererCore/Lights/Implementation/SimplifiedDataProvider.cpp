#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/SimplifiedDataExtractor.h>
#include <RendererCore/Lights/SimplifiedDataProvider.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightDataSimplified.h>
PLASMA_DEFINE_AS_POD_TYPE(plSimplifiedDataConstants);

plSimplifiedDataGPU::plSimplifiedDataGPU()
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plSimplifiedDataConstants>();
}

plSimplifiedDataGPU::~plSimplifiedDataGPU()
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void plSimplifiedDataGPU::BindResources(plRenderContext* pRenderContext)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  auto hReflectionSpecularTextureView = pDevice->GetDefaultResourceView(plReflectionPool::GetReflectionSpecularTexture(m_uiSkyIrradianceIndex, m_cameraUsageHint));
  auto hSkyIrradianceTextureView = pDevice->GetDefaultResourceView(plReflectionPool::GetSkyIrradianceTexture());

  pRenderContext->BindTextureCube("ReflectionSpecularTexture", hReflectionSpecularTextureView);
  pRenderContext->BindTexture2D("SkyIrradianceTexture", hSkyIrradianceTextureView);

  pRenderContext->BindConstantBuffer("plSimplifiedDataConstants", m_hConstantBuffer);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimplifiedDataProvider, 1, plRTTIDefaultAllocator<plSimplifiedDataProvider>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSimplifiedDataProvider::plSimplifiedDataProvider() {}

plSimplifiedDataProvider::~plSimplifiedDataProvider() {}

void* plSimplifiedDataProvider::UpdateData(const plRenderViewContext& renderViewContext, const plExtractedRenderData& extractedData)
{
  plGALCommandEncoder* pGALCommandEncoder = renderViewContext.m_pRenderContext->GetRenderCommandEncoder();

  PLASMA_PROFILE_AND_MARKER(pGALCommandEncoder, "Update Clustered Data");

  if (auto pData = extractedData.GetFrameData<plSimplifiedDataCPU>())
  {
    m_Data.m_uiSkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
    m_Data.m_cameraUsageHint = pData->m_cameraUsageHint;

    // Update Constants
    const plRectFloat& viewport = renderViewContext.m_pViewData->m_ViewPortRect;

    plSimplifiedDataConstants* pConstants =
      renderViewContext.m_pRenderContext->GetConstantBufferData<plSimplifiedDataConstants>(m_Data.m_hConstantBuffer);

    pConstants->SkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
  }

  return &m_Data;
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SimplifiedDataProvider);

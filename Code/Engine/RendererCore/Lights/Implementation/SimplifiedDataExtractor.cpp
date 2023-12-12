#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Lights/SimplifiedDataExtractor.h>
#include <RendererCore/Pipeline/View.h>

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimplifiedDataCPU, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plSimplifiedDataCPU::plSimplifiedDataCPU() = default;
plSimplifiedDataCPU::~plSimplifiedDataCPU() = default;

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimplifiedDataExtractor, 1, plRTTIDefaultAllocator<plSimplifiedDataExtractor>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plSimplifiedDataExtractor::plSimplifiedDataExtractor(const char* szName)
  : plExtractor(szName)
{
  m_DependsOn.PushBack(plMakeHashedString("plVisibleObjectsExtractor"));
}

plSimplifiedDataExtractor::~plSimplifiedDataExtractor() {}

void plSimplifiedDataExtractor::PostSortAndBatch(
  const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData)
{
  const plCamera* pCamera = view.GetCullingCamera();
  const float fAspectRatio = view.GetViewport().width / view.GetViewport().height;

  plSimplifiedDataCPU* pData = PLASMA_NEW(plFrameAllocator::GetCurrentAllocator(), plSimplifiedDataCPU);

  pData->m_uiSkyIrradianceIndex = view.GetWorld()->GetIndex();
  pData->m_cameraUsageHint = view.GetCameraUsageHint();

  ref_extractedRenderData.AddFrameData(pData);
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SimplifiedDataExtractor);

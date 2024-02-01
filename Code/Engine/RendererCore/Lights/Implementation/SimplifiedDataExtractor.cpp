#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Lights/SimplifiedDataExtractor.h>
#include <RendererCore/Pipeline/View.h>

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimplifiedDataCPU, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plSimplifiedDataCPU::plSimplifiedDataCPU() = default;
plSimplifiedDataCPU::~plSimplifiedDataCPU() = default;

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimplifiedDataExtractor, 1, plRTTIDefaultAllocator<plSimplifiedDataExtractor>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plSimplifiedDataExtractor::plSimplifiedDataExtractor(const char* szName)
  : plExtractor(szName)
{
  m_DependsOn.PushBack(plMakeHashedString("plVisibleObjectsExtractor"));
}

plSimplifiedDataExtractor::~plSimplifiedDataExtractor() = default;

void plSimplifiedDataExtractor::PostSortAndBatch(
  const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData)
{
  plSimplifiedDataCPU* pData = PL_NEW(plFrameAllocator::GetCurrentAllocator(), plSimplifiedDataCPU);

  pData->m_uiSkyIrradianceIndex = view.GetWorld()->GetIndex();
  pData->m_cameraUsageHint = view.GetCameraUsageHint();

  ref_extractedRenderData.AddFrameData(pData);
}

plResult plSimplifiedDataExtractor::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return PL_SUCCESS;
}

plResult plSimplifiedDataExtractor::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);
  return PL_SUCCESS;
}

PL_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SimplifiedDataExtractor);

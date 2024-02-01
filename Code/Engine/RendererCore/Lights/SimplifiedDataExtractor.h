#pragma once

#include <RendererCore/Pipeline/Extractor.h>

class plSimplifiedDataCPU : public plRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plSimplifiedDataCPU, plRenderData);

public:
  plSimplifiedDataCPU();
  ~plSimplifiedDataCPU();

  plUInt32 m_uiSkyIrradianceIndex = 0;
  plEnum<plCameraUsageHint> m_cameraUsageHint = plCameraUsageHint::Default;
};

class PL_RENDERERCORE_DLL plSimplifiedDataExtractor : public plExtractor
{
  PL_ADD_DYNAMIC_REFLECTION(plSimplifiedDataExtractor, plExtractor);

public:
  plSimplifiedDataExtractor(const char* szName = "SimplifiedDataExtractor");
  ~plSimplifiedDataExtractor();

  virtual void PostSortAndBatch(
    const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData) override;
  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;
};

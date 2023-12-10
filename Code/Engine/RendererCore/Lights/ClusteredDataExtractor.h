#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/Extractor.h>

struct plPerLightData;
struct plPerDecalData;
struct plPerReflectionProbeData;
struct plPerClusterData;

class plClusteredDataCPU : public plRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plClusteredDataCPU, plRenderData);

public:
  plClusteredDataCPU();
  ~plClusteredDataCPU();

  enum
  {
    MAX_LIGHT_DATA = 1024,
    MAX_DECAL_DATA = 1024,
    MAX_REFLECTION_PROBE_DATA = 1024,
    MAX_ITEMS_PER_CLUSTER = 256
  };

  plArrayPtr<plPerLightData> m_LightData;
  plArrayPtr<plPerDecalData> m_DecalData;
  plArrayPtr<plPerReflectionProbeData> m_ReflectionProbeData;
  plArrayPtr<plPerClusterData> m_ClusterData;
  plArrayPtr<plUInt32> m_ClusterItemList;

  plUInt32 m_uiSkyIrradianceIndex = 0;
  plEnum<plCameraUsageHint> m_cameraUsageHint = plCameraUsageHint::Default;

  float m_fFogHeight = 0.0f;
  float m_fFogHeightFalloff = 0.0f;
  float m_fFogDensityAtCameraPos = 0.0f;
  float m_fFogDensity = 0.0f;
  float m_fFogInvSkyDistance = 0.0f;
  plColor m_FogColor = plColor::Black;
};

class PLASMA_RENDERERCORE_DLL plClusteredDataExtractor : public plExtractor
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plClusteredDataExtractor, plExtractor);

public:
  plClusteredDataExtractor(const char* szName = "ClusteredDataExtractor");
  ~plClusteredDataExtractor();

  virtual void PostSortAndBatch(
    const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData) override;
  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

private:
  void FillItemListAndClusterData(plClusteredDataCPU* pData);

  template <plUInt32 MaxData>
  struct TempCluster
  {
    PLASMA_DECLARE_POD_TYPE();

    plUInt32 m_BitMask[MaxData / 32];
  };

  plDynamicArray<plPerLightData, plAlignedAllocatorWrapper> m_TempLightData;
  plDynamicArray<plPerDecalData, plAlignedAllocatorWrapper> m_TempDecalData;
  plDynamicArray<plPerReflectionProbeData, plAlignedAllocatorWrapper> m_TempReflectionProbeData;
  plDynamicArray<TempCluster<plClusteredDataCPU::MAX_LIGHT_DATA>> m_TempLightsClusters;
  plDynamicArray<TempCluster<plClusteredDataCPU::MAX_DECAL_DATA>> m_TempDecalsClusters;
  plDynamicArray<TempCluster<plClusteredDataCPU::MAX_REFLECTION_PROBE_DATA>> m_TempReflectionProbeClusters;
  plDynamicArray<plUInt32> m_TempClusterItemList;

  plDynamicArray<plSimdBSphere, plAlignedAllocatorWrapper> m_ClusterBoundingSpheres;
};

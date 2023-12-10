#pragma once

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Types/Bitflags.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeMapping.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeUpdater.h>
#include <RendererCore/Pipeline/View.h>

class plSkyLightComponent;
class plSphereReflectionProbeComponent;
class plBoxReflectionProbeComponent;

static const plUInt32 s_uiReflectionCubeMapSize = 128;
static const plUInt32 s_uiNumReflectionProbeCubeMaps = 32;
static const float s_fDebugSphereRadius = 0.3f;

inline plUInt32 GetMipLevels()
{
  return plMath::Log2i(s_uiReflectionCubeMapSize) - 1; // only down to 4x4
}

//////////////////////////////////////////////////////////////////////////
/// plReflectionPool::Data

struct plReflectionPool::Data
{
  Data();
  ~Data();

  struct ProbeData
  {
    plReflectionProbeDesc m_desc;
    plTransform m_GlobalTransform;
    plBitflags<plProbeFlags> m_Flags;
    plTextureCubeResourceHandle m_hCubeMap; // static data or empty for dynamic.
  };

  struct WorldReflectionData
  {
    WorldReflectionData()
      : m_mapping(s_uiNumReflectionProbeCubeMaps)
    {
    }
    PLASMA_DISALLOW_COPY_AND_ASSIGN(WorldReflectionData);

    plIdTable<plReflectionProbeId, ProbeData> m_Probes;
    plReflectionProbeId m_SkyLight; // SkyLight is always fixed at reflectionIndex 0.
    plEventSubscriptionID m_mappingSubscriptionId = 0;
    plReflectionProbeMapping m_mapping;
  };

  // WorldReflectionData management
  plReflectionProbeId AddProbe(const plWorld* pWorld, ProbeData&& probeData);
  plReflectionPool::Data::WorldReflectionData& GetWorldData(const plWorld* pWorld);
  void RemoveProbe(const plWorld* pWorld, plReflectionProbeId id);
  void UpdateProbeData(ProbeData& ref_probeData, const plReflectionProbeDesc& desc, const plReflectionProbeComponentBase* pComponent);
  bool UpdateSkyLightData(ProbeData& ref_probeData, const plReflectionProbeDesc& desc, const plSkyLightComponent* pComponent);
  void OnReflectionProbeMappingEvent(const plUInt32 uiWorldIndex, const plReflectionProbeMappingEvent& e);

  void PreExtraction();
  void PostExtraction();

  // Dynamic Update Queue (all worlds combined)
  plHashSet<plReflectionProbeRef> m_PendingDynamicUpdate;
  plDeque<plReflectionProbeRef> m_DynamicUpdateQueue;

  plHashSet<plReflectionProbeRef> m_ActiveDynamicUpdate;
  plReflectionProbeUpdater m_ReflectionProbeUpdater;

  void CreateReflectionViewsAndResources();
  void CreateSkyIrradianceTexture();

  plMutex m_Mutex;
  plUInt64 m_uiWorldHasSkyLight = 0;
  plUInt64 m_uiSkyIrradianceChanged = 0;
  plHybridArray<plUniquePtr<WorldReflectionData>, 2> m_WorldReflectionData;

  // GPU storage
  plGALTextureHandle m_hFallbackReflectionSpecularTexture;
  plGALTextureHandle m_hSkyIrradianceTexture;
  plHybridArray<plAmbientCube<plColorLinear16f>, 64> m_SkyIrradianceStorage;

  // Debug data
  plMeshResourceHandle m_hDebugSphere;
  plHybridArray<plMaterialResourceHandle, 6 * s_uiNumReflectionProbeCubeMaps> m_hDebugMaterial;
};

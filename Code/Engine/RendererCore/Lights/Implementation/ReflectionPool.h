#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>

class plGALTextureHandle;
class plGALBufferHandle;
class plView;
class plWorld;
class plComponent;
struct plRenderWorldExtractionEvent;
struct plRenderWorldRenderEvent;
struct plMsgExtractRenderData;
struct plReflectionProbeDesc;
class plReflectionProbeRenderData;
typedef plGenericId<24, 8> plReflectionProbeId;
class plReflectionProbeComponentBase;
class plSkyLightComponent;

class PLASMA_RENDERERCORE_DLL plReflectionPool
{
public:
  //Probes
  static plReflectionProbeId RegisterReflectionProbe(const plWorld* pWorld, const plReflectionProbeDesc& desc, const plReflectionProbeComponentBase* pComponent);
  static void DeregisterReflectionProbe(const plWorld* pWorld, plReflectionProbeId id);
  static void UpdateReflectionProbe(const plWorld* pWorld, plReflectionProbeId id, const plReflectionProbeDesc& desc, const plReflectionProbeComponentBase* pComponent);
  static void ExtractReflectionProbe(const plComponent* pComponent, plMsgExtractRenderData& ref_msg, plReflectionProbeRenderData* pRenderData, const plWorld* pWorld, plReflectionProbeId id, float fPriority);

  // SkyLight
  static plReflectionProbeId RegisterSkyLight(const plWorld* pWorld, plReflectionProbeDesc& ref_desc, const plSkyLightComponent* pComponent);
  static void DeregisterSkyLight(const plWorld* pWorld, plReflectionProbeId id);
  static void UpdateSkyLight(const plWorld* pWorld, plReflectionProbeId id, const plReflectionProbeDesc& desc, const plSkyLightComponent* pComponent);


  static void SetConstantSkyIrradiance(const plWorld* pWorld, const plAmbientCube<plColor>& skyIrradiance);
  static void ResetConstantSkyIrradiance(const plWorld* pWorld);

  static plUInt32 GetReflectionCubeMapSize();
  static plGALTextureHandle GetReflectionSpecularTexture(plUInt32 uiWorldIndex, plEnum<plCameraUsageHint> cameraUsageHint);
  static plGALTextureHandle GetSkyIrradianceTexture();

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, ReflectionPool);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnExtractionEvent(const plRenderWorldExtractionEvent& e);
  static void OnRenderEvent(const plRenderWorldRenderEvent& e);

  struct Data;
  static Data* s_pData;
};

#pragma once

#include <RendererCore/Declarations.h>

class plDirectionalLightComponent;
class plPointLightComponent;
class plSpotLightComponent;
class plGALTextureHandle;
class plGALBufferHandle;
class plView;
struct plRenderWorldExtractionEvent;
struct plRenderWorldRenderEvent;

class PL_RENDERERCORE_DLL plShadowPool
{
public:
  static plUInt32 AddDirectionalLight(const plDirectionalLightComponent* pDirLight, const plView* pReferenceView);
  static plUInt32 AddPointLight(const plPointLightComponent* pPointLight, float fScreenSpaceSize, const plView* pReferenceView);
  static plUInt32 AddSpotLight(const plSpotLightComponent* pSpotLight, float fScreenSpaceSize, const plView* pReferenceView);

  static plGALTextureHandle GetShadowAtlasTexture();
  static plGALBufferHandle GetShadowDataBuffer();

  /// \brief All exclude tags on this white list are copied from the reference views to the shadow views.
  static void AddExcludeTagToWhiteList(const plTag& tag);

private:
  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, ShadowPool);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnExtractionEvent(const plRenderWorldExtractionEvent& e);
  static void OnRenderEvent(const plRenderWorldRenderEvent& e);

  struct Data;
  static Data* s_pData;
};

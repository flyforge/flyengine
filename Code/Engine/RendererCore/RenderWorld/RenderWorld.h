#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Declarations.h>

using plRenderPipelineResourceHandle = plTypedResourceHandle<class plRenderPipelineResource>;

struct plRenderWorldExtractionEvent
{
  enum class Type
  {
    BeginExtraction,
    BeforeViewExtraction,
    AfterViewExtraction,
    EndExtraction
  };

  Type m_Type;
  plView* m_pView = nullptr;
  plUInt64 m_uiFrameCounter = 0;
};

struct plRenderWorldRenderEvent
{
  enum class Type
  {
    BeginRender,
    BeforePipelineExecution,
    AfterPipelineExecution,
    EndRender,
  };

  Type m_Type;
  plRenderPipeline* m_pPipeline = nullptr;
  const plRenderViewContext* m_pRenderViewContext = nullptr;
  plUInt64 m_uiFrameCounter = 0;
};

class PLASMA_RENDERERCORE_DLL plRenderWorld
{
public:
  static plViewHandle CreateView(const char* szName, plView*& out_pView);
  static void DeleteView(const plViewHandle& hView);

  static bool TryGetView(const plViewHandle& hView, plView*& out_pView);
  static plView* GetViewByUsageHint(plCameraUsageHint::Enum usageHint, plCameraUsageHint::Enum alternativeUsageHint = plCameraUsageHint::None, const plWorld* pWorld = nullptr);

  static void AddMainView(const plViewHandle& hView);
  static void RemoveMainView(const plViewHandle& hView);
  static void ClearMainViews();
  static plArrayPtr<plViewHandle> GetMainViews();

  static void CacheRenderData(const plView& view, const plGameObjectHandle& hOwnerObject, const plComponentHandle& hOwnerComponent, plUInt16 uiComponentVersion, plArrayPtr<plInternal::RenderDataCacheEntry> cacheEntries);

  static void DeleteAllCachedRenderData();
  static void DeleteCachedRenderData(const plGameObjectHandle& hOwnerObject, const plComponentHandle& hOwnerComponent);
  static void DeleteCachedRenderDataForObject(const plGameObject* pOwnerObject);
  static void DeleteCachedRenderDataForObjectRecursive(const plGameObject* pOwnerObject);
  static void ResetRenderDataCache(plView& ref_view);
  static plArrayPtr<const plInternal::RenderDataCacheEntry> GetCachedRenderData(const plView& view, const plGameObjectHandle& hOwner, plUInt16 uiComponentVersion);

  static void AddViewToRender(const plViewHandle& hView);

  static void ExtractMainViews();

  static void Render(plRenderContext* pRenderContext);

  static void BeginFrame();
  static void EndFrame();

  static plEvent<plView*, plMutex> s_ViewCreatedEvent;
  static plEvent<plView*, plMutex> s_ViewDeletedEvent;

  static const plEvent<const plRenderWorldExtractionEvent&, plMutex>& GetExtractionEvent() { return s_ExtractionEvent; }
  static const plEvent<const plRenderWorldRenderEvent&, plMutex>& GetRenderEvent() { return s_RenderEvent; }

  static bool GetUseMultithreadedRendering();

  PLASMA_ALWAYS_INLINE static void SetOverridePipeline(bool bEnabled) { s_bOverridePipelineSettings = bEnabled; }
  PLASMA_ALWAYS_INLINE static bool GetOverridePipeline() { return s_bOverridePipelineSettings; }

  PLASMA_ALWAYS_INLINE static void SetTAAEnabled(bool bEnabled) { s_bTAAEnabled = bEnabled; }
  PLASMA_ALWAYS_INLINE static bool GetTAAEnabled() { return s_bTAAEnabled; }

  PLASMA_ALWAYS_INLINE static void SetTAAUpscaleEnabled(bool bEnabled) { s_bTAAUpscaleEnabled = bEnabled; }
  PLASMA_ALWAYS_INLINE static bool GetTAAUpscaleEnabled() { return s_bTAAUpscaleEnabled; }

  PLASMA_ALWAYS_INLINE static void SetDOFEnabled(bool bEnabled) { s_bDOFEnabled = bEnabled; }
  PLASMA_ALWAYS_INLINE static bool GetDOFEnabled() { return s_bDOFEnabled; }

  PLASMA_ALWAYS_INLINE static void SetDOFRadius(float radius) { s_bDOFRadius = radius; }
  PLASMA_ALWAYS_INLINE static float GetDOFRadius() { return s_bDOFRadius; }

  PLASMA_ALWAYS_INLINE static void SetMotionBlurEnabled(bool bEnabled) { s_bMotionBlurEnabled = bEnabled; }
  PLASMA_ALWAYS_INLINE static bool GetMotionBlurEnabled() { return s_bMotionBlurEnabled; }

  PLASMA_ALWAYS_INLINE static void SetMotionBlurSamples(float samples) { s_MotionBlurSamples = samples; }
  PLASMA_ALWAYS_INLINE static float GetMotionBlurSamples() { return s_MotionBlurSamples; }

  PLASMA_ALWAYS_INLINE static void SetMotionBlurStrength(float strength) { s_MotionBlurStrength = strength; }
  PLASMA_ALWAYS_INLINE static float GetMotionBlurStrength() { return s_MotionBlurStrength; }

  PLASMA_ALWAYS_INLINE static void SetMotionBlurMode(const plEnum<plMotionBlurMode>&  mode) { s_eMotionBlurMode = mode; }
  PLASMA_ALWAYS_INLINE static plEnum<plMotionBlurMode>  GetMotionBlurMode() { return s_eMotionBlurMode; }

  PLASMA_ALWAYS_INLINE static void SetBloomEnabled(bool bEnabled) { s_bBloomEnabled = bEnabled; }
  PLASMA_ALWAYS_INLINE static bool GetBloomEnabled() { return s_bBloomEnabled; }

  PLASMA_ALWAYS_INLINE static void SetBloomThreshold(float threshold) { s_BloomThreshold = threshold; }
  PLASMA_ALWAYS_INLINE static float GetBloomThreshold() { return s_BloomThreshold; }

  PLASMA_ALWAYS_INLINE static void SetBloomIntensity(float intensity) { s_BloomIntensity = intensity; }
  PLASMA_ALWAYS_INLINE static float GetBloomIntensity() { return s_BloomIntensity; }

  PLASMA_ALWAYS_INLINE static void SetBloomMipCount(int mips) { s_BloomMipCount = mips; }
  PLASMA_ALWAYS_INLINE static int GetBloomMipCount() { return s_BloomMipCount; }

  PLASMA_ALWAYS_INLINE static void SetTonemapMode(const plEnum<plTonemapMode>& tonemapMode) { s_eTonemapMode = tonemapMode; }
  PLASMA_ALWAYS_INLINE static plEnum<plTonemapMode> GetTonemapMode() { return s_eTonemapMode; }

  /// \brief Resets the frame counter to zero. Only for test purposes !
  PLASMA_ALWAYS_INLINE static void ResetFrameCounter() { s_uiFrameCounter = 0; }

  PLASMA_ALWAYS_INLINE static plUInt64 GetFrameCounter() { return s_uiFrameCounter; }

  PLASMA_FORCE_INLINE static plUInt32 GetDataIndexForExtraction() { return GetUseMultithreadedRendering() ? (s_uiFrameCounter & 1) : 0; }

  PLASMA_FORCE_INLINE static plUInt32 GetDataIndexForRendering() { return GetUseMultithreadedRendering() ? ((s_uiFrameCounter + 1) & 1) : 0; }

  static bool IsRenderingThread();

  /// \name Render To Texture
  /// @{
public:
  struct CameraConfig
  {
    plRenderPipelineResourceHandle m_hRenderPipeline;
  };

  static void BeginModifyCameraConfigs();
  static void EndModifyCameraConfigs();
  static void ClearCameraConfigs();
  static void SetCameraConfig(const char* szName, const CameraConfig& config);
  static const CameraConfig* FindCameraConfig(const char* szName);

  static plEvent<void*> s_CameraConfigsModifiedEvent;

private:
  static bool s_bModifyingCameraConfigs;
  static plMap<plString, CameraConfig> s_CameraConfigs;

  /// @}

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RenderWorld);
  friend class plView;
  friend class plRenderPipeline;

  static void DeleteCachedRenderDataInternal(const plGameObjectHandle& hOwnerObject);
  static void ClearRenderDataCache();
  static void UpdateRenderDataCache();

  static void AddRenderPipelineToRebuild(plRenderPipeline* pRenderPipeline, const plViewHandle& hView);
  static void RebuildPipelines();

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static plEvent<const plRenderWorldExtractionEvent&, plMutex> s_ExtractionEvent;
  static plEvent<const plRenderWorldRenderEvent&, plMutex> s_RenderEvent;
  static plUInt64 s_uiFrameCounter;

  static bool s_bOverridePipelineSettings;

  // TAA
  static bool s_bTAAEnabled;
  static bool s_bTAAUpscaleEnabled;

  // Depth of field
  static bool s_bDOFEnabled;
  static float s_bDOFRadius;

  // Motion blur
  static bool s_bMotionBlurEnabled;
  static float s_MotionBlurSamples;
  static float s_MotionBlurStrength;
  static plEnum<plMotionBlurMode> s_eMotionBlurMode;

  // Bloom
  static bool s_bBloomEnabled;
  static float s_BloomThreshold;
  static float s_BloomIntensity;
  static int s_BloomMipCount;

  // Tonemapping
  static plEnum<plTonemapMode> s_eTonemapMode;
};

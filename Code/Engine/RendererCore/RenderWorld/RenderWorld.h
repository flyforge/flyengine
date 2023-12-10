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
};

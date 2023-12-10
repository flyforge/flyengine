#pragma once

#include <Core/Graphics/Camera.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>

class PLASMA_RENDERERCORE_DLL plExtractedRenderData
{
public:
  plExtractedRenderData();

  PLASMA_ALWAYS_INLINE void SetCamera(const plCamera& camera) { m_Camera = camera; }
  PLASMA_ALWAYS_INLINE const plCamera& GetCamera() const { return m_Camera; }

  PLASMA_ALWAYS_INLINE void SetLodCamera(const plCamera& camera) { m_LodCamera = camera; }
  PLASMA_ALWAYS_INLINE const plCamera& GetLodCamera() const { return m_LodCamera; }

  PLASMA_ALWAYS_INLINE void SetViewData(const plViewData& viewData) { m_ViewData = viewData; }
  PLASMA_ALWAYS_INLINE const plViewData& GetViewData() const { return m_ViewData; }

  PLASMA_ALWAYS_INLINE void SetWorldTime(plTime time) { m_WorldTime = time; }
  PLASMA_ALWAYS_INLINE plTime GetWorldTime() const { return m_WorldTime; }

  PLASMA_ALWAYS_INLINE void SetWorldDebugContext(const plDebugRendererContext& debugContext) { m_WorldDebugContext = debugContext; }
  PLASMA_ALWAYS_INLINE const plDebugRendererContext& GetWorldDebugContext() const { return m_WorldDebugContext; }

  PLASMA_ALWAYS_INLINE void SetViewDebugContext(const plDebugRendererContext& debugContext) { m_ViewDebugContext = debugContext; }
  PLASMA_ALWAYS_INLINE const plDebugRendererContext& GetViewDebugContext() const { return m_ViewDebugContext; }

  void AddRenderData(const plRenderData* pRenderData, plRenderData::Category category);
  void AddFrameData(const plRenderData* pFrameData);

  void SortAndBatch();

  void Clear();

  plRenderDataBatchList GetRenderDataBatchesWithCategory(
    plRenderData::Category category, plRenderDataBatch::Filter filter = plRenderDataBatch::Filter()) const;

  template <typename T>
  PLASMA_ALWAYS_INLINE const T* GetFrameData() const
  {
    return static_cast<const T*>(GetFrameData(plGetStaticRTTI<T>()));
  }

private:
  const plRenderData* GetFrameData(const plRTTI* pRtti) const;

  struct DataPerCategory
  {
    plDynamicArray<plRenderDataBatch> m_Batches;
    plDynamicArray<plRenderDataBatch::SortableRenderData> m_SortableRenderData;
  };

  plCamera m_Camera;
  plCamera m_LodCamera; // Temporary until we have a real LOD system
  plViewData m_ViewData;
  plTime m_WorldTime;

  plDebugRendererContext m_WorldDebugContext;
  plDebugRendererContext m_ViewDebugContext;

  plHybridArray<DataPerCategory, 16> m_DataPerCategory;
  plHybridArray<const plRenderData*, 16> m_FrameData;
};

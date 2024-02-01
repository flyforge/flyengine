#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class plCamera;
class plExtractedRenderData;
class plExtractor;
class plView;
class plRenderer;
class plRenderData;
class plRenderDataBatch;
class plRenderPipeline;
class plRenderPipelinePass;
class plRenderContext;
class plDebugRendererContext;

struct plRenderPipelineNodePin;
struct plRenderPipelinePassConnection;
struct plViewData;

namespace plInternal
{
  struct RenderDataCache;

  struct RenderDataCacheEntry
  {
    PL_DECLARE_POD_TYPE();

    const plRenderData* m_pRenderData = nullptr;
    plUInt16 m_uiCategory = 0;
    plUInt16 m_uiComponentIndex = 0;
    plUInt16 m_uiPartIndex = 0;

    PL_ALWAYS_INLINE bool operator==(const RenderDataCacheEntry& other) const { return m_pRenderData == other.m_pRenderData && m_uiCategory == other.m_uiCategory && m_uiComponentIndex == other.m_uiComponentIndex && m_uiPartIndex == other.m_uiPartIndex; }

    // Cache entries need to be sorted by component index and then by part index
    PL_ALWAYS_INLINE bool operator<(const RenderDataCacheEntry& other) const
    {
      if (m_uiComponentIndex == other.m_uiComponentIndex)
        return m_uiPartIndex < other.m_uiPartIndex;

      return m_uiComponentIndex < other.m_uiComponentIndex;
    }
  };
} // namespace plInternal

struct plRenderViewContext
{
  const plCamera* m_pCamera;
  const plCamera* m_pLodCamera;
  const plViewData* m_pViewData;
  plRenderContext* m_pRenderContext;

  const plDebugRendererContext* m_pWorldDebugContext;
  const plDebugRendererContext* m_pViewDebugContext;
};

using plViewId = plGenericId<24, 8>;

class plViewHandle
{
  PL_DECLARE_HANDLE_TYPE(plViewHandle, plViewId);

  friend class plRenderWorld;
};

/// \brief HashHelper implementation so view handles can be used as key in a hashtable.
template <>
struct plHashHelper<plViewHandle>
{
  PL_ALWAYS_INLINE static plUInt32 Hash(plViewHandle value) { return value.GetInternalID().m_Data * 2654435761U; }

  PL_ALWAYS_INLINE static bool Equal(plViewHandle a, plViewHandle b) { return a == b; }
};

/// \brief Usage hint of a camera/view.
struct PL_RENDERERCORE_DLL plCameraUsageHint
{
  using StorageType = plUInt8;

  enum Enum
  {
    None,         ///< No hint, camera may not be used, at all.
    MainView,     ///< The main camera from which the scene gets rendered. There should only be one camera with this hint.
    EditorView,   ///< The editor view shall be rendered from this camera.
    RenderTarget, ///< The camera is used to render to a render target.
    Culling,      ///< This camera should be used for culling only. Usually culling is done from the main view, but with a dedicated culling camera, one can debug the culling system.
    Shadow,       ///< This camera is used for rendering shadow maps.
    Reflection,   ///< This camera is used for rendering reflections.
    Thumbnail,    ///< This camera should be used for rendering a scene thumbnail when exporting from the editor.

    ENUM_COUNT,

    Default = None,
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plCameraUsageHint);

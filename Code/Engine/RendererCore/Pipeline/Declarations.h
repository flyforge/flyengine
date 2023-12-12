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
    PLASMA_DECLARE_POD_TYPE();

    const plRenderData* m_pRenderData = nullptr;
    plUInt16 m_uiCategory = 0;
    plUInt16 m_uiComponentIndex = 0;
    plUInt16 m_uiPartIndex = 0;

    PLASMA_ALWAYS_INLINE bool operator==(const RenderDataCacheEntry& other) const { return m_pRenderData == other.m_pRenderData && m_uiCategory == other.m_uiCategory && m_uiComponentIndex == other.m_uiComponentIndex && m_uiPartIndex == other.m_uiPartIndex; }

    // Cache entries need to be sorted by component index and then by part index
    PLASMA_ALWAYS_INLINE bool operator<(const RenderDataCacheEntry& other) const
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

typedef plGenericId<24, 8> plViewId;

class plViewHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plViewHandle, plViewId);

  friend class plRenderWorld;
};

/// \brief HashHelper implementation so view handles can be used as key in a hashtable.
template <>
struct plHashHelper<plViewHandle>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(plViewHandle value) { return value.GetInternalID().m_Data * 2654435761U; }

  PLASMA_ALWAYS_INLINE static bool Equal(plViewHandle a, plViewHandle b) { return a == b; }
};

/// \brief Usage hint of a camera/view.
struct PLASMA_RENDERERCORE_DLL plCameraUsageHint
{
  typedef plUInt8 StorageType;

  enum Enum
  {
    None,
    MainView,
    EditorView,
    RenderTarget,
    Culling,
    Shadow,
    Reflection,
    Thumbnail,

    ENUM_COUNT,

    Default = None,
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plCameraUsageHint);

struct PLASMA_RENDERERCORE_DLL plTonemapMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    AMD = 0,
    ACES,
    Reinhard,
    Uncharted2,
    None,

    Default = AMD
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plTonemapMode);

struct plMotionBlurMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    /// @brief Applies motion blur on objects when they are moving and on camera movements.
    /// This won't affect the skybox since it's always static in the scene.
    /// @note This mode requires the Velocity texture as input.
    ObjectBased,

    /// @brief Applies motion blur on the whole screen only on camera movements.
    /// This will affect the skybox too.
    /// @note This mode requires the Depth texture as input.
    ScreenBased,

    Default = ObjectBased,
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plMotionBlurMode);
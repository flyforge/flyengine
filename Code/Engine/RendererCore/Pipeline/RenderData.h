#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Pipeline/Declarations.h>

class plRasterizerObject;

/// \brief Base class for all render data. Render data must contain all information that is needed to render the corresponding object.
class PL_RENDERERCORE_DLL plRenderData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plRenderData, plReflectedClass);

public:
  struct Category
  {
    Category();
    explicit Category(plUInt16 uiValue);

    bool operator==(const Category& other) const;
    bool operator!=(const Category& other) const;
    bool IsValid() const { return m_uiValue != 0xFFFF; }

    plUInt16 m_uiValue = 0xFFFF;
  };

  struct Caching
  {
    enum Enum
    {
      Never,
      IfStatic
    };
  };

  /// \brief This function generates a 64bit sorting key for the given render data. Data with lower sorting key is rendered first.
  using SortingKeyFunc = plUInt64 (*)(const plRenderData*, const plCamera&);

  static Category RegisterCategory(const char* szCategoryName, SortingKeyFunc sortingKeyFunc);
  static Category FindCategory(plTempHashedString sCategoryName);

  static void GetAllCategoryNames(plDynamicArray<plHashedString>& out_categoryNames);

  static const plRenderer* GetCategoryRenderer(Category category, const plRTTI* pRenderDataType);

  static plHashedString GetCategoryName(Category category);

  plUInt64 GetCategorySortingKey(Category category, const plCamera& camera) const;

  plTransform m_LastGlobalTransform = plTransform::MakeIdentity();
  plTransform m_GlobalTransform = plTransform::MakeIdentity();
  plBoundingBoxSphere m_GlobalBounds;

  plUInt32 m_uiBatchId = 0; ///< BatchId is used to group render data in batches.
  plUInt32 m_uiSortingKey = 0;
  float m_fSortingDepthOffset = 0.0f;

  plGameObjectHandle m_hOwner;

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  const plGameObject* m_pOwner = nullptr; ///< Debugging only. It is not allowed to access the game object during rendering.
#endif

private:
  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RenderData);

  static void PluginEventHandler(const plPluginEvent& e);
  static void UpdateRendererTypes();

  static void CreateRendererInstances();
  static void ClearRendererInstances();

  struct CategoryData
  {
    plHashedString m_sName;
    SortingKeyFunc m_sortingKeyFunc;

    plHashTable<const plRTTI*, plUInt32> m_TypeToRendererIndex;
  };

  static plHybridArray<CategoryData, 32> s_CategoryData;

  static plHybridArray<const plRTTI*, 16> s_RendererTypes;
  static plDynamicArray<plUniquePtr<plRenderer>> s_RendererInstances;
  static bool s_bRendererInstancesDirty;
};

/// \brief Creates render data that is only valid for this frame. The data is automatically deleted after the frame has been rendered.
template <typename T>
static T* plCreateRenderDataForThisFrame(const plGameObject* pOwner);

struct PL_RENDERERCORE_DLL plDefaultRenderDataCategories
{
  static plRenderData::Category Light;
  static plRenderData::Category Decal;
  static plRenderData::Category ReflectionProbe;
  static plRenderData::Category Sky;
  static plRenderData::Category LitOpaque;
  static plRenderData::Category LitMasked;
  static plRenderData::Category LitTransparent;
  static plRenderData::Category LitForeground;
  static plRenderData::Category LitScreenFX;
  static plRenderData::Category SimpleOpaque;
  static plRenderData::Category SimpleTransparent;
  static plRenderData::Category SimpleForeground;
  static plRenderData::Category Selection;
  static plRenderData::Category GUI;
};

#define plInvalidRenderDataCategory plRenderData::Category()

struct PL_RENDERERCORE_DLL plMsgExtractRenderData : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgExtractRenderData, plMessage);

  const plView* m_pView = nullptr;
  plRenderData::Category m_OverrideCategory = plInvalidRenderDataCategory;

  /// \brief Adds render data for the current view. This data can be cached depending on the specified caching behavior.
  /// Non-cached data is only valid for this frame. Cached data must be manually deleted using the plRenderWorld::DeleteCachedRenderData
  /// function.
  void AddRenderData(const plRenderData* pRenderData, plRenderData::Category category, plRenderData::Caching::Enum cachingBehavior);

private:
  friend class plExtractor;

  struct Data
  {
    const plRenderData* m_pRenderData = nullptr;
    plUInt16 m_uiCategory = 0;
  };

  plHybridArray<Data, 16> m_ExtractedRenderData;
  plUInt32 m_uiNumCacheIfStatic = 0;
};

struct PL_RENDERERCORE_DLL plMsgExtractOccluderData : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgExtractOccluderData, plMessage);

  void AddOccluder(const plRasterizerObject* pObject, const plTransform& transform)
  {
    auto& d = m_ExtractedOccluderData.ExpandAndGetRef();
    d.m_pObject = pObject;
    d.m_Transform = transform;
  }

private:
  friend class plRenderPipeline;

  struct Data
  {
    const plRasterizerObject* m_pObject = nullptr;
    plTransform m_Transform;
  };

  plHybridArray<Data, 16> m_ExtractedOccluderData;
};

#include <RendererCore/Pipeline/Implementation/RenderData_inl.h>

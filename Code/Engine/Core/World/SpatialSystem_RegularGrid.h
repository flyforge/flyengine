#pragma once

#include <Core/World/SpatialSystem.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Types/UniquePtr.h>

namespace plInternal
{
  struct QueryHelper;
}

class PL_CORE_DLL plSpatialSystem_RegularGrid : public plSpatialSystem
{
  PL_ADD_DYNAMIC_REFLECTION(plSpatialSystem_RegularGrid, plSpatialSystem);

public:
  plSpatialSystem_RegularGrid(plUInt32 uiCellSize = 128);
  ~plSpatialSystem_RegularGrid();

  /// \brief Returns the bounding box of the cell associated with the given spatial data. Useful for debug visualizations.
  plResult GetCellBoxForSpatialData(const plSpatialDataHandle& hData, plBoundingBox& out_boundingBox) const;

  /// \brief Returns bounding boxes of all existing cells.
  void GetAllCellBoxes(plDynamicArray<plBoundingBox>& out_boundingBoxes, plSpatialData::Category filterCategory = plInvalidSpatialDataCategory) const;

private:
  friend plInternal::QueryHelper;

  // plSpatialSystem implementation
  virtual void StartNewFrame() override;

  plSpatialDataHandle CreateSpatialData(const plSimdBBoxSphere& bounds, plGameObject* pObject, plUInt32 uiCategoryBitmask, const plTagSet& tags) override;
  plSpatialDataHandle CreateSpatialDataAlwaysVisible(plGameObject* pObject, plUInt32 uiCategoryBitmask, const plTagSet& tags) override;

  void DeleteSpatialData(const plSpatialDataHandle& hData) override;

  void UpdateSpatialDataBounds(const plSpatialDataHandle& hData, const plSimdBBoxSphere& bounds) override;
  void UpdateSpatialDataObject(const plSpatialDataHandle& hData, plGameObject* pObject) override;

  void FindObjectsInSphere(const plBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const override;
  void FindObjectsInBox(const plBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const override;

  void FindVisibleObjects(const plFrustum& frustum, const QueryParams& queryParams, plDynamicArray<const plGameObject*>& out_Objects, plSpatialSystem::IsOccludedFunc IsOccluded, plVisibilityState visType) const override;

  plVisibilityState GetVisibilityState(const plSpatialDataHandle& hData, plUInt32 uiNumFramesBeforeInvisible) const override;

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  virtual void GetInternalStats(plStringBuilder& sb) const override;
#endif

  plProxyAllocator m_AlignedAllocator;

  plSimdVec4i m_vCellSize;
  plSimdVec4f m_vOverlapSize;
  plSimdFloat m_fInvCellSize;

  enum
  {
    MAX_NUM_GRIDS = 63,
    MAX_NUM_REGULAR_GRIDS = (sizeof(plSpatialData::Category::m_uiValue) * 8),
    MAX_NUM_CACHED_GRIDS = MAX_NUM_GRIDS - MAX_NUM_REGULAR_GRIDS
  };

  struct Cell;
  struct Grid;
  plDynamicArray<plUniquePtr<Grid>> m_Grids;
  plUInt32 m_uiFirstCachedGridIndex = MAX_NUM_GRIDS;

  struct Data
  {
    PL_DECLARE_POD_TYPE();

    plUInt64 m_uiGridBitmask : MAX_NUM_GRIDS;
    plUInt64 m_uiAlwaysVisible : 1;
  };

  plIdTable<plSpatialDataId, Data, plLocalAllocatorWrapper> m_DataTable;

  bool IsAlwaysVisibleData(const Data& data) const;

  plSpatialDataHandle AddSpatialDataToGrids(const plSimdBBoxSphere& bounds, plGameObject* pObject, plUInt32 uiCategoryBitmask, const plTagSet& tags, bool bAlwaysVisible);

  template <typename Functor>
  void ForEachGrid(const Data& data, const plSpatialDataHandle& hData, Functor func) const;

  struct Stats;
  using CellCallback = plDelegate<plVisitorExecution::Enum(const Cell&, const QueryParams&, Stats&, void*, plVisibilityState)>;
  void ForEachCellInBoxInMatchingGrids(const plSimdBBox& box, const QueryParams& queryParams, CellCallback noFilterCallback, CellCallback filterByTagsCallback, void* pUserData, plVisibilityState visType) const;

  struct CacheCandidate
  {
    plTagSet m_IncludeTags;
    plTagSet m_ExcludeTags;
    plSpatialData::Category m_Category;
    float m_fQueryCount = 0.0f;
    float m_fFilteredRatio = 0.0f;
    plUInt32 m_uiGridIndex = plInvalidIndex;
  };

  mutable plDynamicArray<CacheCandidate> m_CacheCandidates;
  mutable plMutex m_CacheCandidatesMutex;

  struct SortedCacheCandidate
  {
    plUInt32 m_uiIndex = 0;
    float m_fScore = 0;

    bool operator<(const SortedCacheCandidate& other) const
    {
      if (m_fScore != other.m_fScore)
        return m_fScore > other.m_fScore; // higher score comes first

      return m_uiIndex < other.m_uiIndex;
    }
  };

  plDynamicArray<SortedCacheCandidate> m_SortedCacheCandidates;

  void MigrateCachedGrid(plUInt32 uiCandidateIndex);
  void MigrateSpatialData(plUInt32 uiTargetGridIndex, plUInt32 uiSourceGridIndex);

  void RemoveCachedGrid(plUInt32 uiCandidateIndex);
  void RemoveAllCachedGrids();

  void UpdateCacheCandidate(const plTagSet* pIncludeTags, const plTagSet* pExcludeTags, plSpatialData::Category category, float filteredRatio) const;
};

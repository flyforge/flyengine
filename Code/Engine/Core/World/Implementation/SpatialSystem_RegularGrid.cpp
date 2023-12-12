#include <Core/CorePCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Time/Stopwatch.h>

plCVarInt cvar_SpatialQueriesCachingThreshold("Spatial.Queries.CachingThreshold", 100, plCVarFlags::Default, "Number of objects that are tested for a query before it is considered for caching");

namespace
{
  enum
  {
    MAX_CELL_INDEX = (1 << 20) - 1,
    CELL_INDEX_MASK = (1 << 21) - 1
  };

  PLASMA_ALWAYS_INLINE plSimdVec4f ToVec3(const plSimdVec4i& v)
  {
    return v.ToFloat();
  }

  PLASMA_ALWAYS_INLINE plSimdVec4i ToVec3I32(const plSimdVec4f& v)
  {
    plSimdVec4f vf = v.Floor();
    return plSimdVec4i::Truncate(vf);
  }

  PLASMA_ALWAYS_INLINE plUInt64 GetCellKey(plInt32 x, plInt32 y, plInt32 z)
  {
    plUInt64 sx = (x + MAX_CELL_INDEX) & CELL_INDEX_MASK;
    plUInt64 sy = (y + MAX_CELL_INDEX) & CELL_INDEX_MASK;
    plUInt64 sz = (z + MAX_CELL_INDEX) & CELL_INDEX_MASK;

    return (sx << 42) | (sy << 21) | sz;
  }

  PLASMA_ALWAYS_INLINE plSimdBBox ComputeCellBoundingBox(const plSimdVec4i& vCellIndex, const plSimdVec4i& vCellSize)
  {
    plSimdVec4i overlapSize = vCellSize >> 2;
    plSimdVec4i minPos = vCellIndex.CompMul(vCellSize);

    plSimdVec4f bmin = ToVec3(minPos - overlapSize);
    plSimdVec4f bmax = ToVec3(minPos + overlapSize + vCellSize);

    return plSimdBBox(bmin, bmax);
  }

  PLASMA_ALWAYS_INLINE bool FilterByCategory(plUInt32 uiCategoryBitmask, plUInt32 uiQueryBitmask)
  {
    return (uiCategoryBitmask & uiQueryBitmask) == 0;
  }

  PLASMA_ALWAYS_INLINE bool FilterByTags(const plTagSet& tags, const plTagSet& includeTags, const plTagSet& excludeTags)
  {
    if (!excludeTags.IsEmpty() && excludeTags.IsAnySet(tags))
      return true;

    if (!includeTags.IsEmpty() && !includeTags.IsAnySet(tags))
      return true;

    return false;
  }

  PLASMA_ALWAYS_INLINE bool CanBeCached(plSpatialData::Category category)
  {
    return plSpatialData::GetCategoryFlags(category).IsSet(plSpatialData::Flags::FrequentChanges) == false;
  }

  void TagsToString(const plTagSet& tags, plStringBuilder& out_sSb)
  {
    out_sSb.Append("{ ");

    bool first = true;
    for (auto it = tags.GetIterator(); it.IsValid(); ++it)
    {
      if (!first)
      {
        out_sSb.Append(", ");
        first = false;
      }
      out_sSb.Append(it->GetTagString().GetView());
    }

    out_sSb.Append(" }");
  }

  struct PlaneData
  {
    plSimdVec4f m_x0x1x2x3;
    plSimdVec4f m_y0y1y2y3;
    plSimdVec4f m_z0z1z2z3;
    plSimdVec4f m_w0w1w2w3;

    plSimdVec4f m_x4x5x4x5;
    plSimdVec4f m_y4y5y4y5;
    plSimdVec4f m_z4z5z4z5;
    plSimdVec4f m_w4w5w4w5;
  };

  PLASMA_FORCE_INLINE bool SphereFrustumIntersect(const plSimdBSphere& sphere, const PlaneData& planeData)
  {
    plSimdVec4f pos_xxxx(sphere.m_CenterAndRadius.x());
    plSimdVec4f pos_yyyy(sphere.m_CenterAndRadius.y());
    plSimdVec4f pos_zzzz(sphere.m_CenterAndRadius.z());
    plSimdVec4f pos_rrrr(sphere.m_CenterAndRadius.w());

    plSimdVec4f dot_0123;
    dot_0123 = plSimdVec4f::MulAdd(pos_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dot_0123 = plSimdVec4f::MulAdd(pos_yyyy, planeData.m_y0y1y2y3, dot_0123);
    dot_0123 = plSimdVec4f::MulAdd(pos_zzzz, planeData.m_z0z1z2z3, dot_0123);

    plSimdVec4f dot_4545;
    dot_4545 = plSimdVec4f::MulAdd(pos_xxxx, planeData.m_x4x5x4x5, planeData.m_w4w5w4w5);
    dot_4545 = plSimdVec4f::MulAdd(pos_yyyy, planeData.m_y4y5y4y5, dot_4545);
    dot_4545 = plSimdVec4f::MulAdd(pos_zzzz, planeData.m_z4z5z4z5, dot_4545);

    plSimdVec4b cmp_0123 = dot_0123 > pos_rrrr;
    plSimdVec4b cmp_4545 = dot_4545 > pos_rrrr;
    return (cmp_0123 || cmp_4545).NoneSet<4>();
  }

  PLASMA_FORCE_INLINE plUInt32 SphereFrustumIntersect(const plSimdBSphere& sphereA, const plSimdBSphere& sphereB, const PlaneData& planeData)
  {
    plSimdVec4f posA_xxxx(sphereA.m_CenterAndRadius.x());
    plSimdVec4f posA_yyyy(sphereA.m_CenterAndRadius.y());
    plSimdVec4f posA_zzzz(sphereA.m_CenterAndRadius.z());
    plSimdVec4f posA_rrrr(sphereA.m_CenterAndRadius.w());

    plSimdVec4f dotA_0123;
    dotA_0123 = plSimdVec4f::MulAdd(posA_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dotA_0123 = plSimdVec4f::MulAdd(posA_yyyy, planeData.m_y0y1y2y3, dotA_0123);
    dotA_0123 = plSimdVec4f::MulAdd(posA_zzzz, planeData.m_z0z1z2z3, dotA_0123);

    plSimdVec4f posB_xxxx(sphereB.m_CenterAndRadius.x());
    plSimdVec4f posB_yyyy(sphereB.m_CenterAndRadius.y());
    plSimdVec4f posB_zzzz(sphereB.m_CenterAndRadius.z());
    plSimdVec4f posB_rrrr(sphereB.m_CenterAndRadius.w());

    plSimdVec4f dotB_0123;
    dotB_0123 = plSimdVec4f::MulAdd(posB_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dotB_0123 = plSimdVec4f::MulAdd(posB_yyyy, planeData.m_y0y1y2y3, dotB_0123);
    dotB_0123 = plSimdVec4f::MulAdd(posB_zzzz, planeData.m_z0z1z2z3, dotB_0123);

    plSimdVec4f posAB_xxxx = posA_xxxx.GetCombined<plSwizzle::XXXX>(posB_xxxx);
    plSimdVec4f posAB_yyyy = posA_yyyy.GetCombined<plSwizzle::XXXX>(posB_yyyy);
    plSimdVec4f posAB_zzzz = posA_zzzz.GetCombined<plSwizzle::XXXX>(posB_zzzz);
    plSimdVec4f posAB_rrrr = posA_rrrr.GetCombined<plSwizzle::XXXX>(posB_rrrr);

    plSimdVec4f dot_A45B45;
    dot_A45B45 = plSimdVec4f::MulAdd(posAB_xxxx, planeData.m_x4x5x4x5, planeData.m_w4w5w4w5);
    dot_A45B45 = plSimdVec4f::MulAdd(posAB_yyyy, planeData.m_y4y5y4y5, dot_A45B45);
    dot_A45B45 = plSimdVec4f::MulAdd(posAB_zzzz, planeData.m_z4z5z4z5, dot_A45B45);

    plSimdVec4b cmp_A0123 = dotA_0123 > posA_rrrr;
    plSimdVec4b cmp_B0123 = dotB_0123 > posB_rrrr;
    plSimdVec4b cmp_A45B45 = dot_A45B45 > posAB_rrrr;

    plSimdVec4b cmp_A45 = cmp_A45B45.Get<plSwizzle::XYXY>();
    plSimdVec4b cmp_B45 = cmp_A45B45.Get<plSwizzle::ZWZW>();

    plUInt32 result = (cmp_A0123 || cmp_A45).NoneSet<4>() ? 1 : 0;
    result |= (cmp_B0123 || cmp_B45).NoneSet<4>() ? 2 : 0;

    return result;
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

struct CellDataMapping
{
  PLASMA_DECLARE_POD_TYPE();

  plUInt32 m_uiCellIndex = plInvalidIndex;
  plUInt32 m_uiCellDataIndex = plInvalidIndex;
};

struct plSpatialSystem_RegularGrid::Cell
{
  Cell(plAllocatorBase* pAlignedAlloctor, plAllocatorBase* pAllocator)
    : m_BoundingSpheres(pAlignedAlloctor)
    , m_BoundingBoxHalfExtents(pAlignedAlloctor)
    , m_TagSets(pAllocator)
    , m_ObjectPointers(pAllocator)
    , m_DataIndices(pAllocator)
  {
  }

  PLASMA_FORCE_INLINE plUInt32 AddData(const plSimdBBoxSphere& bounds, const plTagSet& tags, plGameObject* pObject, plUInt64 uiLastVisibleFrameIdxAndVisType, plUInt32 uiDataIndex)
  {
    m_BoundingSpheres.PushBack(bounds.GetSphere());
    m_BoundingBoxHalfExtents.PushBack(bounds.m_BoxHalfExtents);
    m_TagSets.PushBack(tags);
    m_ObjectPointers.PushBack(pObject);
    m_DataIndices.PushBack(uiDataIndex);
    m_LastVisibleFrameIdxAndVisType.PushBack(uiLastVisibleFrameIdxAndVisType);

    return m_BoundingSpheres.GetCount() - 1;
  }

  // Returns the data index of the moved data
  PLASMA_FORCE_INLINE plUInt32 RemoveData(plUInt32 uiCellDataIndex)
  {
    plUInt32 uiMovedDataIndex = m_DataIndices.PeekBack();

    m_BoundingSpheres.RemoveAtAndSwap(uiCellDataIndex);
    m_BoundingBoxHalfExtents.RemoveAtAndSwap(uiCellDataIndex);
    m_TagSets.RemoveAtAndSwap(uiCellDataIndex);
    m_ObjectPointers.RemoveAtAndSwap(uiCellDataIndex);
    m_DataIndices.RemoveAtAndSwap(uiCellDataIndex);
    m_LastVisibleFrameIdxAndVisType.RemoveAtAndSwap(uiCellDataIndex);

    PLASMA_ASSERT_DEBUG(m_DataIndices.GetCount() == uiCellDataIndex || m_DataIndices[uiCellDataIndex] == uiMovedDataIndex, "Implementation error");

    return uiMovedDataIndex;
  }

  PLASMA_ALWAYS_INLINE plBoundingBox GetBoundingBox() const { return plSimdConversion::ToBBoxSphere(m_Bounds).GetBox(); }

  plSimdBBoxSphere m_Bounds;

  plDynamicArray<plSimdBSphere> m_BoundingSpheres;
  plDynamicArray<plSimdVec4f> m_BoundingBoxHalfExtents;
  plDynamicArray<plTagSet> m_TagSets;
  plDynamicArray<plGameObject*> m_ObjectPointers;
  mutable plDynamicArray<plAtomicInteger64> m_LastVisibleFrameIdxAndVisType;
  plDynamicArray<plUInt32> m_DataIndices;
};

//////////////////////////////////////////////////////////////////////////

struct CellKeyHashHelper
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(plUInt64 value)
  {
    // return plUInt32(value * 2654435761U);
    return plHashHelper<plUInt64>::Hash(value);
  }

  PLASMA_ALWAYS_INLINE static bool Equal(plUInt64 a, plUInt64 b) { return a == b; }
};

//////////////////////////////////////////////////////////////////////////

struct plSpatialSystem_RegularGrid::Grid
{
  Grid(plSpatialSystem_RegularGrid& ref_system, plSpatialData::Category category)
    : m_System(ref_system)
    , m_Cells(&ref_system.m_Allocator)
    , m_CellKeyToCellIndex(&ref_system.m_Allocator)
    , m_Category(category)
    , m_bCanBeCached(CanBeCached(category))
  {
    plSimdBBox overflowBox;
    overflowBox.SetCenterAndHalfExtents(plSimdVec4f::ZeroVector(), plSimdVec4f((float)(ref_system.m_vCellSize.x() * MAX_CELL_INDEX)));

    auto pOverflowCell = PLASMA_NEW(&m_System.m_AlignedAllocator, Cell, &m_System.m_AlignedAllocator, &m_System.m_Allocator);
    pOverflowCell->m_Bounds = overflowBox;

    m_Cells.PushBack(pOverflowCell);
  }

  plUInt32 GetOrCreateCell(const plSimdBBoxSphere& bounds)
  {
    plSimdVec4i cellIndex = ToVec3I32(bounds.m_CenterAndRadius * m_System.m_fInvCellSize);
    plSimdBBox cellBox = ComputeCellBoundingBox(cellIndex, m_System.m_vCellSize);

    if (cellBox.Contains(bounds.GetBox()))
    {
      plUInt64 cellKey = GetCellKey(cellIndex.x(), cellIndex.y(), cellIndex.z());

      plUInt32 uiCellIndex = 0;
      if (m_CellKeyToCellIndex.TryGetValue(cellKey, uiCellIndex))
      {
        return uiCellIndex;
      }

      uiCellIndex = m_Cells.GetCount();
      m_CellKeyToCellIndex.Insert(cellKey, uiCellIndex);

      auto pNewCell = PLASMA_NEW(&m_System.m_AlignedAllocator, Cell, &m_System.m_AlignedAllocator, &m_System.m_Allocator);
      pNewCell->m_Bounds = cellBox;

      m_Cells.PushBack(pNewCell);

      return uiCellIndex;
    }
    else
    {
      return m_uiOverflowCellIndex;
    }
  }

  void AddSpatialData(const plSimdBBoxSphere& bounds, const plTagSet& tags, plGameObject* pObject, plUInt64 uiLastVisibleFrameIdxAndVisType, const plSpatialDataHandle& hData)
  {
    plUInt32 uiDataIndex = hData.GetInternalID().m_InstanceIndex;

    plUInt32 uiCellIndex = GetOrCreateCell(bounds);
    plUInt32 uiCellDataIndex = m_Cells[uiCellIndex]->AddData(bounds, tags, pObject, uiLastVisibleFrameIdxAndVisType, uiDataIndex);

    m_CellDataMappings.EnsureCount(uiDataIndex + 1);
    PLASMA_ASSERT_DEBUG(m_CellDataMappings[uiDataIndex].m_uiCellIndex == plInvalidIndex, "data has already been added to a cell");
    m_CellDataMappings[uiDataIndex] = {uiCellIndex, uiCellDataIndex};
  }

  void RemoveSpatialData(const plSpatialDataHandle& hData)
  {
    plUInt32 uiDataIndex = hData.GetInternalID().m_InstanceIndex;

    auto& mapping = m_CellDataMappings[uiDataIndex];
    plUInt32 uiMovedDataIndex = m_Cells[mapping.m_uiCellIndex]->RemoveData(mapping.m_uiCellDataIndex);
    if (uiMovedDataIndex != uiDataIndex)
    {
      m_CellDataMappings[uiMovedDataIndex].m_uiCellDataIndex = mapping.m_uiCellDataIndex;
    }

    mapping = {};
  }

  bool MigrateSpatialDataFromOtherGrid(plUInt32 uiDataIndex, const Grid& other)
  {
    // Data has already been added
    if (uiDataIndex < m_CellDataMappings.GetCount() && m_CellDataMappings[uiDataIndex].m_uiCellIndex != plInvalidIndex)
      return false;

    auto& mapping = other.m_CellDataMappings[uiDataIndex];
    if (mapping.m_uiCellIndex == plInvalidIndex)
      return false;

    auto& pOtherCell = other.m_Cells[mapping.m_uiCellIndex];

    const plTagSet& tags = pOtherCell->m_TagSets[mapping.m_uiCellDataIndex];
    if (FilterByTags(tags, m_IncludeTags, m_ExcludeTags))
      return false;

    plSimdBBoxSphere bounds;
    bounds.m_CenterAndRadius = pOtherCell->m_BoundingSpheres[mapping.m_uiCellDataIndex].m_CenterAndRadius;
    bounds.m_BoxHalfExtents = pOtherCell->m_BoundingBoxHalfExtents[mapping.m_uiCellDataIndex];
    plGameObject* objectPointer = pOtherCell->m_ObjectPointers[mapping.m_uiCellDataIndex];
    const plUInt64 uiLastVisibleFrameIdxAndVisType = pOtherCell->m_LastVisibleFrameIdxAndVisType[mapping.m_uiCellDataIndex];

    PLASMA_ASSERT_DEBUG(pOtherCell->m_DataIndices[mapping.m_uiCellDataIndex] == uiDataIndex, "Implementation error");
    plSpatialDataHandle hData = plSpatialDataHandle(plSpatialDataId(uiDataIndex, 1));

    AddSpatialData(bounds, tags, objectPointer, uiLastVisibleFrameIdxAndVisType, hData);
    return true;
  }

  PLASMA_ALWAYS_INLINE bool CachingCompleted() const { return m_uiLastMigrationIndex == plInvalidIndex; }

  template <typename Functor>
  PLASMA_FORCE_INLINE void ForEachCellInBox(const plSimdBBox& box, Functor func) const
  {
    plSimdVec4i minIndex = ToVec3I32((box.m_Min - m_System.m_vOverlapSize) * m_System.m_fInvCellSize);
    plSimdVec4i maxIndex = ToVec3I32((box.m_Max + m_System.m_vOverlapSize) * m_System.m_fInvCellSize);

    PLASMA_ASSERT_DEBUG((minIndex.Abs() < plSimdVec4i(MAX_CELL_INDEX)).AllSet<3>(), "Position is too big");
    PLASMA_ASSERT_DEBUG((maxIndex.Abs() < plSimdVec4i(MAX_CELL_INDEX)).AllSet<3>(), "Position is too big");

    const plInt32 iMinX = minIndex.x();
    const plInt32 iMinY = minIndex.y();
    const plInt32 iMinZ = minIndex.z();

    const plSimdVec4i diff = maxIndex - minIndex + plSimdVec4i(1);
    const plInt32 iDiffX = diff.x();
    const plInt32 iDiffY = diff.y();
    const plInt32 iDiffZ = diff.z();
    const plInt32 iNumIterations = iDiffX * iDiffY * iDiffZ;

    for (plInt32 i = 0; i < iNumIterations; ++i)
    {
      plInt32 index = i;
      plInt32 z = i / (iDiffX * iDiffY);
      index -= z * iDiffX * iDiffY;
      plInt32 y = index / iDiffX;
      plInt32 x = index - (y * iDiffX);

      x += iMinX;
      y += iMinY;
      z += iMinZ;

      plUInt64 cellKey = GetCellKey(x, y, z);
      plUInt32 cellIndex = 0;
      if (m_CellKeyToCellIndex.TryGetValue(cellKey, cellIndex))
      {
        const Cell& constCell = *m_Cells[cellIndex];
        if (func(constCell) == plVisitorExecution::Stop)
          return;
      }
    }

    const Cell& overflowCell = *m_Cells[m_uiOverflowCellIndex];
    func(overflowCell);
  }

  plSpatialSystem_RegularGrid& m_System;
  plDynamicArray<plUniquePtr<Cell>> m_Cells;

  plHashTable<plUInt64, plUInt32, CellKeyHashHelper> m_CellKeyToCellIndex;
  static constexpr plUInt32 m_uiOverflowCellIndex = 0;

  plDynamicArray<CellDataMapping> m_CellDataMappings;

  const plSpatialData::Category m_Category;
  const bool m_bCanBeCached;

  plTagSet m_IncludeTags;
  plTagSet m_ExcludeTags;

  plUInt32 m_uiLastMigrationIndex = 0;
};

//////////////////////////////////////////////////////////////////////////

struct plSpatialSystem_RegularGrid::Stats
{
  plUInt32 m_uiNumObjectsTested = 0;
  plUInt32 m_uiNumObjectsPassed = 0;
  plUInt32 m_uiNumObjectsFiltered = 0;
};

//////////////////////////////////////////////////////////////////////////

namespace plInternal
{
  struct QueryHelper
  {
    template <typename T>
    struct ShapeQueryData
    {
      T m_Shape;
      plSpatialSystem::QueryCallback m_Callback;
    };

    template <typename T, bool UseTagsFilter>
    static plVisitorExecution::Enum ShapeQueryCallback(const plSpatialSystem_RegularGrid::Cell& cell, const plSpatialSystem::QueryParams& queryParams, plSpatialSystem_RegularGrid::Stats& ref_stats, void* pUserData, plVisibilityState visType)
    {
      auto pQueryData = static_cast<const ShapeQueryData<T>*>(pUserData);
      T shape = pQueryData->m_Shape;

      plSimdBBox cellBox = cell.m_Bounds.GetBox();
      if (!cellBox.Overlaps(shape))
        return plVisitorExecution::Continue;

      auto boundingSpheres = cell.m_BoundingSpheres.GetData();
      auto tagSets = cell.m_TagSets.GetData();
      auto objectPointers = cell.m_ObjectPointers.GetData();

      const plUInt32 numSpheres = cell.m_BoundingSpheres.GetCount();
      ref_stats.m_uiNumObjectsTested += numSpheres;

      for (plUInt32 i = 0; i < numSpheres; ++i)
      {
        if (!shape.Overlaps(boundingSpheres[i]))
          continue;

        if constexpr (UseTagsFilter)
        {
          if (FilterByTags(tagSets[i], queryParams.m_IncludeTags, queryParams.m_ExcludeTags))
          {
            ref_stats.m_uiNumObjectsFiltered++;
            continue;
          }
        }

        ref_stats.m_uiNumObjectsPassed++;

        if (pQueryData->m_Callback(objectPointers[i]) == plVisitorExecution::Stop)
          return plVisitorExecution::Stop;
      }

      return plVisitorExecution::Continue;
    }

    struct FrustumQueryData
    {
      PlaneData m_PlaneData;
      plDynamicArray<const plGameObject*>* m_pOutObjects;
      plUInt64 m_uiFrameCounter;
      plSpatialSystem::IsOccludedFunc m_IsOccludedCB;
    };

    template <bool UseTagsFilter, bool UseOcclusionCallback>
    static plVisitorExecution::Enum FrustumQueryCallback(const plSpatialSystem_RegularGrid::Cell& cell, const plSpatialSystem::QueryParams& queryParams, plSpatialSystem_RegularGrid::Stats& ref_stats, void* pUserData, plVisibilityState visType)
    {
      auto pQueryData = static_cast<FrustumQueryData*>(pUserData);
      PlaneData planeData = pQueryData->m_PlaneData;

      plSimdBSphere cellSphere = cell.m_Bounds.GetSphere();
      if (!SphereFrustumIntersect(cellSphere, planeData))
        return plVisitorExecution::Continue;

      if constexpr (UseOcclusionCallback)
      {
        if (pQueryData->m_IsOccludedCB(cell.m_Bounds.GetBox()))
        {
          return plVisitorExecution::Continue;
        }
      }

      plSimdBBox bbox;
      auto boundingSpheres = cell.m_BoundingSpheres.GetData();
      auto boundingBoxHalfExtents = cell.m_BoundingBoxHalfExtents.GetData();
      auto tagSets = cell.m_TagSets.GetData();
      auto objectPointers = cell.m_ObjectPointers.GetData();
      auto lastVisibleFrameIdxAndVisType = cell.m_LastVisibleFrameIdxAndVisType.GetData();

      const plUInt32 numSpheres = cell.m_BoundingSpheres.GetCount();
      ref_stats.m_uiNumObjectsTested += numSpheres;

      plUInt32 currentIndex = 0;
      const plUInt64 uiFrameIdxAndType = (pQueryData->m_uiFrameCounter << 4) | static_cast<plUInt64>(visType);

      while (currentIndex < numSpheres)
      {
        if (numSpheres - currentIndex >= 32)
        {
          plUInt32 mask = 0;

          for (plUInt32 i = 0; i < 32; i += 2)
          {
            auto& objectSphereA = boundingSpheres[currentIndex + i + 0];
            auto& objectSphereB = boundingSpheres[currentIndex + i + 1];

            mask |= SphereFrustumIntersect(objectSphereA, objectSphereB, planeData) << i;
          }

          while (mask > 0)
          {
            plUInt32 i = plMath::FirstBitLow(mask) + currentIndex;
            mask &= mask - 1;

            if constexpr (UseTagsFilter)
            {
              if (FilterByTags(tagSets[i], queryParams.m_IncludeTags, queryParams.m_ExcludeTags))
              {
                ref_stats.m_uiNumObjectsFiltered++;
                continue;
              }
            }

            if constexpr (UseOcclusionCallback)
            {
              bbox.SetCenterAndHalfExtents(boundingSpheres[i].GetCenter(), boundingBoxHalfExtents[i]);
              if (pQueryData->m_IsOccludedCB(bbox))
              {
                continue;
              }
            }

            lastVisibleFrameIdxAndVisType[i].Max(uiFrameIdxAndType);
            pQueryData->m_pOutObjects->PushBack(objectPointers[i]);

            ref_stats.m_uiNumObjectsPassed++;
          }

          currentIndex += 32;
        }
        else
        {
          plUInt32 i = currentIndex;
          ++currentIndex;

          if (!SphereFrustumIntersect(boundingSpheres[i], planeData))
            continue;

          if constexpr (UseTagsFilter)
          {
            if (FilterByTags(tagSets[i], queryParams.m_IncludeTags, queryParams.m_ExcludeTags))
            {
              ref_stats.m_uiNumObjectsFiltered++;
              continue;
            }
          }

          if constexpr (UseOcclusionCallback)
          {
            bbox.SetCenterAndHalfExtents(boundingSpheres[i].GetCenter(), boundingBoxHalfExtents[i]);

            if (pQueryData->m_IsOccludedCB(bbox))
            {
              continue;
            }
          }

          lastVisibleFrameIdxAndVisType[i].Max(uiFrameIdxAndType);
          pQueryData->m_pOutObjects->PushBack(objectPointers[i]);

          ref_stats.m_uiNumObjectsPassed++;
        }
      }

      return plVisitorExecution::Continue;
    }
  };
} // namespace plInternal

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSpatialSystem_RegularGrid, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plSpatialSystem_RegularGrid::plSpatialSystem_RegularGrid(plUInt32 uiCellSize /*= 128*/)
  : m_AlignedAllocator("Spatial System Aligned", plFoundation::GetAlignedAllocator())
  , m_Grids(&m_Allocator)
  , m_DataTable(&m_Allocator)
  , m_vCellSize(uiCellSize)
  , m_vOverlapSize(uiCellSize / 4.0f)
  , m_fInvCellSize(1.0f / uiCellSize)
{
  PLASMA_CHECK_AT_COMPILETIME(sizeof(Data) == 8);

  m_Grids.SetCount(MAX_NUM_GRIDS);

  cvar_SpatialQueriesCachingThreshold.m_CVarEvents.AddEventHandler([&](const plCVarEvent& e) {
      if (e.m_EventType == plCVarEvent::ValueChanged)
      {
        RemoveAllCachedGrids();
      } });
}

plSpatialSystem_RegularGrid::~plSpatialSystem_RegularGrid() = default;

plResult plSpatialSystem_RegularGrid::GetCellBoxForSpatialData(const plSpatialDataHandle& hData, plBoundingBox& out_boundingBox) const
{
  Data* pData = nullptr;
  if (!m_DataTable.TryGetValue(hData.GetInternalID(), pData))
    return PLASMA_FAILURE;

  ForEachGrid(*pData, hData,
    [&](Grid& ref_grid, const CellDataMapping& mapping) {
      auto& pCell = ref_grid.m_Cells[mapping.m_uiCellIndex];

      out_boundingBox = pCell->GetBoundingBox();
      return plVisitorExecution::Stop;
    });

  return PLASMA_SUCCESS;
}

template <>
struct plHashHelper<plBoundingBox>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(const plBoundingBox& value) { return plHashingUtils::xxHash32(&value, sizeof(plBoundingBox)); }

  PLASMA_ALWAYS_INLINE static bool Equal(const plBoundingBox& a, const plBoundingBox& b) { return a == b; }
};

void plSpatialSystem_RegularGrid::GetAllCellBoxes(plDynamicArray<plBoundingBox>& out_boundingBoxes, plSpatialData::Category filterCategory /*= plInvalidSpatialDataCategory*/) const
{
  if (filterCategory != plInvalidSpatialDataCategory)
  {
    plUInt32 uiGridIndex = filterCategory.m_uiValue;
    auto& pGrid = m_Grids[uiGridIndex];
    if (pGrid != nullptr)
    {
      for (auto& pCell : pGrid->m_Cells)
      {
        out_boundingBoxes.ExpandAndGetRef() = pCell->GetBoundingBox();
      }
    }
  }
  else
  {
    plHashSet<plBoundingBox> boundingBoxes;

    for (auto& pGrid : m_Grids)
    {
      if (pGrid != nullptr)
      {
        for (auto& pCell : pGrid->m_Cells)
        {
          boundingBoxes.Insert(pCell->GetBoundingBox());
        }
      }
    }

    for (auto boundingBox : boundingBoxes)
    {
      out_boundingBoxes.PushBack(boundingBox);
    }
  }
}

void plSpatialSystem_RegularGrid::StartNewFrame()
{
  SUPER::StartNewFrame();

  m_SortedCacheCandidates.Clear();

  {
    PLASMA_LOCK(m_CacheCandidatesMutex);

    for (plUInt32 i = 0; i < m_CacheCandidates.GetCount(); ++i)
    {
      auto& cacheCandidate = m_CacheCandidates[i];

      const float fScore = cacheCandidate.m_fQueryCount + cacheCandidate.m_fFilteredRatio * 100.0f;
      m_SortedCacheCandidates.PushBack({i, fScore});

      // Query has to be issued at least once every 10 frames to keep a stable value
      cacheCandidate.m_fQueryCount = plMath::Max(cacheCandidate.m_fQueryCount - 0.1f, 0.0f);
    }
  }

  m_SortedCacheCandidates.Sort();

  // First remove all cached grids that don't make it into the top MAX_NUM_CACHED_GRIDS to make space for new grids
  if (m_SortedCacheCandidates.GetCount() > MAX_NUM_CACHED_GRIDS)
  {
    for (plUInt32 i = MAX_NUM_CACHED_GRIDS; i < m_SortedCacheCandidates.GetCount(); ++i)
    {
      RemoveCachedGrid(m_SortedCacheCandidates[i].m_uiIndex);
    }
  }

  // Then take the MAX_NUM_CACHED_GRIDS candidates with the highest score and migrate the data
  for (plUInt32 i = 0; i < plMath::Min<plUInt32>(m_SortedCacheCandidates.GetCount(), MAX_NUM_CACHED_GRIDS); ++i)
  {
    MigrateCachedGrid(m_SortedCacheCandidates[i].m_uiIndex);
  }
}

plSpatialDataHandle plSpatialSystem_RegularGrid::CreateSpatialData(const plSimdBBoxSphere& bounds, plGameObject* pObject, plUInt32 uiCategoryBitmask, const plTagSet& tags)
{
  if (uiCategoryBitmask == 0)
    return plSpatialDataHandle();

  return AddSpatialDataToGrids(bounds, pObject, uiCategoryBitmask, tags, false);
}

plSpatialDataHandle plSpatialSystem_RegularGrid::CreateSpatialDataAlwaysVisible(plGameObject* pObject, plUInt32 uiCategoryBitmask, const plTagSet& tags)
{
  if (uiCategoryBitmask == 0)
    return plSpatialDataHandle();

  plSimdBBox hugeBox;
  hugeBox.SetCenterAndHalfExtents(plSimdVec4f::ZeroVector(), plSimdVec4f((float)(m_vCellSize.x() * MAX_CELL_INDEX)));

  return AddSpatialDataToGrids(hugeBox, pObject, uiCategoryBitmask, tags, true);
}

void plSpatialSystem_RegularGrid::DeleteSpatialData(const plSpatialDataHandle& hData)
{
  Data oldData;
  PLASMA_VERIFY(m_DataTable.Remove(hData.GetInternalID(), &oldData), "Invalid spatial data handle");

  ForEachGrid(oldData, hData,
    [&](Grid& ref_grid, const CellDataMapping& mapping) {
      ref_grid.RemoveSpatialData(hData);
      return plVisitorExecution::Continue;
    });
}

void plSpatialSystem_RegularGrid::UpdateSpatialDataBounds(const plSpatialDataHandle& hData, const plSimdBBoxSphere& bounds)
{
  Data* pData = nullptr;
  PLASMA_VERIFY(m_DataTable.TryGetValue(hData.GetInternalID(), pData), "Invalid spatial data handle");

  // No need to update bounds for always visible data
  if (IsAlwaysVisibleData(*pData))
    return;

  ForEachGrid(*pData, hData,
    [&](Grid& ref_grid, const CellDataMapping& mapping) {
      auto& pOldCell = ref_grid.m_Cells[mapping.m_uiCellIndex];

      if (pOldCell->m_Bounds.GetBox().Contains(bounds.GetBox()))
      {
        pOldCell->m_BoundingSpheres[mapping.m_uiCellDataIndex] = bounds.GetSphere();
        pOldCell->m_BoundingBoxHalfExtents[mapping.m_uiCellDataIndex] = bounds.m_BoxHalfExtents;
      }
      else
      {
        const plTagSet tags = pOldCell->m_TagSets[mapping.m_uiCellDataIndex];
        plGameObject* objectPointer = pOldCell->m_ObjectPointers[mapping.m_uiCellDataIndex];

        const plUInt64 uiLastVisibleFrameIdxAndVisType = pOldCell->m_LastVisibleFrameIdxAndVisType[mapping.m_uiCellDataIndex];

        ref_grid.RemoveSpatialData(hData);

        ref_grid.AddSpatialData(bounds, tags, objectPointer, uiLastVisibleFrameIdxAndVisType, hData);
      }

      return plVisitorExecution::Continue;
    });
}

void plSpatialSystem_RegularGrid::UpdateSpatialDataObject(const plSpatialDataHandle& hData, plGameObject* pObject)
{
  Data* pData = nullptr;
  PLASMA_VERIFY(m_DataTable.TryGetValue(hData.GetInternalID(), pData), "Invalid spatial data handle");

  ForEachGrid(*pData, hData,
    [&](Grid& ref_grid, const CellDataMapping& mapping) {
      auto& pCell = ref_grid.m_Cells[mapping.m_uiCellIndex];
      pCell->m_ObjectPointers[mapping.m_uiCellDataIndex] = pObject;
      return plVisitorExecution::Continue;
    });
}

void plSpatialSystem_RegularGrid::FindObjectsInSphere(const plBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const
{
  PLASMA_PROFILE_SCOPE("FindObjectsInSphere");

  plSimdBSphere simdSphere(plSimdConversion::ToVec3(sphere.m_vCenter), sphere.m_fRadius);
  plSimdBBox simdBox;
  simdBox.SetCenterAndHalfExtents(simdSphere.m_CenterAndRadius, simdSphere.m_CenterAndRadius.Get<plSwizzle::WWWW>());

  plInternal::QueryHelper::ShapeQueryData<plSimdBSphere> queryData = {simdSphere, callback};

  ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
    &plInternal::QueryHelper::ShapeQueryCallback<plSimdBSphere, false>,
    &plInternal::QueryHelper::ShapeQueryCallback<plSimdBSphere, true>,
    &queryData, plVisibilityState::Indirect);
}

void plSpatialSystem_RegularGrid::FindObjectsInBox(const plBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const
{
  PLASMA_PROFILE_SCOPE("FindObjectsInBox");

  plSimdBBox simdBox(plSimdConversion::ToVec3(box.m_vMin), plSimdConversion::ToVec3(box.m_vMax));

  plInternal::QueryHelper::ShapeQueryData<plSimdBBox> queryData = {simdBox, callback};

  ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
    &plInternal::QueryHelper::ShapeQueryCallback<plSimdBBox, false>,
    &plInternal::QueryHelper::ShapeQueryCallback<plSimdBBox, true>,
    &queryData, plVisibilityState::Indirect);
}

void plSpatialSystem_RegularGrid::FindVisibleObjects(const plFrustum& frustum, const QueryParams& queryParams, plDynamicArray<const plGameObject*>& out_Objects, plSpatialSystem::IsOccludedFunc IsOccluded, plVisibilityState visType) const
{
  PLASMA_PROFILE_SCOPE("FindVisibleObjects");

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  plStopwatch timer;
#endif

  plVec3 cornerPoints[8];
  frustum.ComputeCornerPoints(cornerPoints);

  plSimdVec4f simdCornerPoints[8];
  for (plUInt32 i = 0; i < 8; ++i)
  {
    simdCornerPoints[i] = plSimdConversion::ToVec3(cornerPoints[i]);
  }

  plSimdBBox simdBox;
  simdBox.SetFromPoints(simdCornerPoints, 8);

  plInternal::QueryHelper::FrustumQueryData queryData;
  {
    // Compiler is too stupid to properly unroll a constant loop so we do it by hand
    plSimdVec4f plane0 = plSimdConversion::ToVec4(*reinterpret_cast<const plVec4*>(&(frustum.GetPlane(0).m_vNormal.x)));
    plSimdVec4f plane1 = plSimdConversion::ToVec4(*reinterpret_cast<const plVec4*>(&(frustum.GetPlane(1).m_vNormal.x)));
    plSimdVec4f plane2 = plSimdConversion::ToVec4(*reinterpret_cast<const plVec4*>(&(frustum.GetPlane(2).m_vNormal.x)));
    plSimdVec4f plane3 = plSimdConversion::ToVec4(*reinterpret_cast<const plVec4*>(&(frustum.GetPlane(3).m_vNormal.x)));
    plSimdVec4f plane4 = plSimdConversion::ToVec4(*reinterpret_cast<const plVec4*>(&(frustum.GetPlane(4).m_vNormal.x)));
    plSimdVec4f plane5 = plSimdConversion::ToVec4(*reinterpret_cast<const plVec4*>(&(frustum.GetPlane(5).m_vNormal.x)));

    plSimdMat4f helperMat;
    helperMat.SetRows(plane0, plane1, plane2, plane3);

    queryData.m_PlaneData.m_x0x1x2x3 = helperMat.m_col0;
    queryData.m_PlaneData.m_y0y1y2y3 = helperMat.m_col1;
    queryData.m_PlaneData.m_z0z1z2z3 = helperMat.m_col2;
    queryData.m_PlaneData.m_w0w1w2w3 = helperMat.m_col3;

    helperMat.SetRows(plane4, plane5, plane4, plane5);

    queryData.m_PlaneData.m_x4x5x4x5 = helperMat.m_col0;
    queryData.m_PlaneData.m_y4y5y4y5 = helperMat.m_col1;
    queryData.m_PlaneData.m_z4z5z4z5 = helperMat.m_col2;
    queryData.m_PlaneData.m_w4w5w4w5 = helperMat.m_col3;

    queryData.m_pOutObjects = &out_Objects;
    queryData.m_uiFrameCounter = m_uiFrameCounter;

    queryData.m_IsOccludedCB = IsOccluded;
  }

  if (IsOccluded.IsValid())
  {
    ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
      &plInternal::QueryHelper::FrustumQueryCallback<false, true>,
      &plInternal::QueryHelper::FrustumQueryCallback<true, true>,
      &queryData, visType);
  }
  else
  {
    ForEachCellInBoxInMatchingGrids(simdBox, queryParams,
      &plInternal::QueryHelper::FrustumQueryCallback<false, false>,
      &plInternal::QueryHelper::FrustumQueryCallback<true, false>,
      &queryData, visType);
  }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  if (queryParams.m_pStats != nullptr)
  {
    queryParams.m_pStats->m_TimeTaken = timer.GetRunningTotal();
  }
#endif
}

plVisibilityState plSpatialSystem_RegularGrid::GetVisibilityState(const plSpatialDataHandle& hData, plUInt32 uiNumFramesBeforeInvisible) const
{
  Data* pData = nullptr;
  PLASMA_VERIFY(m_DataTable.TryGetValue(hData.GetInternalID(), pData), "Invalid spatial data handle");

  if (IsAlwaysVisibleData(*pData))
    return plVisibilityState::Direct;

  plUInt64 uiLastVisibleFrameIdxAndVisType = 0;
  ForEachGrid(*pData, hData,
    [&](const Grid& grid, const CellDataMapping& mapping) {
      auto& pCell = grid.m_Cells[mapping.m_uiCellIndex];
      uiLastVisibleFrameIdxAndVisType = plMath::Max<plUInt64>(uiLastVisibleFrameIdxAndVisType, pCell->m_LastVisibleFrameIdxAndVisType[mapping.m_uiCellDataIndex]);
      return plVisitorExecution::Continue;
    });

  const plUInt64 uiLastVisibleFrameIdx = (uiLastVisibleFrameIdxAndVisType >> 4);
  const plUInt64 uiLastVisibilityType = (uiLastVisibleFrameIdxAndVisType & static_cast<plUInt64>(15)); // mask out lower 4 bits

  if (m_uiFrameCounter > uiLastVisibleFrameIdx + uiNumFramesBeforeInvisible)
    return plVisibilityState::Invisible;

  return static_cast<plVisibilityState>(uiLastVisibilityType);
}

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
void plSpatialSystem_RegularGrid::GetInternalStats(plStringBuilder& sb) const
{
  sb = "Cache Candidates:\n";

  PLASMA_LOCK(m_CacheCandidatesMutex);

  for (auto& sortedCandidate : m_SortedCacheCandidates)
  {
    auto& candidate = m_CacheCandidates[sortedCandidate.m_uiIndex];

    sb.AppendFormat(" \nCategory: {}\nInclude Tags: ", candidate.m_Category.m_uiValue);
    TagsToString(candidate.m_IncludeTags, sb);
    sb.Append("\nExclude Tags: ");
    TagsToString(candidate.m_ExcludeTags, sb);
    sb.AppendFormat("\nScore: {}", plArgF(sortedCandidate.m_fScore, 2));

    const plUInt32 uiGridIndex = candidate.m_uiGridIndex;
    if (uiGridIndex != plInvalidIndex)
    {
      auto& pGrid = m_Grids[uiGridIndex];
      if (pGrid->CachingCompleted())
      {
        sb.Append("\nReady to use!\n");
      }
      else
      {
        const plUInt32 uiNumObjectsMigrated = pGrid->m_uiLastMigrationIndex;
        sb.AppendFormat("\nMigration Status: {}%%\n", plArgF(float(uiNumObjectsMigrated) / m_DataTable.GetCount() * 100.0f, 2));
      }
    }
  }
}
#endif

PLASMA_ALWAYS_INLINE bool plSpatialSystem_RegularGrid::IsAlwaysVisibleData(const Data& data) const
{
  return data.m_uiAlwaysVisible != 0;
}

plSpatialDataHandle plSpatialSystem_RegularGrid::AddSpatialDataToGrids(const plSimdBBoxSphere& bounds, plGameObject* pObject, plUInt32 uiCategoryBitmask, const plTagSet& tags, bool bAlwaysVisible)
{
  Data data;
  data.m_uiGridBitmask = uiCategoryBitmask;
  data.m_uiAlwaysVisible = bAlwaysVisible ? 1 : 0;

  // find matching cached grids and add them to data.m_uiGridBitmask
  for (plUInt32 uiCachedGridIndex = m_uiFirstCachedGridIndex; uiCachedGridIndex < m_Grids.GetCount(); ++uiCachedGridIndex)
  {
    auto& pGrid = m_Grids[uiCachedGridIndex];
    if (pGrid == nullptr)
      continue;

    if ((pGrid->m_Category.GetBitmask() & uiCategoryBitmask) == 0 ||
        FilterByTags(tags, pGrid->m_IncludeTags, pGrid->m_ExcludeTags))
      continue;

    data.m_uiGridBitmask |= PLASMA_BIT(uiCachedGridIndex);
  }

  auto hData = plSpatialDataHandle(m_DataTable.Insert(data));

  plUInt64 uiGridBitmask = data.m_uiGridBitmask;
  while (uiGridBitmask > 0)
  {
    plUInt32 uiGridIndex = plMath::FirstBitLow(uiGridBitmask);
    uiGridBitmask &= uiGridBitmask - 1;

    auto& pGrid = m_Grids[uiGridIndex];
    if (pGrid == nullptr)
    {
      pGrid = PLASMA_NEW(&m_Allocator, Grid, *this, plSpatialData::Category(uiGridIndex));
    }

    pGrid->AddSpatialData(bounds, tags, pObject, m_uiFrameCounter, hData);
  }

  return hData;
}

template <typename Functor>
PLASMA_FORCE_INLINE void plSpatialSystem_RegularGrid::ForEachGrid(const Data& data, const plSpatialDataHandle& hData, Functor func) const
{
  plUInt64 uiGridBitmask = data.m_uiGridBitmask;
  plUInt32 uiDataIndex = hData.GetInternalID().m_InstanceIndex;

  while (uiGridBitmask > 0)
  {
    plUInt32 uiGridIndex = plMath::FirstBitLow(uiGridBitmask);
    uiGridBitmask &= uiGridBitmask - 1;

    auto& grid = *m_Grids[uiGridIndex];
    auto& mapping = grid.m_CellDataMappings[uiDataIndex];

    if (func(grid, mapping) == plVisitorExecution::Stop)
      break;
  }
}

void plSpatialSystem_RegularGrid::ForEachCellInBoxInMatchingGrids(const plSimdBBox& box, const QueryParams& queryParams, CellCallback noFilterCallback, CellCallback filterByTagsCallback, void* pUserData, plVisibilityState visType) const
{
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  if (queryParams.m_pStats != nullptr)
  {
    queryParams.m_pStats->m_uiTotalNumObjects = m_DataTable.GetCount();
  }
#endif

  plUInt32 uiGridBitmask = queryParams.m_uiCategoryBitmask;

  // search for cached grids that match the exact query params first
  for (plUInt32 uiCachedGridIndex = m_uiFirstCachedGridIndex; uiCachedGridIndex < m_Grids.GetCount(); ++uiCachedGridIndex)
  {
    auto& pGrid = m_Grids[uiCachedGridIndex];
    if (pGrid == nullptr || pGrid->CachingCompleted() == false)
      continue;

    if ((pGrid->m_Category.GetBitmask() & uiGridBitmask) == 0 ||
        pGrid->m_IncludeTags != queryParams.m_IncludeTags ||
        pGrid->m_ExcludeTags != queryParams.m_ExcludeTags)
      continue;

    uiGridBitmask &= ~pGrid->m_Category.GetBitmask();

    Stats stats;
    pGrid->ForEachCellInBox(box,
      [&](const Cell& cell) {
        return noFilterCallback(cell, queryParams, stats, pUserData, visType);
      });

    UpdateCacheCandidate(queryParams.m_IncludeTags, queryParams.m_ExcludeTags, pGrid->m_Category, 0.0f);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
    if (queryParams.m_pStats != nullptr)
    {
      queryParams.m_pStats->m_uiNumObjectsTested += stats.m_uiNumObjectsTested;
      queryParams.m_pStats->m_uiNumObjectsPassed += stats.m_uiNumObjectsPassed;
    }
#endif
  }

  // then search for the rest
  const bool useTagsFilter = queryParams.m_IncludeTags.IsEmpty() == false || queryParams.m_ExcludeTags.IsEmpty() == false;
  CellCallback cellCallback = useTagsFilter ? filterByTagsCallback : noFilterCallback;

  while (uiGridBitmask > 0)
  {
    plUInt32 uiGridIndex = plMath::FirstBitLow(uiGridBitmask);
    uiGridBitmask &= uiGridBitmask - 1;

    auto& pGrid = m_Grids[uiGridIndex];
    if (pGrid == nullptr)
      continue;

    Stats stats;
    pGrid->ForEachCellInBox(box,
      [&](const Cell& cell) {
        return cellCallback(cell, queryParams, stats, pUserData, visType);
      });

    if (pGrid->m_bCanBeCached && useTagsFilter)
    {
      const plUInt32 totalNumObjectsAfterSpatialTest = stats.m_uiNumObjectsFiltered + stats.m_uiNumObjectsPassed;
      const plUInt32 cacheThreshold = plUInt32(plMath::Max(cvar_SpatialQueriesCachingThreshold.GetValue(), 1));

      // 1.0 => all objects filtered, 0.0 => no object filtered by tags
      const float filteredRatio = float(double(stats.m_uiNumObjectsFiltered) / totalNumObjectsAfterSpatialTest);

      // Doesn't make sense to cache if there are only few objects in total or only few objects have been filtered
      if (totalNumObjectsAfterSpatialTest > cacheThreshold && filteredRatio > 0.1f)
      {
        UpdateCacheCandidate(queryParams.m_IncludeTags, queryParams.m_ExcludeTags, pGrid->m_Category, filteredRatio);
      }
    }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
    if (queryParams.m_pStats != nullptr)
    {
      queryParams.m_pStats->m_uiNumObjectsTested += stats.m_uiNumObjectsTested;
      queryParams.m_pStats->m_uiNumObjectsPassed += stats.m_uiNumObjectsPassed;
    }
#endif
  }
}

void plSpatialSystem_RegularGrid::MigrateCachedGrid(plUInt32 uiCandidateIndex)
{
  plUInt32 uiTargetGridIndex = plInvalidIndex;
  plUInt32 uiSourceGridIndex = plInvalidIndex;

  {
    PLASMA_LOCK(m_CacheCandidatesMutex);

    auto& cacheCandidate = m_CacheCandidates[uiCandidateIndex];
    uiTargetGridIndex = cacheCandidate.m_uiGridIndex;
    uiSourceGridIndex = cacheCandidate.m_Category.m_uiValue;

    if (uiTargetGridIndex == plInvalidIndex)
    {
      for (plUInt32 i = m_Grids.GetCount() - 1; i >= MAX_NUM_REGULAR_GRIDS; --i)
      {
        if (m_Grids[i] == nullptr)
        {
          uiTargetGridIndex = i;
          break;
        }
      }

      PLASMA_ASSERT_DEBUG(uiTargetGridIndex != plInvalidIndex, "No free cached grid");
      cacheCandidate.m_uiGridIndex = uiTargetGridIndex;

      auto pGrid = PLASMA_NEW(&m_Allocator, Grid, *this, cacheCandidate.m_Category);
      pGrid->m_IncludeTags = cacheCandidate.m_IncludeTags;
      pGrid->m_ExcludeTags = cacheCandidate.m_ExcludeTags;

      m_Grids[uiTargetGridIndex] = pGrid;

      m_uiFirstCachedGridIndex = plMath::Min(m_uiFirstCachedGridIndex, uiTargetGridIndex);
    }
  }

  MigrateSpatialData(uiTargetGridIndex, uiSourceGridIndex);
}

void plSpatialSystem_RegularGrid::MigrateSpatialData(plUInt32 uiTargetGridIndex, plUInt32 uiSourceGridIndex)
{
  auto& pTargetGrid = m_Grids[uiTargetGridIndex];
  if (pTargetGrid->CachingCompleted())
    return;

  auto& pSourceGrid = m_Grids[uiSourceGridIndex];

  constexpr plUInt32 uiNumObjectsPerStep = 64;
  plUInt32& uiLastMigrationIndex = pTargetGrid->m_uiLastMigrationIndex;
  const plUInt32 uiSourceCount = pSourceGrid->m_CellDataMappings.GetCount();
  const plUInt32 uiEndIndex = plMath::Min(uiLastMigrationIndex + uiNumObjectsPerStep, uiSourceCount);

  for (plUInt32 i = uiLastMigrationIndex; i < uiEndIndex; ++i)
  {
    if (pTargetGrid->MigrateSpatialDataFromOtherGrid(i, *pSourceGrid))
    {
      m_DataTable.GetValueUnchecked(i).m_uiGridBitmask |= PLASMA_BIT(uiTargetGridIndex);
    }
  }

  uiLastMigrationIndex = (uiEndIndex == uiSourceCount) ? plInvalidIndex : uiEndIndex;
}

void plSpatialSystem_RegularGrid::RemoveCachedGrid(plUInt32 uiCandidateIndex)
{
  plUInt32 uiGridIndex;

  {
    PLASMA_LOCK(m_CacheCandidatesMutex);

    auto& cacheCandidate = m_CacheCandidates[uiCandidateIndex];
    uiGridIndex = cacheCandidate.m_uiGridIndex;

    if (uiGridIndex == plInvalidIndex)
      return;

    cacheCandidate.m_fQueryCount = 0.0f;
    cacheCandidate.m_fFilteredRatio = 0.0f;
    cacheCandidate.m_uiGridIndex = plInvalidIndex;
  }

  m_Grids[uiGridIndex] = nullptr;
}

void plSpatialSystem_RegularGrid::RemoveAllCachedGrids()
{
  PLASMA_LOCK(m_CacheCandidatesMutex);

  for (plUInt32 i = 0; i < m_CacheCandidates.GetCount(); ++i)
  {
    RemoveCachedGrid(i);
  }
}

void plSpatialSystem_RegularGrid::UpdateCacheCandidate(const plTagSet& includeTags, const plTagSet& excludeTags, plSpatialData::Category category, float filteredRatio) const
{
  PLASMA_LOCK(m_CacheCandidatesMutex);

  CacheCandidate* pCacheCandiate = nullptr;
  for (auto& cacheCandidate : m_CacheCandidates)
  {
    if (cacheCandidate.m_Category == category &&
        cacheCandidate.m_IncludeTags == includeTags &&
        cacheCandidate.m_ExcludeTags == excludeTags)
    {
      pCacheCandiate = &cacheCandidate;
      break;
    }
  }

  if (pCacheCandiate != nullptr)
  {
    pCacheCandiate->m_fQueryCount = plMath::Min(pCacheCandiate->m_fQueryCount + 1.0f, 100.0f);
    pCacheCandiate->m_fFilteredRatio = plMath::Max(pCacheCandiate->m_fFilteredRatio, filteredRatio);
  }
  else
  {
    m_CacheCandidates.PushBack({includeTags, excludeTags, category, 1, filteredRatio});
  }
}

PLASMA_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem_RegularGrid);

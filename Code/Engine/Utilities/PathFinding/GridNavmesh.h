#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Rect.h>
#include <Utilities/DataStructures/GameGrid.h>

/// \brief Takes an plGameGrid and creates an optimized navmesh structure from it, that is more efficient for path searches.
class PL_UTILITIES_DLL plGridNavmesh
{
public:
  struct ConvexArea
  {
    PL_DECLARE_POD_TYPE();

    /// The space that is enclosed by this convex area.
    plRectU32 m_Rect;

    /// The first AreaEdge that belongs to this ConvexArea.
    plUInt32 m_uiFirstEdge;

    /// The number of AreaEdge's that belong to this ConvexArea.
    plUInt32 m_uiNumEdges;
  };

  struct AreaEdge
  {
    PL_DECLARE_POD_TYPE();

    /// The 'area' of the edge. This is a one cell wide line that is always WITHIN the ConvexArea from where the edge connects to a neighbor
    /// area.
    plRectU16 m_EdgeRect;

    /// The index of the area that can be reached over this edge. This is always a valid index.
    plInt32 m_iNeighborArea;
  };

  /// \brief Callback that determines whether the cell with index \a uiCell1 and the cell with index \a uiCell2 represent the same type of
  /// terrain.
  using CellComparator = bool (*)(plUInt32, plUInt32, void*);

  /// \brief Callback that determines whether the cell with index \a uiCell is blocked entirely (for every type of unit) and therefore can
  /// be optimized away.
  using CellBlocked = bool (*)(plUInt32, void*);

  /// \brief Creates the navmesh from the given plGameGrid.
  template <class CellData>
  void CreateFromGrid(
    const plGameGrid<CellData>& grid, CellComparator isSameCellType, void* pPassThroughSame, CellBlocked isCellBlocked, void* pPassThroughBlocked);

  /// \brief Returns the index of the ConvexArea at the given cell coordinates. Negative, if the cell is blocked.
  plInt32 GetAreaAt(const plVec2I32& vCoord) const { return m_NodesGrid.GetCell(vCoord); }

  /// \brief Returns the number of convex areas that this navmesh consists of.
  plUInt32 GetNumConvexAreas() const { return m_ConvexAreas.GetCount(); }

  /// \brief Returns the given convex area by index.
  const ConvexArea& GetConvexArea(plInt32 iArea) const { return m_ConvexAreas[iArea]; }

  /// \brief Returns the number of edges between convex areas.
  plUInt32 GetNumAreaEdges() const { return m_GraphEdges.GetCount(); }

  /// \brief Returns the given area edge by index.
  const AreaEdge& GetAreaEdge(plInt32 iAreaEdge) const { return m_GraphEdges[iAreaEdge]; }

private:
  void UpdateRegion(plRectU32 region, CellComparator IsSameCellType, void* pPassThrough1, CellBlocked IsCellBlocked, void* pPassThrough2);

  void Optimize(plRectU32 region, CellComparator IsSameCellType, void* pPassThrough);
  bool OptimizeBoxes(plRectU32 region, CellComparator IsSameCellType, void* pPassThrough, plUInt32 uiIntervalX, plUInt32 uiIntervalY,
    plUInt32 uiWidth, plUInt32 uiHeight, plUInt32 uiOffsetX = 0, plUInt32 uiOffsetY = 0);
  bool CanCreateArea(plRectU32 region, CellComparator IsSameCellType, void* pPassThrough) const;

  bool CanMergeRight(plInt32 x, plInt32 y, CellComparator IsSameCellType, void* pPassThrough, plRectU32& out_Result) const;
  bool CanMergeDown(plInt32 x, plInt32 y, CellComparator IsSameCellType, void* pPassThrough, plRectU32& out_Result) const;
  bool MergeBestFit(plRectU32 region, CellComparator IsSameCellType, void* pPassThrough);

  void CreateGraphEdges();
  void CreateGraphEdges(ConvexArea& Area);

  plRectU32 GetCellBBox(plInt32 x, plInt32 y) const;
  void Merge(const plRectU32& rect);
  void CreateNodes(plRectU32 region, CellBlocked IsCellBlocked, void* pPassThrough);

  plGameGrid<plInt32> m_NodesGrid;
  plDynamicArray<ConvexArea> m_ConvexAreas;
  plDeque<AreaEdge> m_GraphEdges;
};

#include <Utilities/PathFinding/Implementation/GridNavmesh_inl.h>

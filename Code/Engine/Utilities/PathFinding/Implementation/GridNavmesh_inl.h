#pragma once

template <class CellData>
void plGridNavmesh::CreateFromGrid(
  const plGameGrid<CellData>& grid, CellComparator isSameCellType, void* pPassThrough, CellBlocked isCellBlocked, void* pPassThrough2)
{
  m_NodesGrid.CreateGrid(grid.GetGridSizeX(), grid.GetGridSizeY());

  UpdateRegion(plRectU32(grid.GetGridSizeX(), grid.GetGridSizeY()), isSameCellType, pPassThrough, isCellBlocked, pPassThrough2);

  CreateGraphEdges();
}

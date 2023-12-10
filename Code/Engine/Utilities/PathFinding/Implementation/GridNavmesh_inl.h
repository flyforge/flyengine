#pragma once

template <class CellData>
void plGridNavmesh::CreateFromGrid(
  const plGameGrid<CellData>& Grid, CellComparator IsSameCellType, void* pPassThrough, CellBlocked IsCellBlocked, void* pPassThrough2)
{
  m_NodesGrid.CreateGrid(Grid.GetGridSizeX(), Grid.GetGridSizeY());

  UpdateRegion(plRectU32(Grid.GetGridSizeX(), Grid.GetGridSizeY()), IsSameCellType, pPassThrough, IsCellBlocked, pPassThrough2);

  CreateGraphEdges();
}

#include <Utilities/UtilitiesPCH.h>

#include <Utilities/PathFinding/GridNavmesh.h>

void plGridNavmesh::UpdateRegion(plRectU32 region, CellComparator IsSameCellType, void* pPassThrough1, CellBlocked IsCellBlocked, void* pPassThrough2)
{
  plInt32 iInvalidNode = -(plInt32)m_ConvexAreas.GetCount();

  // initialize with 'invalid'
  for (plUInt32 y = region.y; y < region.y + region.height; ++y)
  {
    for (plUInt32 x = region.x; x < region.x + region.width; ++x)
    {
      --iInvalidNode;
      m_NodesGrid.GetCell(plVec2I32(x, y)) = iInvalidNode;
    }
  }

  Optimize(region, IsSameCellType, pPassThrough1);

  CreateNodes(region, IsCellBlocked, pPassThrough2);
}


plRectU32 plGridNavmesh::GetCellBBox(plInt32 x, plInt32 y) const
{
  const plInt32 iCellNode = m_NodesGrid.GetCell(plVec2I32(x, y));

  plRectU32 r;
  r.x = x;
  r.y = y;
  r.width = 1;
  r.height = 1;

  while (r.x > 0)
  {
    if (iCellNode != m_NodesGrid.GetCell(plVec2I32(r.x - 1, y)))
      break;

    r.x--;
  }

  while (r.y > 0)
  {
    if (iCellNode != m_NodesGrid.GetCell(plVec2I32(x, r.y - 1)))
      break;

    r.y--;
  }

  while (x < m_NodesGrid.GetGridSizeX() - 1)
  {
    if (iCellNode != m_NodesGrid.GetCell(plVec2I32(x + 1, y)))
      break;

    x++;
  }

  while (y < m_NodesGrid.GetGridSizeY() - 1)
  {
    if (iCellNode != m_NodesGrid.GetCell(plVec2I32(x, y + 1)))
      break;

    y++;
  }

  r.width = x - r.x + 1;
  r.height = y - r.y + 1;

  return r;
}

void plGridNavmesh::Merge(const plRectU32& rect)
{
  const plInt32 iCellNode = m_NodesGrid.GetCell(plVec2I32(rect.x, rect.y));

  for (plUInt32 y = rect.y; y < rect.y + rect.height; ++y)
  {
    for (plUInt32 x = rect.x; x < rect.x + rect.width; ++x)
    {
      m_NodesGrid.GetCell(plVec2I32(x, y)) = iCellNode;
    }
  }
}

void plGridNavmesh::CreateNodes(plRectU32 region, CellBlocked IsCellBlocked, void* pPassThrough)
{
  for (plUInt32 y = region.y; y < region.y + region.height; ++y)
  {
    for (plUInt32 x = region.x; x < region.x + region.width; ++x)
    {
      const plInt32 iCellNode = m_NodesGrid.GetCell(plVec2I32(x, y));

      if (iCellNode >= 0)
        continue;

      if (IsCellBlocked(m_NodesGrid.ConvertCellCoordinateToIndex(plVec2I32(x, y)), pPassThrough))
      {
        m_NodesGrid.GetCell(plVec2I32(x, y)) = -1;
        continue;
      }

      ConvexArea a;
      a.m_Rect = GetCellBBox(x, y);

      m_NodesGrid.GetCell(plVec2I32(a.m_Rect.x, a.m_Rect.y)) = m_ConvexAreas.GetCount();
      m_ConvexAreas.PushBack(a);

      Merge(a.m_Rect);
    }
  }
}


void plGridNavmesh::Optimize(plRectU32 region, CellComparator IsSameCellType, void* pPassThrough)
{
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 8, 8, 8, 8))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 4, 4, 4, 4))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 2, 2, 2, 2))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 4, 2, 3, 2))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 2, 4, 2, 3))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 4, 2, 3, 2, 1, 0))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 2, 4, 2, 3, 0, 1))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 4, 2, 4, 2))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 2, 4, 2, 4))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 8, 4, 6, 4))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 4, 8, 4, 6))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 8, 4, 6, 4, 2, 0))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 4, 8, 4, 6, 0, 2))
  {
  }


  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 1, 1, 4, 2))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 1, 1, 2, 4))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 1, 1, 3, 2))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 1, 1, 2, 3))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 1, 1, 2, 2))
  {
  }

  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 1, 1, 2, 1))
  {
  }
  if (OptimizeBoxes(region, IsSameCellType, pPassThrough, 1, 1, 1, 2))
  {
  }

  while (MergeBestFit(region, IsSameCellType, pPassThrough))
  {
  }
}

bool plGridNavmesh::CanCreateArea(plRectU32 region, CellComparator IsSameCellType, void* pPassThrough) const
{
  if (region.x + region.width > m_NodesGrid.GetGridSizeX())
    return false;
  if (region.y + region.height > m_NodesGrid.GetGridSizeY())
    return false;

  const plUInt32 uiStartNode = m_NodesGrid.ConvertCellCoordinateToIndex(plVec2I32(region.x, region.y));
  // const plInt32 iStartNodeArea = m_NodesGrid.GetCell(uiStartNode);

  for (plUInt32 y = region.y; y < region.y + region.height; ++y)
  {
    for (plUInt32 x = region.x; x < region.x + region.width; ++x)
    {
      const plUInt32 uiCurNode = m_NodesGrid.ConvertCellCoordinateToIndex(plVec2I32(x, y));
      // const plInt32 iCurNodeArea = m_NodesGrid.GetCell(uiCurNode);

      // if (iCurNodeArea == iStartNodeArea)
      // continue;

      if (!IsSameCellType(uiStartNode, uiCurNode, pPassThrough))
        return false;

      const plRectU32 rect = GetCellBBox(x, y);

      if (rect.x < region.x || rect.y < region.y)
        return false;
      if (rect.x + rect.width > region.x + region.width)
        return false;
      if (rect.y + rect.height > region.y + region.height)
        return false;
    }
  }

  return true;
}

bool plGridNavmesh::OptimizeBoxes(plRectU32 region, CellComparator IsSameCellType, void* pPassThrough, plUInt32 uiIntervalX, plUInt32 uiIntervalY,
  plUInt32 uiWidth, plUInt32 uiHeight, plUInt32 uiOffsetX, plUInt32 uiOffsetY)
{
  bool bMergedAny = false;

  for (plUInt32 y = region.y; y < region.y + region.height; y += uiIntervalY)
  {
    for (plUInt32 x = region.x; x < region.x + region.width; x += uiIntervalX)
    {
      plRectU32 NewArea;
      NewArea.x = x + uiOffsetX;
      NewArea.y = y + uiOffsetY;
      NewArea.width = uiWidth;
      NewArea.height = uiHeight;

      if (CanCreateArea(NewArea, IsSameCellType, pPassThrough))
      {
        bMergedAny = true;

        Merge(NewArea);
      }
    }
  }

  return bMergedAny;
}



bool plGridNavmesh::CanMergeRight(plInt32 x, plInt32 y, CellComparator IsSameCellType, void* pPassThrough, plRectU32& out_Result) const
{
  const plRectU32 r1 = GetCellBBox(x, y);

  if (r1.x + r1.width >= m_NodesGrid.GetGridSizeX())
    return false;

  // const plInt32 iCellNode = m_NodesGrid.GetCell(plVec2I32(x, y));

  if (!IsSameCellType(m_NodesGrid.ConvertCellCoordinateToIndex(plVec2I32(x, y)),
        m_NodesGrid.ConvertCellCoordinateToIndex(plVec2I32(r1.x + r1.width, y)), pPassThrough))
    return false;

  const plRectU32 r2 = GetCellBBox(r1.x + r1.width, y);

  if (r1.y != r2.y || r1.height != r2.height)
    return false;

  out_Result.x = r1.x;
  out_Result.y = r1.y;
  out_Result.width = r1.width + r2.width;
  out_Result.height = r1.height;

  return true;
}

bool plGridNavmesh::CanMergeDown(plInt32 x, plInt32 y, CellComparator IsSameCellType, void* pPassThrough, plRectU32& out_Result) const
{
  const plRectU32 r1 = GetCellBBox(x, y);

  if (r1.y + r1.height >= m_NodesGrid.GetGridSizeY())
    return false;

  // const plInt32 iCellNode = m_NodesGrid.GetCell(plVec2I32(x, y));

  if (!IsSameCellType(m_NodesGrid.ConvertCellCoordinateToIndex(plVec2I32(x, y)),
        m_NodesGrid.ConvertCellCoordinateToIndex(plVec2I32(x, r1.y + r1.height)), pPassThrough))
    return false;

  const plRectU32 r2 = GetCellBBox(x, r1.y + r1.height);

  if (r1.x != r2.x || r1.width != r2.width)
    return false;

  out_Result.x = r1.x;
  out_Result.y = r1.y;
  out_Result.width = r1.width;
  out_Result.height = r1.height + r2.height;

  return true;
}


bool plGridNavmesh::MergeBestFit(plRectU32 region, CellComparator IsSameCellType, void* pPassThrough)
{
  bool bMergedAny = false;

  for (plUInt32 y = region.y; y < region.y + region.height; y += 1)
  {
    for (plUInt32 x = region.x; x < region.x + region.width; x += 1)
    {
      const plInt32 iCellNode = m_NodesGrid.GetCell(plVec2I32(x, y));

      if (iCellNode >= 0)
        continue;

      plRectU32 rd;
      bool bRD = false;
      if (CanMergeDown(x, y, IsSameCellType, pPassThrough, rd))
      {
        if (rd.x == x && rd.y == y)
          bRD = true;
      }

      plRectU32 rr;
      bool bRR = false;
      if (CanMergeRight(x, y, IsSameCellType, pPassThrough, rr))
      {
        if (rr.x == x && rr.y == y)
          bRR = true;
      }

      if (bRR && bRD)
      {
        const float fRatioRR = (float)plMath::Max(rr.width, rr.height) / (float)plMath::Min(rr.width, rr.height);
        const float fRatioRD = (float)plMath::Max(rd.width, rd.height) / (float)plMath::Min(rd.width, rd.height);

        if (fRatioRR < fRatioRD)
          bRD = false;
        else
          bRR = false;
      }

      if (bRR)
      {
        // const float fRatio = (float) plMath::Max(rr.width,  rr.height) / (float) plMath::Min(rr.width,  rr.height);

        bMergedAny = true;
        Merge(rr);
      }

      if (bRD)
      {
        // const float fRatio = (float) plMath::Max(rd.width,  rd.height) / (float) plMath::Min(rd.width,  rd.height);

        Merge(rd);
        bMergedAny = true;
      }
    }
  }

  return bMergedAny;
}

void plGridNavmesh::CreateGraphEdges()
{
  m_GraphEdges.Clear();

  for (plUInt32 i = 0; i < m_ConvexAreas.GetCount(); ++i)
    CreateGraphEdges(m_ConvexAreas[i]);
}

void plGridNavmesh::CreateGraphEdges(ConvexArea& Area)
{
  Area.m_uiFirstEdge = m_GraphEdges.GetCount();
  Area.m_uiNumEdges = 0;

  if (Area.m_Rect.y > 0)
  {
    AreaEdge e;
    e.m_EdgeRect.x = static_cast<plUInt16>(Area.m_Rect.x);
    e.m_EdgeRect.y = static_cast<plUInt16>(Area.m_Rect.y);
    e.m_EdgeRect.width = 1;
    e.m_EdgeRect.height = 1;
    e.m_iNeighborArea = m_NodesGrid.GetCell(plVec2I32(Area.m_Rect.x, Area.m_Rect.y - 1));

    for (plUInt32 x = Area.m_Rect.x + 1; x < Area.m_Rect.x + Area.m_Rect.width; ++x)
    {
      const plInt32 iThisNeighbor = m_NodesGrid.GetCell(plVec2I32(x, Area.m_Rect.y - 1));

      if (e.m_iNeighborArea != iThisNeighbor)
      {
        if (e.m_iNeighborArea >= 0)
        {
          m_GraphEdges.PushBack(e);
          ++Area.m_uiNumEdges;
        }

        e.m_iNeighborArea = iThisNeighbor;
        e.m_EdgeRect.x = static_cast<plUInt16>(x);
        e.m_EdgeRect.width = 0;
      }

      ++e.m_EdgeRect.width;
    }

    if (e.m_iNeighborArea >= 0)
    {
      m_GraphEdges.PushBack(e);
      ++Area.m_uiNumEdges;
    }
  }

  if (Area.m_Rect.y + Area.m_Rect.height < m_NodesGrid.GetGridSizeY())
  {
    AreaEdge e;
    e.m_EdgeRect.x = static_cast<plUInt16>(Area.m_Rect.x);
    e.m_EdgeRect.y = static_cast<plUInt16>(Area.m_Rect.y + Area.m_Rect.height - 1);
    e.m_EdgeRect.width = 1;
    e.m_EdgeRect.height = 1;
    e.m_iNeighborArea = m_NodesGrid.GetCell(plVec2I32(Area.m_Rect.x, Area.m_Rect.y + Area.m_Rect.height));

    for (plUInt32 x = Area.m_Rect.x + 1; x < Area.m_Rect.x + Area.m_Rect.width; ++x)
    {
      const plInt32 iThisNeighbor = m_NodesGrid.GetCell(plVec2I32(x, Area.m_Rect.y + Area.m_Rect.height));

      if (e.m_iNeighborArea != iThisNeighbor)
      {
        if (e.m_iNeighborArea >= 0)
        {
          m_GraphEdges.PushBack(e);
          ++Area.m_uiNumEdges;
        }

        e.m_iNeighborArea = iThisNeighbor;
        e.m_EdgeRect.x = static_cast<plUInt16>(x);
        e.m_EdgeRect.width = 0;
      }

      ++e.m_EdgeRect.width;
    }

    if (e.m_iNeighborArea >= 0)
    {
      m_GraphEdges.PushBack(e);
      ++Area.m_uiNumEdges;
    }
  }

  if (Area.m_Rect.x > 0)
  {
    AreaEdge e;
    e.m_EdgeRect.x = static_cast<plUInt16>(Area.m_Rect.x);
    e.m_EdgeRect.y = static_cast<plUInt16>(Area.m_Rect.y);
    e.m_EdgeRect.width = 1;
    e.m_EdgeRect.height = 1;
    e.m_iNeighborArea = m_NodesGrid.GetCell(plVec2I32(Area.m_Rect.x - 1, Area.m_Rect.y));

    for (plUInt32 y = Area.m_Rect.y + 1; y < Area.m_Rect.y + Area.m_Rect.height; ++y)
    {
      const plInt32 iThisNeighbor = m_NodesGrid.GetCell(plVec2I32(Area.m_Rect.x - 1, y));

      if (e.m_iNeighborArea != iThisNeighbor)
      {
        if (e.m_iNeighborArea >= 0)
        {
          m_GraphEdges.PushBack(e);
          ++Area.m_uiNumEdges;
        }

        e.m_iNeighborArea = iThisNeighbor;
        e.m_EdgeRect.y = static_cast<plUInt16>(y);
        e.m_EdgeRect.height = 0;
      }

      ++e.m_EdgeRect.height;
    }

    if (e.m_iNeighborArea >= 0)
    {
      m_GraphEdges.PushBack(e);
      ++Area.m_uiNumEdges;
    }
  }

  if (Area.m_Rect.x + Area.m_Rect.width < m_NodesGrid.GetGridSizeX())
  {
    AreaEdge e;
    e.m_EdgeRect.x = static_cast<plUInt16>(Area.m_Rect.x + Area.m_Rect.width - 1);
    e.m_EdgeRect.y = static_cast<plUInt16>(Area.m_Rect.y);
    e.m_EdgeRect.width = 1;
    e.m_EdgeRect.height = 1;
    e.m_iNeighborArea = m_NodesGrid.GetCell(plVec2I32(Area.m_Rect.x + Area.m_Rect.width, Area.m_Rect.y));

    for (plUInt32 y = Area.m_Rect.y + 1; y < Area.m_Rect.y + Area.m_Rect.height; ++y)
    {
      const plInt32 iThisNeighbor = m_NodesGrid.GetCell(plVec2I32(Area.m_Rect.x + Area.m_Rect.width, y));

      if (e.m_iNeighborArea != iThisNeighbor)
      {
        if (e.m_iNeighborArea >= 0)
        {
          m_GraphEdges.PushBack(e);
          ++Area.m_uiNumEdges;
        }

        e.m_iNeighborArea = iThisNeighbor;
        e.m_EdgeRect.y = static_cast<plUInt16>(y);
        e.m_EdgeRect.height = 0;
      }

      ++e.m_EdgeRect.height;
    }

    if (e.m_iNeighborArea >= 0)
    {
      m_GraphEdges.PushBack(e);
      ++Area.m_uiNumEdges;
    }
  }
}



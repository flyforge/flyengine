#include <Utilities/UtilitiesPCH.h>

#include <Utilities/GridAlgorithms/Rasterization.h>

plRasterizationResult::Enum pl2DGridUtils::ComputePointsOnLine(
  plInt32 iStartX, plInt32 iStartY, plInt32 iEndX, plInt32 iEndY, PLASMA_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */)
{
  // Implements Bresenham's line algorithm:
  // http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm

  plInt32 dx = plMath::Abs(iEndX - iStartX);
  plInt32 dy = plMath::Abs(iEndY - iStartY);

  plInt32 sx = (iStartX < iEndX) ? 1 : -1;
  plInt32 sy = (iStartY < iEndY) ? 1 : -1;

  plInt32 err = dx - dy;

  while (true)
  {
    // The user callback can stop the algorithm at any point, if no further points on the line are required
    if (Callback(iStartX, iStartY, pPassThrough) == plCallbackResult::Stop)
      return plRasterizationResult::Aborted;

    if ((iStartX == iEndX) && (iStartY == iEndY))
      return plRasterizationResult::Finished;

    plInt32 e2 = 2 * err;

    if (e2 > -dy)
    {
      err = err - dy;
      iStartX = iStartX + sx;
    }

    if (e2 < dx)
    {
      err = err + dx;
      iStartY = iStartY + sy;
    }
  }
}

plRasterizationResult::Enum pl2DGridUtils::ComputePointsOnLineConservative(plInt32 iStartX, plInt32 iStartY, plInt32 iEndX, plInt32 iEndY,
  PLASMA_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */, bool bVisitBothNeighbors /* = false */)
{
  plInt32 dx = plMath::Abs(iEndX - iStartX);
  plInt32 dy = plMath::Abs(iEndY - iStartY);

  plInt32 sx = (iStartX < iEndX) ? 1 : -1;
  plInt32 sy = (iStartY < iEndY) ? 1 : -1;

  plInt32 err = dx - dy;

  plInt32 iLastX = iStartX;
  plInt32 iLastY = iStartY;

  while (true)
  {
    // if this is going to be a diagonal step, make sure to insert horizontal/vertical steps

    if ((plMath::Abs(iLastX - iStartX) + plMath::Abs(iLastY - iStartY)) == 2)
    {
      // This part is the difference to the non-conservative line algorithm

      if (Callback(iLastX, iStartY, pPassThrough) == plCallbackResult::Continue)
      {
        // first one succeeded, going to continue

        // if this is true, the user still wants a callback for the alternative, even though it does not change the outcome anymore
        if (bVisitBothNeighbors)
          Callback(iStartX, iLastY, pPassThrough);
      }
      else
      {
        // first one failed, try the second
        if (Callback(iStartX, iLastY, pPassThrough) == plCallbackResult::Stop)
          return plRasterizationResult::Aborted;
      }
    }

    iLastX = iStartX;
    iLastY = iStartY;

    // The user callback can stop the algorithm at any point, if no further points on the line are required
    if (Callback(iStartX, iStartY, pPassThrough) == plCallbackResult::Stop)
      return plRasterizationResult::Aborted;

    if ((iStartX == iEndX) && (iStartY == iEndY))
      return plRasterizationResult::Finished;

    plInt32 e2 = 2 * err;

    if (e2 > -dy)
    {
      err = err - dy;
      iStartX = iStartX + sx;
    }

    if (e2 < dx)
    {
      err = err + dx;
      iStartY = iStartY + sy;
    }
  }
}


plRasterizationResult::Enum pl2DGridUtils::ComputePointsOnCircle(
  plInt32 iStartX, plInt32 iStartY, plUInt32 uiRadius, PLASMA_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */)
{
  int f = 1 - uiRadius;
  int ddF_x = 1;
  int ddF_y = -2 * uiRadius;
  int x = 0;
  int y = uiRadius;

  // report the four extremes
  if (Callback(iStartX, iStartY + uiRadius, pPassThrough) == plCallbackResult::Stop)
    return plRasterizationResult::Aborted;
  if (Callback(iStartX, iStartY - uiRadius, pPassThrough) == plCallbackResult::Stop)
    return plRasterizationResult::Aborted;
  if (Callback(iStartX + uiRadius, iStartY, pPassThrough) == plCallbackResult::Stop)
    return plRasterizationResult::Aborted;
  if (Callback(iStartX - uiRadius, iStartY, pPassThrough) == plCallbackResult::Stop)
    return plRasterizationResult::Aborted;

  // the loop iterates over an eighth of the circle (a 45 degree segment) and then mirrors each point 8 times to fill the entire circle
  while (x < y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    if (Callback(iStartX + x, iStartY + y, pPassThrough) == plCallbackResult::Stop)
      return plRasterizationResult::Aborted;
    if (Callback(iStartX - x, iStartY + y, pPassThrough) == plCallbackResult::Stop)
      return plRasterizationResult::Aborted;
    if (Callback(iStartX + x, iStartY - y, pPassThrough) == plCallbackResult::Stop)
      return plRasterizationResult::Aborted;
    if (Callback(iStartX - x, iStartY - y, pPassThrough) == plCallbackResult::Stop)
      return plRasterizationResult::Aborted;
    if (Callback(iStartX + y, iStartY + x, pPassThrough) == plCallbackResult::Stop)
      return plRasterizationResult::Aborted;
    if (Callback(iStartX - y, iStartY + x, pPassThrough) == plCallbackResult::Stop)
      return plRasterizationResult::Aborted;
    if (Callback(iStartX + y, iStartY - x, pPassThrough) == plCallbackResult::Stop)
      return plRasterizationResult::Aborted;
    if (Callback(iStartX - y, iStartY - x, pPassThrough) == plCallbackResult::Stop)
      return plRasterizationResult::Aborted;
  }

  return plRasterizationResult::Finished;
}

plUInt32 pl2DGridUtils::FloodFill(plInt32 iStartX, plInt32 iStartY, PLASMA_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */,
  plDeque<plVec2I32>* pTempArray /* = nullptr */)
{
  plUInt32 uiFilled = 0;

  plDeque<plVec2I32> FallbackQueue;

  if (pTempArray == nullptr)
    pTempArray = &FallbackQueue;

  pTempArray->Clear();
  pTempArray->PushBack(plVec2I32(iStartX, iStartY));

  while (!pTempArray->IsEmpty())
  {
    plVec2I32 v = pTempArray->PeekBack();
    pTempArray->PopBack();

    if (Callback(v.x, v.y, pPassThrough) == plCallbackResult::Continue)
    {
      ++uiFilled;

      // put the four neighbors into the queue
      pTempArray->PushBack(plVec2I32(v.x - 1, v.y));
      pTempArray->PushBack(plVec2I32(v.x + 1, v.y));
      pTempArray->PushBack(plVec2I32(v.x, v.y - 1));
      pTempArray->PushBack(plVec2I32(v.x, v.y + 1));
    }
  }

  return uiFilled;
}

plUInt32 pl2DGridUtils::FloodFillDiag(plInt32 iStartX, plInt32 iStartY, PLASMA_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /*= nullptr*/,
  plDeque<plVec2I32>* pTempArray /*= nullptr*/)
{
  plUInt32 uiFilled = 0;

  plDeque<plVec2I32> FallbackQueue;

  if (pTempArray == nullptr)
    pTempArray = &FallbackQueue;

  pTempArray->Clear();
  pTempArray->PushBack(plVec2I32(iStartX, iStartY));

  while (!pTempArray->IsEmpty())
  {
    plVec2I32 v = pTempArray->PeekBack();
    pTempArray->PopBack();

    if (Callback(v.x, v.y, pPassThrough) == plCallbackResult::Continue)
    {
      ++uiFilled;

      // put the eight neighbors into the queue
      pTempArray->PushBack(plVec2I32(v.x - 1, v.y));
      pTempArray->PushBack(plVec2I32(v.x + 1, v.y));
      pTempArray->PushBack(plVec2I32(v.x, v.y - 1));
      pTempArray->PushBack(plVec2I32(v.x, v.y + 1));

      pTempArray->PushBack(plVec2I32(v.x - 1, v.y - 1));
      pTempArray->PushBack(plVec2I32(v.x + 1, v.y - 1));
      pTempArray->PushBack(plVec2I32(v.x + 1, v.y + 1));
      pTempArray->PushBack(plVec2I32(v.x - 1, v.y + 1));
    }
  }

  return uiFilled;
}

// Lookup table that describes the shape of the circle
// When rasterizing circles with few pixels algorithms usually don't give nice shapes
// so this lookup table is handcrafted for better results
static const plUInt8 OverlapCircle[15][15] = {{9, 9, 9, 9, 9, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9}, {9, 9, 9, 8, 8, 7, 7, 7, 7, 7, 8, 8, 9, 9, 9},
  {9, 9, 8, 8, 7, 6, 6, 6, 6, 6, 7, 8, 8, 9, 9}, {9, 8, 8, 7, 6, 6, 5, 5, 5, 6, 6, 7, 8, 8, 9}, {9, 8, 7, 6, 6, 5, 4, 4, 4, 5, 6, 6, 7, 8, 9},
  {8, 7, 6, 6, 5, 4, 3, 3, 3, 4, 5, 6, 6, 7, 8}, {8, 7, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 7, 8}, {8, 7, 6, 5, 4, 3, 1, 0, 1, 3, 4, 5, 6, 7, 8},
  {8, 7, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 7, 8}, {8, 7, 6, 6, 5, 4, 3, 3, 3, 4, 5, 6, 6, 7, 8}, {9, 8, 7, 6, 6, 5, 4, 4, 4, 5, 6, 6, 7, 8, 9},
  {9, 8, 8, 7, 6, 6, 5, 5, 5, 6, 6, 7, 8, 8, 9}, {9, 9, 8, 8, 7, 6, 6, 6, 6, 6, 7, 8, 8, 9, 9}, {9, 9, 9, 8, 8, 7, 7, 7, 7, 7, 8, 8, 9, 9, 9},
  {9, 9, 9, 9, 9, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9}};

static const plInt32 CircleCenter = 7;
static const plUInt8 CircleAreaMin[9] = {7, 6, 6, 5, 4, 3, 2, 1, 0};
static const plUInt8 CircleAreaMax[9] = {7, 8, 8, 9, 10, 11, 12, 13, 14};

plRasterizationResult::Enum pl2DGridUtils::RasterizeBlob(
  plInt32 iPosX, plInt32 iPosY, plBlobType eType, PLASMA_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */)
{
  const plUInt8 uiCircleType = plMath::Clamp<plUInt8>(eType, 0, 8);

  const plInt32 iAreaMin = CircleAreaMin[uiCircleType];
  const plInt32 iAreaMax = CircleAreaMax[uiCircleType];

  iPosX -= CircleCenter;
  iPosY -= CircleCenter;

  for (plInt32 y = iAreaMin; y <= iAreaMax; ++y)
  {
    for (plInt32 x = iAreaMin; x <= iAreaMax; ++x)
    {
      if (OverlapCircle[y][x] <= uiCircleType)
      {
        if (Callback(iPosX + x, iPosY + y, pPassThrough) == plCallbackResult::Stop)
          return plRasterizationResult::Aborted;
      }
    }
  }

  return plRasterizationResult::Finished;
}

plRasterizationResult::Enum pl2DGridUtils::RasterizeBlobWithDistance(
  plInt32 iPosX, plInt32 iPosY, plBlobType eType, PLASMA_RASTERIZED_BLOB_CALLBACK Callback, void* pPassThrough /*= nullptr*/)
{
  const plUInt8 uiCircleType = plMath::Clamp<plUInt8>(eType, 0, 8);

  const plInt32 iAreaMin = CircleAreaMin[uiCircleType];
  const plInt32 iAreaMax = CircleAreaMax[uiCircleType];

  iPosX -= CircleCenter;
  iPosY -= CircleCenter;

  for (plInt32 y = iAreaMin; y <= iAreaMax; ++y)
  {
    for (plInt32 x = iAreaMin; x <= iAreaMax; ++x)
    {
      const plUInt8 uiDistance = OverlapCircle[y][x];

      if (uiDistance <= uiCircleType)
      {
        if (Callback(iPosX + x, iPosY + y, pPassThrough, uiDistance) == plCallbackResult::Stop)
          return plRasterizationResult::Aborted;
      }
    }
  }

  return plRasterizationResult::Finished;
}

plRasterizationResult::Enum pl2DGridUtils::RasterizeCircle(
  plInt32 iPosX, plInt32 iPosY, float fRadius, PLASMA_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */)
{
  const plVec2 vCenter((float)iPosX, (float)iPosY);

  const plInt32 iRadius = (plInt32)fRadius;
  const float fRadiusSqr = plMath::Square(fRadius);

  for (plInt32 y = iPosY - iRadius; y <= iPosY + iRadius; ++y)
  {
    for (plInt32 x = iPosX - iRadius; x <= iPosX + iRadius; ++x)
    {
      const plVec2 v((float)x, (float)y);

      if ((v - vCenter).GetLengthSquared() > fRadiusSqr)
        continue;

      if (Callback(x, y, pPassThrough) == plCallbackResult::Stop)
        return plRasterizationResult::Aborted;
    }
  }

  return plRasterizationResult::Finished;
}


struct VisibilityLine
{
  plDynamicArray<plUInt8>* m_pVisible;
  plUInt32 m_uiSize;
  plUInt32 m_uiRadius;
  plInt32 m_iCenterX;
  plInt32 m_iCenterY;
  pl2DGridUtils::PLASMA_RASTERIZED_POINT_CALLBACK m_VisCallback;
  void* m_pUserPassThrough;
  plUInt32 m_uiWidth;
  plUInt32 m_uiHeight;
  plVec2 m_vDirection;
  plAngle m_ConeAngle;
};

struct CellFlags
{
  enum Enum
  {
    NotVisited = 0,
    Visited = PLASMA_BIT(0),
    Visible = Visited | PLASMA_BIT(1),
    Invisible = Visited,
  };
};

static plCallbackResult::Enum MarkPointsOnLineVisible(plInt32 x, plInt32 y, void* pPassThrough)
{
  VisibilityLine* VisLine = (VisibilityLine*)pPassThrough;

  // if the reported point is outside the playing field, don't continue
  if (x < 0 || y < 0 || x >= (plInt32)VisLine->m_uiWidth || y >= (plInt32)VisLine->m_uiHeight)
    return plCallbackResult::Stop;

  // compute the point position inside our virtual grid (where the start position is at the center)
  const plUInt32 VisX = x - VisLine->m_iCenterX + VisLine->m_uiRadius;
  const plUInt32 VisY = y - VisLine->m_iCenterY + VisLine->m_uiRadius;

  // if we are outside our virtual grid, stop
  if (VisX >= (plInt32)VisLine->m_uiSize || VisY >= (plInt32)VisLine->m_uiSize)
    return plCallbackResult::Stop;

  // We actually only need two bits for each cell (visited + visible)
  // so we pack the information for four cells into one byte
  const plUInt32 uiCellIndex = VisY * VisLine->m_uiSize + VisX;
  const plUInt32 uiBitfieldByte = uiCellIndex >> 2;                   // division by four
  const plUInt32 uiBitfieldBiteOff = uiBitfieldByte << 2;             // modulo to determine where in the byte this cell is stored
  const plUInt32 uiMaskShift = (uiCellIndex - uiBitfieldBiteOff) * 2; // times two because we use two bits

  plUInt8& CellFlagsRef = (*VisLine->m_pVisible)[uiBitfieldByte];    // for writing into the byte later
  const plUInt8 ThisCellsFlags = (CellFlagsRef >> uiMaskShift) & 3U; // the decoded flags value for reading (3U == lower two bits)

  // if this point on the line was already visited and determined to be invisible, don't continue
  if (ThisCellsFlags == CellFlags::Invisible)
    return plCallbackResult::Stop;

  // this point has been visited already and the point was determined to be visible, so just continue
  if (ThisCellsFlags == CellFlags::Visible)
    return plCallbackResult::Continue;

  // apparently this cell has not been visited yet, so ask the user callback what to do
  if (VisLine->m_VisCallback(x, y, VisLine->m_pUserPassThrough) == plCallbackResult::Continue)
  {
    // the callback reported this cell as visible, so flag it and continue
    CellFlagsRef |= ((plUInt8)CellFlags::Visible) << uiMaskShift;
    return plCallbackResult::Continue;
  }

  // the callback reported this flag as invisible, flag it and stop the line
  CellFlagsRef |= ((plUInt8)CellFlags::Invisible) << uiMaskShift;
  return plCallbackResult::Stop;
}

static plCallbackResult::Enum MarkPointsInCircleVisible(plInt32 x, plInt32 y, void* pPassThrough)
{
  VisibilityLine* ld = (VisibilityLine*)pPassThrough;

  pl2DGridUtils::ComputePointsOnLineConservative(ld->m_iCenterX, ld->m_iCenterY, x, y, MarkPointsOnLineVisible, pPassThrough, false);

  return plCallbackResult::Continue;
}

void pl2DGridUtils::ComputeVisibleArea(plInt32 iPosX, plInt32 iPosY, plUInt16 uiRadius, plUInt32 uiWidth, plUInt32 uiHeight,
  PLASMA_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */, plDynamicArray<plUInt8>* pTempArray /* = nullptr */)
{
  const plUInt32 uiSize = uiRadius * 2 + 1;

  plDynamicArray<plUInt8> VisiblityFlags;

  // if we don't get a temp array, use our own array, with blackjack etc.
  if (pTempArray == nullptr)
    pTempArray = &VisiblityFlags;

  pTempArray->Clear();
  pTempArray->SetCount(plMath::Square(uiSize) / 4); // we store only two bits per cell, so we can pack four values into each byte

  VisibilityLine ld;
  ld.m_uiSize = uiSize;
  ld.m_uiRadius = uiRadius;
  ld.m_pVisible = pTempArray;
  ld.m_iCenterX = iPosX;
  ld.m_iCenterY = iPosY;
  ld.m_VisCallback = Callback;
  ld.m_pUserPassThrough = pPassThrough;
  ld.m_uiWidth = uiWidth;
  ld.m_uiHeight = uiHeight;

  // from the center, trace lines to all points on the circle around it
  // each line determines for each cell whether it is visible
  // once an invisible cell is encountered, a line will stop further tracing
  // no cell is ever reported twice to the user callback
  pl2DGridUtils::ComputePointsOnCircle(iPosX, iPosY, uiRadius, MarkPointsInCircleVisible, &ld);
}

static plCallbackResult::Enum MarkPointsInConeVisible(plInt32 x, plInt32 y, void* pPassThrough)
{
  VisibilityLine* ld = (VisibilityLine*)pPassThrough;

  const plVec2 vPos((float)x, (float)y);
  const plVec2 vDirToPos = (vPos - plVec2((float)ld->m_iCenterX, (float)ld->m_iCenterY)).GetNormalized();

  const plAngle angle = plMath::ACos(vDirToPos.Dot(ld->m_vDirection));

  if (angle.GetRadian() < ld->m_ConeAngle.GetRadian())
    pl2DGridUtils::ComputePointsOnLineConservative(ld->m_iCenterX, ld->m_iCenterY, x, y, MarkPointsOnLineVisible, pPassThrough, false);

  return plCallbackResult::Continue;
}

void pl2DGridUtils::ComputeVisibleAreaInCone(plInt32 iPosX, plInt32 iPosY, plUInt16 uiRadius, const plVec2& vDirection, plAngle ConeAngle,
  plUInt32 uiWidth, plUInt32 uiHeight, PLASMA_RASTERIZED_POINT_CALLBACK Callback, void* pPassThrough /* = nullptr */,
  plDynamicArray<plUInt8>* pTempArray /* = nullptr */)
{
  const plUInt32 uiSize = uiRadius * 2 + 1;

  plDynamicArray<plUInt8> VisiblityFlags;

  // if we don't get a temp array, use our own array, with blackjack etc.
  if (pTempArray == nullptr)
    pTempArray = &VisiblityFlags;

  pTempArray->Clear();
  pTempArray->SetCount(plMath::Square(uiSize) / 4); // we store only two bits per cell, so we can pack four values into each byte


  VisibilityLine ld;
  ld.m_uiSize = uiSize;
  ld.m_uiRadius = uiRadius;
  ld.m_pVisible = pTempArray;
  ld.m_iCenterX = iPosX;
  ld.m_iCenterY = iPosY;
  ld.m_VisCallback = Callback;
  ld.m_pUserPassThrough = pPassThrough;
  ld.m_uiWidth = uiWidth;
  ld.m_uiHeight = uiHeight;
  ld.m_vDirection = vDirection;
  ld.m_ConeAngle = ConeAngle;

  pl2DGridUtils::ComputePointsOnCircle(iPosX, iPosY, uiRadius, MarkPointsInConeVisible, &ld);
}



PLASMA_STATICLINK_FILE(Utilities, Utilities_GridAlgorithms_Implementation_Rasterization);

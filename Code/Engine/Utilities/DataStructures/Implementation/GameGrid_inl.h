#pragma once

template <class CellData>
plGameGrid<CellData>::plGameGrid()
{
  m_uiGridSizeX = 0;
  m_uiGridSizeY = 0;

  m_mRotateToWorldspace.SetIdentity();
  m_mRotateToGridspace.SetIdentity();

  m_vWorldSpaceOrigin.SetZero();
  m_vLocalSpaceCellSize.Set(1.0f);
  m_vInverseLocalSpaceCellSize.Set(1.0f);
}

template <class CellData>
void plGameGrid<CellData>::CreateGrid(plUInt16 uiSizeX, plUInt16 uiSizeY)
{
  m_Cells.Clear();

  m_uiGridSizeX = uiSizeX;
  m_uiGridSizeY = uiSizeY;

  m_Cells.SetCount(m_uiGridSizeX * m_uiGridSizeY);
}

template <class CellData>
void plGameGrid<CellData>::SetWorldSpaceDimensions(const plVec3& vLowerLeftCorner, const plVec3& vCellSize, Orientation ori)
{
  plMat3 mRot;

  switch (ori)
  {
    case InPlaneXY:
      mRot.SetIdentity();
      break;
    case InPlaneXZ:
      mRot = plMat3::MakeAxisRotation(plVec3(1, 0, 0), plAngle::MakeFromDegree(90.0f));
      break;
    case InPlaneXminusZ:
      mRot = plMat3::MakeAxisRotation(plVec3(1, 0, 0), plAngle::MakeFromDegree(-90.0f));
      break;
  }

  SetWorldSpaceDimensions(vLowerLeftCorner, vCellSize, mRot);
}

template <class CellData>
void plGameGrid<CellData>::SetWorldSpaceDimensions(const plVec3& vLowerLeftCorner, const plVec3& vCellSize, const plMat3& mRotation)
{
  m_vWorldSpaceOrigin = vLowerLeftCorner;
  m_vLocalSpaceCellSize = vCellSize;
  m_vInverseLocalSpaceCellSize = plVec3(1.0f).CompDiv(vCellSize);

  m_mRotateToWorldspace = mRotation;
  m_mRotateToGridspace = mRotation.GetInverse();
}

template <class CellData>
plVec2I32 plGameGrid<CellData>::GetCellAtWorldPosition(const plVec3& vWorldSpacePos) const
{
  const plVec3 vCell = (m_mRotateToGridspace * ((vWorldSpacePos - m_vWorldSpaceOrigin)).CompMul(m_vInverseLocalSpaceCellSize));

  // Without the Floor, the border case when the position is outside (-1 / -1) is not immediately detected
  return plVec2I32((plInt32)plMath::Floor(vCell.x), (plInt32)plMath::Floor(vCell.y));
}

template <class CellData>
plVec3 plGameGrid<CellData>::GetCellWorldSpaceOrigin(const plVec2I32& vCoord) const
{
  return m_vWorldSpaceOrigin + m_mRotateToWorldspace * GetCellLocalSpaceOrigin(vCoord);
}

template <class CellData>
plVec3 plGameGrid<CellData>::GetCellLocalSpaceOrigin(const plVec2I32& vCoord) const
{
  return m_vLocalSpaceCellSize.CompMul(plVec3((float)vCoord.x, (float)vCoord.y, 0.0f));
}

template <class CellData>
plVec3 plGameGrid<CellData>::GetCellWorldSpaceCenter(const plVec2I32& vCoord) const
{
  return m_vWorldSpaceOrigin + m_mRotateToWorldspace * GetCellLocalSpaceCenter(vCoord);
}

template <class CellData>
plVec3 plGameGrid<CellData>::GetCellLocalSpaceCenter(const plVec2I32& vCoord) const
{
  return m_vLocalSpaceCellSize.CompMul(plVec3((float)vCoord.x + 0.5f, (float)vCoord.y + 0.5f, 0.5f));
}

template <class CellData>
bool plGameGrid<CellData>::IsValidCellCoordinate(const plVec2I32& vCoord) const
{
  return (vCoord.x >= 0 && vCoord.x < m_uiGridSizeX && vCoord.y >= 0 && vCoord.y < m_uiGridSizeY);
}

template <class CellData>
bool plGameGrid<CellData>::PickCell(const plVec3& vRayStartPos, const plVec3& vRayDirNorm, plVec2I32* out_pCellCoord, plVec3* out_pIntersection) const
{
  plPlane p;
  p = plPlane::MakeFromNormalAndPoint(m_mRotateToWorldspace * plVec3(0, 0, -1), m_vWorldSpaceOrigin);

  plVec3 vPos;

  if (!p.GetRayIntersection(vRayStartPos, vRayDirNorm, nullptr, &vPos))
    return false;

  if (out_pIntersection)
    *out_pIntersection = vPos;

  if (out_pCellCoord)
    *out_pCellCoord = GetCellAtWorldPosition(vPos);

  return true;
}

template <class CellData>
plBoundingBox plGameGrid<CellData>::GetWorldBoundingBox() const
{
  plVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  vGridBox = m_mRotateToWorldspace * m_vLocalSpaceCellSize.CompMul(vGridBox);

  return plBoundingBox(m_vWorldSpaceOrigin, m_vWorldSpaceOrigin + vGridBox);
}

template <class CellData>
bool plGameGrid<CellData>::GetRayIntersection(const plVec3& vRayStartWorldSpace, const plVec3& vRayDirNormalizedWorldSpace, float fMaxLength,
  float& out_fIntersection, plVec2I32& out_vCellCoord) const
{
  const plVec3 vRayStart = m_mRotateToGridspace * (vRayStartWorldSpace - m_vWorldSpaceOrigin);
  const plVec3 vRayDir = m_mRotateToGridspace * vRayDirNormalizedWorldSpace;

  plVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  const plBoundingBox localBox(plVec3(0.0f), m_vLocalSpaceCellSize.CompMul(vGridBox));

  if (localBox.Contains(vRayStart))
  {
    // if the ray is already inside the box, we know that a cell is hit
    out_fIntersection = 0.0f;
  }
  else
  {
    if (!localBox.GetRayIntersection(vRayStart, vRayDir, &out_fIntersection, nullptr))
      return false;

    if (out_fIntersection > fMaxLength)
      return false;
  }

  const plVec3 vEnterPos = vRayStart + vRayDir * out_fIntersection;

  const plVec3 vCell = vEnterPos.CompMul(m_vInverseLocalSpaceCellSize);

  // Without the Floor, the border case when the position is outside (-1 / -1) is not immediately detected
  out_vCellCoord = plVec2I32((plInt32)plMath::Floor(vCell.x), (plInt32)plMath::Floor(vCell.y));
  out_vCellCoord.x = plMath::Clamp(out_vCellCoord.x, 0, m_uiGridSizeX - 1);
  out_vCellCoord.y = plMath::Clamp(out_vCellCoord.y, 0, m_uiGridSizeY - 1);

  return true;
}

template <class CellData>
bool plGameGrid<CellData>::GetRayIntersectionExpandedBBox(const plVec3& vRayStartWorldSpace, const plVec3& vRayDirNormalizedWorldSpace,
  float fMaxLength, float& out_fIntersection, const plVec3& vExpandBBoxByThis) const
{
  const plVec3 vRayStart = m_mRotateToGridspace * (vRayStartWorldSpace - m_vWorldSpaceOrigin);
  const plVec3 vRayDir = m_mRotateToGridspace * vRayDirNormalizedWorldSpace;

  plVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  plBoundingBox localBox(plVec3(0.0f), m_vLocalSpaceCellSize.CompMul(vGridBox));
  localBox.Grow(vExpandBBoxByThis);

  if (localBox.Contains(vRayStart))
  {
    // if the ray is already inside the box, we know that a cell is hit
    out_fIntersection = 0.0f;
  }
  else
  {
    if (!localBox.GetRayIntersection(vRayStart, vRayDir, &out_fIntersection, nullptr))
      return false;

    if (out_fIntersection > fMaxLength)
      return false;
  }

  return true;
}

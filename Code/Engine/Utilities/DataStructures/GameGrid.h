#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Utilities/UtilitiesDLL.h>

/// \brief plGameGrid is a general purpose 2D grid structure that has several convenience functions which are often required when working
/// with a grid.
template <class CellData>
class plGameGrid
{
public:
  enum Orientation
  {
    InPlaneXY,      ///< The grid is expected to lie in the XY plane in world-space (when Y is up, this is similar to a 2D side scroller)
    InPlaneXZ,      ///< The grid is expected to lie in the XZ plane in world-space (when Y is up, this is similar to a top down RTS game)
    InPlaneXminusZ, ///< The grid is expected to lie in the XZ plane in world-space (when Y is up, this is similar to a top down RTS game)
  };

  plGameGrid();

  /// \brief Clears all data and reallocates the grid with the given dimensions.
  void CreateGrid(plUInt16 uiSizeX, plUInt16 uiSizeY);

  /// \brief Sets the lower left position of the grid in world space coordinates and the cell size.
  ///
  /// Together with the grid size, these values determine the final world space dimensions.
  /// The rotation defines how the grid is rotated in world space. An identity rotation means that grid cell coordinates (X, Y)
  /// map directly to world space coordinates (X, Y). So the grid is 'standing up' in world space (considering that Y is 'up').
  /// Other rotations allow to rotate the grid into other planes, such as XZ, if that is more convenient.
  void SetWorldSpaceDimensions(const plVec3& vLowerLeftCorner, const plVec3& vCellSize, Orientation ori = InPlaneXZ);

  /// \brief Sets the lower left position of the grid in world space coordinates and the cell size.
  ///
  /// Together with the grid size, these values determine the final world space dimensions.
  /// The rotation defines how the grid is rotated in world space. An identity rotation means that grid cell coordinates (X, Y)
  /// map directly to world space coordinates (X, Y). So the grid is 'standing up' in world space (considering that Y is 'up').
  /// Other rotations allow to rotate the grid into other planes, such as XZ, if that is more convenient.
  void SetWorldSpaceDimensions(const plVec3& vLowerLeftCorner, const plVec3& vCellSize, const plMat3& mRotation);

  /// \brief Returns the size of each cell.
  plVec3 GetCellSize() const { return m_vLocalSpaceCellSize; }

  /// \brief Returns the coordinate of the cell at the given world-space position. The world space dimension must be set for this to work.
  /// The indices might be outside valid ranges (negative, larger than the maximum size).
  plVec2I32 GetCellAtWorldPosition(const plVec3& vWorldSpacePos) const;

  /// \brief Returns the number of cells along the X axis.
  plUInt16 GetGridSizeX() const { return m_uiGridSizeX; }

  /// \brief Returns the number of cells along the Y axis.
  plUInt16 GetGridSizeY() const { return m_uiGridSizeY; }

  /// \brief Returns the world-space bounding box of the grid, as specified via SetWorldDimensions.
  plBoundingBox GetWorldBoundingBox() const;

  /// \brief Returns the total number of cells.
  plUInt32 GetNumCells() const { return m_uiGridSizeX * m_uiGridSizeY; }

  /// \brief Gives access to a cell by cell index.
  CellData& GetCell(plUInt32 uiIndex) { return m_Cells[uiIndex]; }

  /// \brief Gives access to a cell by cell index.
  const CellData& GetCell(plUInt32 uiIndex) const { return m_Cells[uiIndex]; }

  /// \brief Gives access to a cell by cell coordinates.
  CellData& GetCell(const plVec2I32& Coord) { return m_Cells[ConvertCellCoordinateToIndex(Coord)]; }

  /// \brief Gives access to a cell by cell coordinates.
  const CellData& GetCell(const plVec2I32& Coord) const { return m_Cells[ConvertCellCoordinateToIndex(Coord)]; }

  /// \brief Converts a cell index into a 2D cell coordinate.
  plVec2I32 ConvertCellIndexToCoordinate(plUInt32 uiIndex) const { return plVec2I32(uiIndex % m_uiGridSizeX, uiIndex / m_uiGridSizeX); }

  /// \brief Converts a cell coordinate into a cell index.
  plUInt32 ConvertCellCoordinateToIndex(const plVec2I32& Coord) const { return Coord.y * m_uiGridSizeX + Coord.x; }

  /// \brief Returns the lower left world space position of the cell with the given coordinates.
  plVec3 GetCellWorldSpaceOrigin(const plVec2I32& Coord) const;
  plVec3 GetCellLocalSpaceOrigin(const plVec2I32& Coord) const;

  /// \brief Returns the center world space position of the cell with the given coordinates.
  plVec3 GetCellWorldSpaceCenter(const plVec2I32& Coord) const;
  plVec3 GetCellLocalSpaceCenter(const plVec2I32& Coord) const;

  /// \brief Checks whether the given cell coordinate is inside valid ranges.
  bool IsValidCellCoordinate(const plVec2I32& Coord) const;

  /// \brief Casts a world space ray through the grid and determines which cell is hit (if any).
  /// \note The picked cell is determined from where the ray hits the 'ground plane', ie. the plane that goes through the world space
  /// origin.
  bool PickCell(const plVec3& vRayStartPos, const plVec3& vRayDirNorm, plVec2I32* out_CellCoord, plVec3* out_vIntersection = nullptr) const;

  /// \brief Returns the lower left corner position in world space of the grid
  const plVec3& GetWorldSpaceOrigin() const { return m_vWorldSpaceOrigin; }

  /// \brief Returns the matrix used to rotate coordinates from grid space to world space
  const plMat3& GetRotationToWorldSpace() const { return m_RotateToWorldspace; }

  /// \brief Returns the matrix used to rotate coordinates from world space to grid space
  const plMat3& GetRotationToGridSpace() const { return m_RotateToGridspace; }

  /// \brief Tests where and at which cell the given world space ray intersects the grids bounding box
  bool GetRayIntersection(const plVec3& vRayStartWorldSpace, const plVec3& vRayDirNormalizedWorldSpace, float fMaxLength, float& out_fIntersection,
    plVec2I32& out_CellCoord) const;

  /// \brief Tests whether a ray would hit the grid bounding box, if it were expanded by a constant.
  bool GetRayIntersectionExpandedBBox(const plVec3& vRayStartWorldSpace, const plVec3& vRayDirNormalizedWorldSpace, float fMaxLength,
    float& out_fIntersection, const plVec3& vExpandBBoxByThis) const;

private:
  plUInt16 m_uiGridSizeX;
  plUInt16 m_uiGridSizeY;

  plMat3 m_RotateToWorldspace;
  plMat3 m_RotateToGridspace;

  plVec3 m_vWorldSpaceOrigin;
  plVec3 m_vLocalSpaceCellSize;
  plVec3 m_vInverseLocalSpaceCellSize;

  plDynamicArray<CellData> m_Cells;
};

#include <Utilities/DataStructures/Implementation/GameGrid_inl.h>

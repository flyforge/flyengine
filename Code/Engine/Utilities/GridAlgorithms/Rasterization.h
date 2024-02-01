#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Vec3.h>
#include <Utilities/UtilitiesDLL.h>

/// \brief Enum values for success and failure. To be used by functions as return values mostly, instead of bool.
struct plCallbackResult
{
  enum Enum
  {
    Stop,     ///< The calling function should stop expanding in this direction (might mean it should abort entirely)
    Continue, ///< The calling function should continue further.
  };
};

/// \brief Enum values for the result of some rasterization functions.
struct plRasterizationResult
{
  enum Enum
  {
    Aborted,  ///< The function was aborted before it reached the end.
    Finished, ///< The function rasterized all possible points.
  };
};

namespace pl2DGridUtils
{
  /// \brief The callback declaration for the function that needs to be passed to the various rasterization functions.
  using PL_RASTERIZED_POINT_CALLBACK = plCallbackResult::Enum (*)(plInt32, plInt32, void*);

  /// \brief The callback declaration for the function that needs to be passed to RasterizeBlobWithDistance().
  using PL_RASTERIZED_BLOB_CALLBACK = plCallbackResult::Enum (*)(plInt32, plInt32, void*, plUInt8);

  /// \brief Computes all the points on a 2D line and calls a function to report every point.
  ///
  /// The function implements Bresenham's algorithm for line rasterization. The first point to be reported through the
  /// callback is always the start position, the last point is always the end position.
  /// pPassThrough is passed through to the user callback for custom data.
  ///
  /// The function returns plRasterizationResult::Aborted if the callback returned plCallbackResult::Stop at any time
  /// and the line will not be computed further in that case.
  /// It returns plRasterizationResult::Finished if the entire line was rasterized.
  ///
  /// This function does not do any dynamic memory allocations internally.
  PL_UTILITIES_DLL plRasterizationResult::Enum ComputePointsOnLine(
    plInt32 iStartX, plInt32 iStartY, plInt32 iEndX, plInt32 iEndY, PL_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr);

  /// \brief Computes all the points on a 2D line and calls a function to report every point.
  ///
  /// Contrary to ComputePointsOnLine() this function does not do diagonal steps but inserts horizontal or vertical steps, such that
  /// reported cells are always connected by an edge.
  /// However, since there are always two possibilities to go from one cell to a diagonal cell, this function tries both and as long
  /// as one of them reports plCallbackResult::Continue, it will continue. Only if both cells are blocked will the algorithm abort.
  ///
  /// If bVisitBothNeighbors is false, the line will continue with the diagonal cell if the first tried neighbor cell is free.
  /// However, if bVisitBothNeighbors is true, the second alternative cell is also reported to the callback, even though its return value
  /// has no effect on whether the line continues or aborts.
  PL_UTILITIES_DLL plRasterizationResult::Enum ComputePointsOnLineConservative(plInt32 iStartX, plInt32 iStartY, plInt32 iEndX, plInt32 iEndY,
    PL_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr, bool bVisitBothNeighbors = false);

  /// \brief Computes all the points on a 2D circle and calls a function to report every point.
  ///
  /// The points are reported in a rather chaotic order (ie. when one draws a line from point to point, it does not yield a circle shape).
  /// The callback may abort the operation by returning plCallbackResult::Stop.
  ///
  /// This function does not do any dynamic memory allocations internally.
  PL_UTILITIES_DLL plRasterizationResult::Enum ComputePointsOnCircle(
    plInt32 iStartX, plInt32 iStartY, plUInt32 uiRadius, PL_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr);

  /// \brief Starts at the given point and then fills all surrounding cells until a border is detected.
  ///
  /// The callback should return plCallbackResult::Continue for each cell that has not been visited so far and for which all four direct
  /// neighbors should be visited. If the flood-fill algorithm leaves the valid area, the callback must return plCallbackResult::Stop to
  /// signal a border. Thus the callback must be able to handle point positions outside the valid range and it also needs to be able to
  /// detect which cells have been visited before, as the FloodFill function will not keep that state internally.
  ///
  /// The function returns the number of cells that were visited and returned plCallbackResult::Continue (ie. which were not classified as
  /// border cells).
  ///
  /// Note that the FloodFill function requires an internal queue to store which cells still need to be visited, as such it will do
  /// dynamic memory allocations. You can pass in a queue that will be used as the temp buffer, thus you can reuse the same container for
  /// several operations, which will reduce the amount of memory allocations that need to be done.
  PL_UTILITIES_DLL plUInt32 FloodFill(
    plInt32 iStartX, plInt32 iStartY, PL_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr, plDeque<plVec2I32>* pTempArray = nullptr);

  /// \brief Same as FloodFill() but also visits the diagonal neighbors, ie. all eight neighboring cells.
  PL_UTILITIES_DLL plUInt32 FloodFillDiag(
    plInt32 iStartX, plInt32 iStartY, PL_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr, plDeque<plVec2I32>* pTempArray = nullptr);

  /// \brief Describes the different circle types that can be rasterized
  enum plBlobType : plUInt8
  {
    Point1x1,    ///< The circle has just one point at the center
    Cross3x3,    ///< The circle has 5 points, one at the center, 1 at each edge of that
    Block3x3,    ///< The 'circle' is just a 3x3 rectangle (9 points)
    Circle5x5,   ///< The circle is a rectangle with each of the 4 corner points missing (21 points)
    Circle7x7,   ///< The circle is a actually starts looking like a circle (37 points)
    Circle9x9,   ///< Circle with 57 points
    Circle11x11, ///< Circle with 97 points
    Circle13x13, ///< Circle with 129 points
    Circle15x15, ///< Circle with 177 points
  };

  /// \brief Rasterizes a circle of limited dimensions and calls the given callback for each point.
  ///
  /// See plCircleType for the available circle types. Those circles are handcrafted to have good looking shapes at low resolutions.
  /// This type of circle is not meant for actually rendering circles, but for doing area operations and overlapping checks for game
  /// units, visibility determination etc. Basically everything that is usually small, but where a simple point might not suffice.
  /// For example most units in a strategy game might only occupy a single cell, but some units might be larger and thus need to occupy
  /// the surrounding cells as well. Using RasterizeBlob() you can compute the units footprint easily.
  ///
  /// RasterizeBlob() will stop immediately and return plRasterizationResult::Aborted when the callback function returns
  /// plCallbackResult::Stop.
  PL_UTILITIES_DLL plRasterizationResult::Enum RasterizeBlob(
    plInt32 iPosX, plInt32 iPosY, plBlobType type, PL_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr);

  /// \brief Same as RasterizeBlob(), but the distance from the center is passed through to the callback, which can use this information to
  /// adjust what it is doing.
  PL_UTILITIES_DLL plRasterizationResult::Enum RasterizeBlobWithDistance(
    plInt32 iPosX, plInt32 iPosY, plBlobType type, PL_RASTERIZED_BLOB_CALLBACK callback, void* pPassThrough = nullptr);

  /// \brief Rasterizes a circle of any size (unlike RasterizeBlob()), though finding the right radius values for nice looking small circles
  /// can be more difficult.
  ///
  /// This function rasterizes a full circle. The radius is a float value, ie. you can use fractional values to shave off cells at the
  /// borders bit by bit.
  ///
  /// RasterizeCircle() will stop immediately and return plRasterizationResult::Aborted when the callback function returns
  /// plCallbackResult::Stop.
  PL_UTILITIES_DLL plRasterizationResult::Enum RasterizeCircle(
    plInt32 iPosX, plInt32 iPosY, float fRadius, PL_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr);


  /// \brief Computes which points are visible from the start position by tracing lines radially outwards.
  ///
  /// The center start position is at iPosX, iPosY and uiRadius defines the maximum distance that an object can see.
  /// uiWidth and uiHeight define the maximum coordinates at which the end of the grid is reached (and thus the line tracing can early out
  /// if it reaches those). For the minimum coordinate (0, 0) is assumed.
  ///
  /// The callback function must return plCallbackResult::Continue for cells that are not blocking and plCallbackResult::Stop for cells that
  /// block visibility.
  ///
  /// The algorithm requires internal state and thus needs to do dynamic memory allocations. If you want to reduce the number of
  /// allocations, you can pass in your own array, that can be reused for many queries.
  PL_UTILITIES_DLL void ComputeVisibleArea(plInt32 iPosX, plInt32 iPosY, plUInt16 uiRadius, plUInt32 uiWidth, plUInt32 uiHeight,
    PL_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr, plDynamicArray<plUInt8>* pTempArray = nullptr);

  /// \brief Computes which points are visible from the start position by tracing lines radially outwards. Limits the computation to a cone.
  ///
  /// This function works exactly like ComputeVisibleArea() but limits the computation to a cone that is defined by vDirection and
  /// ConeAngle.
  PL_UTILITIES_DLL void ComputeVisibleAreaInCone(plInt32 iPosX, plInt32 iPosY, plUInt16 uiRadius, const plVec2& vDirection, plAngle coneAngle,
    plUInt32 uiWidth, plUInt32 uiHeight, PL_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr,
    plDynamicArray<plUInt8>* pTempArray = nullptr);
} // namespace pl2DGridUtils

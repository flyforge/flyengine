#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Vec3.h>

namespace plIntersectionUtils
{
  /// \brief Checks whether a ray intersects with a polygon.
  ///
  /// \param vRayStartPos
  ///   The start position of the ray.
  /// \param vRayDir
  ///   The direction of the ray. This does not need to be normalized. Depending on its length, out_fIntersectionTime will be scaled differently.
  /// \param pPolygonVertices
  ///   Pointer to the first vertex of the polygon.
  /// \param uiNumVertices
  ///   The number of vertices in the polygon.
  /// \param out_fIntersectionTime
  ///   The 'time' at which the ray intersects the polygon. If \a vRayDir is normalized, this is the exact distance.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param out_fIntersectionPoint
  ///   The point where the ray intersects the polygon.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param uiVertexStride
  ///   The stride in bytes between each vertex in the pPolygonVertices array. If the array is tightly packed, this will equal sizeof(plVec3), but it
  ///   can be larger, if the vertices are interleaved with other data.
  /// \return
  ///   True, if the ray intersects the polygon, false otherwise.
  PL_FOUNDATION_DLL bool RayPolygonIntersection(const plVec3& vRayStartPos, const plVec3& vRayDir, const plVec3* pPolygonVertices,
    plUInt32 uiNumVertices, float* out_pIntersectionTime = nullptr, plVec3* out_pIntersectionPoint = nullptr,
    plUInt32 uiVertexStride = sizeof(plVec3)); // [tested]


  /// \brief Returns point on the line segment that is closest to \a vStartPoint. Optionally also returns the fraction along the segment, where that
  /// point is located.
  PL_FOUNDATION_DLL plVec3 ClosestPoint_PointLineSegment(const plVec3& vStartPoint, const plVec3& vLineSegmentPos0, const plVec3& vLineSegmentPos1,
    float* out_pFractionAlongSegment = nullptr); // [tested]

  /// \brief Computes the intersection point and time of the 2D ray with the 2D line segment. Returns true, if there is an intersection.
  PL_FOUNDATION_DLL bool Ray2DLine2D(const plVec2& vRayStartPos, const plVec2& vRayDir, const plVec2& vLineSegmentPos0,
    const plVec2& vLineSegmentPos1, float* out_pIntersectionTime = nullptr, plVec2* out_pIntersectionPoint = nullptr); // [tested]

  /// \brief Tests whether a point is located on a line
  PL_FOUNDATION_DLL bool IsPointOnLine(const plVec3& vLineStart, const plVec3& vLineEnd, const plVec3& vPoint, float fMaxDist = 0.01f);
} // namespace plIntersectionUtils

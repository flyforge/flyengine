#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Intersection.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Plane.h>

bool plIntersectionUtils::RayPolygonIntersection(const plVec3& vRayStartPos, const plVec3& vRayDir, const plVec3* pPolygonVertices,
  plUInt32 uiNumVertices, float* out_pIntersectionTime, plVec3* out_pIntersectionPoint, plUInt32 uiVertexStride)
{
  PLASMA_ASSERT_DEBUG(uiNumVertices >= 3, "A polygon must have at least three vertices.");
  PLASMA_ASSERT_DEBUG(uiVertexStride >= sizeof(plVec3), "The vertex stride is invalid.");

  plPlane p(*pPolygonVertices, *plMemoryUtils::AddByteOffset(pPolygonVertices, uiVertexStride),
    *plMemoryUtils::AddByteOffset(pPolygonVertices, uiVertexStride * 2));

  PLASMA_ASSERT_DEBUG(p.IsValid(), "The given polygon's plane is invalid (computed from the first three vertices only).");

  plVec3 vIntersection;

  if (!p.GetRayIntersection(vRayStartPos, vRayDir, out_pIntersectionTime, &vIntersection))
    return false;

  if (out_pIntersectionPoint)
    *out_pIntersectionPoint = vIntersection;

  // start with the last point as the 'wrap around' position
  plVec3 vPrevPoint = *plMemoryUtils::AddByteOffset(pPolygonVertices, plMath::SafeMultiply32(uiVertexStride, (uiNumVertices - 1)));

  // for each polygon edge
  for (plUInt32 i = 0; i < uiNumVertices; ++i)
  {
    const plVec3 vThisPoint = *plMemoryUtils::AddByteOffset(pPolygonVertices, plMath::SafeMultiply32(uiVertexStride, i));

    const plPlane EdgePlane(vThisPoint, vPrevPoint, vPrevPoint + p.m_vNormal);

    // if the intersection point is outside of any of the edge planes, it is not inside the (convex) polygon
    if (EdgePlane.GetPointPosition(vIntersection) == plPositionOnPlane::Back)
      return false;

    vPrevPoint = vThisPoint;
  }

  // inside all edge planes -> inside the polygon -> there is a proper intersection
  return true;
}

plVec3 plIntersectionUtils::ClosestPoint_PointLineSegment(
  const plVec3& vStartPoint, const plVec3& vLineSegmentPos0, const plVec3& vLineSegmentPos1, float* out_pFractionAlongSegment)
{
  const plVec3 vLineDir = vLineSegmentPos1 - vLineSegmentPos0;
  const plVec3 vToStartPoint = vStartPoint - vLineSegmentPos0;

  const float fProjected = vToStartPoint.Dot(vLineDir);

  float fPosAlongSegment;

  // clamp t to [0; 1] range, and only do the division etc. when necessary
  if (fProjected <= 0.0f)
  {
    fPosAlongSegment = 0.0f;
  }
  else
  {
    const float fSquaredDirLen = vLineDir.GetLengthSquared();

    if (fProjected >= fSquaredDirLen)
    {
      fPosAlongSegment = 1.0f;
    }
    else
    {
      fPosAlongSegment = fProjected / fSquaredDirLen;
    }
  }

  if (out_pFractionAlongSegment)
    *out_pFractionAlongSegment = fPosAlongSegment;

  return vLineSegmentPos0 + fPosAlongSegment * vLineDir;
}

bool plIntersectionUtils::Ray2DLine2D(const plVec2& vRayStartPos, const plVec2& vRayDir, const plVec2& vLineSegmentPos0,
  const plVec2& vLineSegmentPos1, float* out_pIntersectionTime, plVec2* out_pIntersectionPoint)
{
  const plVec2 vLineDir = vLineSegmentPos1 - vLineSegmentPos0;

  // 2D Plane
  const plVec2 vPlaneNormal = vLineDir.GetOrthogonalVector();
  const float fPlaneNegDist = -vPlaneNormal.Dot(vLineSegmentPos0);

  plVec2 vIntersection;
  float fIntersectionTime;

  // 2D Plane ray intersection test
  {
    const float fPlaneSide = vPlaneNormal.Dot(vRayStartPos) + fPlaneNegDist;
    const float fCosAlpha = vPlaneNormal.Dot(vRayDir);

    if (fCosAlpha == 0) // ray is orthogonal to plane
      return false;

    if (plMath::Sign(fPlaneSide) == plMath::Sign(fCosAlpha)) // ray points away from the plane
      return false;

    fIntersectionTime = -fPlaneSide / fCosAlpha;

    vIntersection = vRayStartPos + fIntersectionTime * vRayDir;
  }

  const plVec2 vToIntersection = vIntersection - vLineSegmentPos0;

  const float fProjected = vLineDir.Dot(vToIntersection);

  if (fProjected < 0.0f)
    return false;

  if (fProjected > vLineDir.GetLengthSquared())
    return false;

  if (out_pIntersectionTime)
    *out_pIntersectionTime = fIntersectionTime;

  if (out_pIntersectionPoint)
    *out_pIntersectionPoint = vIntersection;

  return true;
}

bool plIntersectionUtils::IsPointOnLine(const plVec3& vLineStart, const plVec3& vLineEnd, const plVec3& vPoint, float fMaxDist /*= 0.01f*/)
{
  const plVec3 vClosest = ClosestPoint_PointLineSegment(vPoint, vLineStart, vLineEnd);
  const float fClosestDistSqr = (vClosest - vPoint).GetLengthSquared();

  return (fClosestDistSqr <= fMaxDist * fMaxDist);
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Intersection);

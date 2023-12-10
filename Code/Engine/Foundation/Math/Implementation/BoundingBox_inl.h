#pragma once

#include <Foundation/Math/Mat4.h>

template <typename Type>
PLASMA_ALWAYS_INLINE plBoundingBoxTemplate<Type>::plBoundingBoxTemplate() = default;

template <typename Type>
PLASMA_FORCE_INLINE plBoundingBoxTemplate<Type>::plBoundingBoxTemplate(const plVec3Template<Type>& vMin, const plVec3Template<Type>& vMax)
{
  *this = MakeFromMinMax(vMin, vMax);
}

template <typename Type>
PLASMA_FORCE_INLINE plBoundingBoxTemplate<Type> plBoundingBoxTemplate<Type>::MakeZero()
{
  plBoundingBoxTemplate<Type> res;
  res.m_vMin = plVec3Template<Type>::MakeZero();
  res.m_vMax = plVec3Template<Type>::MakeZero();
  return res;
}

template <typename Type>
PLASMA_FORCE_INLINE plBoundingBoxTemplate<Type> plBoundingBoxTemplate<Type>::MakeInvalid()
{
  plBoundingBoxTemplate<Type> res;
  res.m_vMin.Set(plMath::MaxValue<Type>());
  res.m_vMax.Set(-plMath::MaxValue<Type>());
  return res;
}

template <typename Type>
PLASMA_FORCE_INLINE plBoundingBoxTemplate<Type> plBoundingBoxTemplate<Type>::MakeFromCenterAndHalfExtents(const plVec3Template<Type>& vCenter, const plVec3Template<Type>& vHalfExtents)
{
  plBoundingBoxTemplate<Type> res;
  res.m_vMin = vCenter - vHalfExtents;
  res.m_vMax = vCenter + vHalfExtents;
  return res;
}

template <typename Type>
PLASMA_FORCE_INLINE plBoundingBoxTemplate<Type> plBoundingBoxTemplate<Type>::MakeFromMinMax(const plVec3Template<Type>& vMin, const plVec3Template<Type>& vMax)
{
  plBoundingBoxTemplate<Type> res;
  res.m_vMin = vMin;
  res.m_vMax = vMax;

  PLASMA_ASSERT_DEBUG(res.IsValid(), "The given values don't create a valid bounding box ({0} | {1} | {2} - {3} | {4} | {5})", plArgF(vMin.x, 2), plArgF(vMin.y, 2), plArgF(vMin.z, 2), plArgF(vMax.x, 2), plArgF(vMax.y, 2), plArgF(vMax.z, 2));

  return res;
}

template <typename Type>
PLASMA_FORCE_INLINE plBoundingBoxTemplate<Type> plBoundingBoxTemplate<Type>::MakeFromPoints(const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride /*= sizeof(plVec3Template<Type>)*/)
{
  plBoundingBoxTemplate<Type> res = MakeInvalid();
  res.ExpandToInclude(pPoints, uiNumPoints, uiStride);
  return res;
}

template <typename Type>
void plBoundingBoxTemplate<Type>::GetCorners(plVec3Template<Type>* out_pCorners) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_ASSERT_DEBUG(out_pCorners != nullptr, "Out Parameter must not be nullptr.");

  out_pCorners[0].Set(m_vMin.x, m_vMin.y, m_vMin.z);
  out_pCorners[1].Set(m_vMin.x, m_vMin.y, m_vMax.z);
  out_pCorners[2].Set(m_vMin.x, m_vMax.y, m_vMin.z);
  out_pCorners[3].Set(m_vMin.x, m_vMax.y, m_vMax.z);
  out_pCorners[4].Set(m_vMax.x, m_vMin.y, m_vMin.z);
  out_pCorners[5].Set(m_vMax.x, m_vMin.y, m_vMax.z);
  out_pCorners[6].Set(m_vMax.x, m_vMax.y, m_vMin.z);
  out_pCorners[7].Set(m_vMax.x, m_vMax.y, m_vMax.z);
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> plBoundingBoxTemplate<Type>::GetCenter() const
{
  return m_vMin + GetHalfExtents();
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plVec3Template<Type> plBoundingBoxTemplate<Type>::GetExtents() const
{
  return m_vMax - m_vMin;
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> plBoundingBoxTemplate<Type>::GetHalfExtents() const
{
  return (m_vMax - m_vMin) / (Type)2;
}

template <typename Type>
bool plBoundingBoxTemplate<Type>::IsValid() const
{
  return (m_vMin.IsValid() && m_vMax.IsValid() && m_vMin.x <= m_vMax.x && m_vMin.y <= m_vMax.y && m_vMin.z <= m_vMax.z);
}

template <typename Type>
bool plBoundingBoxTemplate<Type>::IsNaN() const
{
  return m_vMin.IsNaN() || m_vMax.IsNaN();
}

template <typename Type>
PLASMA_FORCE_INLINE void plBoundingBoxTemplate<Type>::ExpandToInclude(const plVec3Template<Type>& vPoint)
{
  m_vMin = m_vMin.CompMin(vPoint);
  m_vMax = m_vMax.CompMax(vPoint);
}

template <typename Type>
PLASMA_FORCE_INLINE void plBoundingBoxTemplate<Type>::ExpandToInclude(const plBoundingBoxTemplate<Type>& rhs)
{
  PLASMA_ASSERT_DEBUG(rhs.IsValid(), "rhs must be a valid AABB.");
  ExpandToInclude(rhs.m_vMin);
  ExpandToInclude(rhs.m_vMax);
}

template <typename Type>
void plBoundingBoxTemplate<Type>::ExpandToInclude(const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride)
{
  PLASMA_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  PLASMA_ASSERT_DEBUG(uiStride >= sizeof(plVec3Template<Type>), "Data may not overlap.");

  const plVec3Template<Type>* pCur = &pPoints[0];

  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    ExpandToInclude(*pCur);

    pCur = plMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

template <typename Type>
void plBoundingBoxTemplate<Type>::ExpandToCube()
{
  plVec3Template<Type> vHalfExtents = GetHalfExtents();
  const plVec3Template<Type> vCenter = m_vMin + vHalfExtents;

  const Type f = plMath::Max(vHalfExtents.x, vHalfExtents.y, vHalfExtents.z);

  m_vMin = vCenter - plVec3Template<Type>(f);
  m_vMax = vCenter + plVec3Template<Type>(f);
}

template <typename Type>
PLASMA_FORCE_INLINE void plBoundingBoxTemplate<Type>::Grow(const plVec3Template<Type>& vDiff)
{
  PLASMA_ASSERT_DEBUG(IsValid(), "Cannot grow a box that is invalid.");

  m_vMax += vDiff;
  m_vMin -= vDiff;

  PLASMA_ASSERT_DEBUG(IsValid(), "The grown box has become invalid.");
}

template <typename Type>
PLASMA_FORCE_INLINE bool plBoundingBoxTemplate<Type>::Contains(const plVec3Template<Type>& vPoint) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&vPoint);

  return (plMath::IsInRange(vPoint.x, m_vMin.x, m_vMax.x) && plMath::IsInRange(vPoint.y, m_vMin.y, m_vMax.y) &&
          plMath::IsInRange(vPoint.z, m_vMin.z, m_vMax.z));
}

template <typename Type>
PLASMA_FORCE_INLINE bool plBoundingBoxTemplate<Type>::Contains(const plBoundingBoxTemplate<Type>& rhs) const
{
  return Contains(rhs.m_vMin) && Contains(rhs.m_vMax);
}

template <typename Type>
bool plBoundingBoxTemplate<Type>::Contains(const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride /* = sizeof(plVec3Template<Type>) */) const
{
  PLASMA_ASSERT_DEBUG(pPoints != nullptr, "Array must not be NuLL.");
  PLASMA_ASSERT_DEBUG(uiStride >= sizeof(plVec3Template<Type>), "Data must not overlap.");

  const plVec3Template<Type>* pCur = &pPoints[0];

  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if (!Contains(*pCur))
      return false;

    pCur = plMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return true;
}

template <typename Type>
bool plBoundingBoxTemplate<Type>::Overlaps(const plBoundingBoxTemplate<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  if (rhs.m_vMin.x >= m_vMax.x)
    return false;
  if (rhs.m_vMin.y >= m_vMax.y)
    return false;
  if (rhs.m_vMin.z >= m_vMax.z)
    return false;

  if (m_vMin.x >= rhs.m_vMax.x)
    return false;
  if (m_vMin.y >= rhs.m_vMax.y)
    return false;
  if (m_vMin.z >= rhs.m_vMax.z)
    return false;

  return true;
}

template <typename Type>
bool plBoundingBoxTemplate<Type>::Overlaps(const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride /* = sizeof(plVec3Template<Type>) */) const
{
  PLASMA_ASSERT_DEBUG(pPoints != nullptr, "Array must not be NuLL.");
  PLASMA_ASSERT_DEBUG(uiStride >= sizeof(plVec3Template<Type>), "Data must not overlap.");

  const plVec3Template<Type>* pCur = &pPoints[0];

  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if (Contains(*pCur))
      return true;

    pCur = plMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return false;
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool plBoundingBoxTemplate<Type>::IsIdentical(const plBoundingBoxTemplate<Type>& rhs) const
{
  return (m_vMin == rhs.m_vMin && m_vMax == rhs.m_vMax);
}

template <typename Type>
bool plBoundingBoxTemplate<Type>::IsEqual(const plBoundingBoxTemplate<Type>& rhs, Type fEpsilon) const
{
  return (m_vMin.IsEqual(rhs.m_vMin, fEpsilon) && m_vMax.IsEqual(rhs.m_vMax, fEpsilon));
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool operator==(const plBoundingBoxTemplate<Type>& lhs, const plBoundingBoxTemplate<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool operator!=(const plBoundingBoxTemplate<Type>& lhs, const plBoundingBoxTemplate<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template <typename Type>
PLASMA_FORCE_INLINE void plBoundingBoxTemplate<Type>::Translate(const plVec3Template<Type>& vDiff)
{
  m_vMin += vDiff;
  m_vMax += vDiff;
}

template <typename Type>
void plBoundingBoxTemplate<Type>::ScaleFromCenter(const plVec3Template<Type>& vScale)
{
  const plVec3Template<Type> vCenter = GetCenter();
  const plVec3 vNewMin = vCenter + (m_vMin - vCenter).CompMul(vScale);
  const plVec3 vNewMax = vCenter + (m_vMax - vCenter).CompMul(vScale);

  // this is necessary for negative scalings to work as expected
  m_vMin = vNewMin.CompMin(vNewMax);
  m_vMax = vNewMin.CompMax(vNewMax);
}

template <typename Type>
PLASMA_FORCE_INLINE void plBoundingBoxTemplate<Type>::ScaleFromOrigin(const plVec3Template<Type>& vScale)
{
  const plVec3 vNewMin = m_vMin.CompMul(vScale);
  const plVec3 vNewMax = m_vMax.CompMul(vScale);

  // this is necessary for negative scalings to work as expected
  m_vMin = vNewMin.CompMin(vNewMax);
  m_vMax = vNewMin.CompMax(vNewMax);
}

template <typename Type>
void plBoundingBoxTemplate<Type>::TransformFromCenter(const plMat4Template<Type>& mTransform)
{
  plVec3Template<Type> vCorners[8];
  GetCorners(vCorners);

  const plVec3Template<Type> vCenter = GetCenter();
  *this = MakeInvalid();

  for (plUInt32 i = 0; i < 8; ++i)
    ExpandToInclude(vCenter + mTransform.TransformPosition(vCorners[i] - vCenter));
}

template <typename Type>
void plBoundingBoxTemplate<Type>::TransformFromOrigin(const plMat4Template<Type>& mTransform)
{
  plVec3Template<Type> vCorners[8];
  GetCorners(vCorners);

  mTransform.TransformPosition(vCorners, 8);

  *this = MakeInvalid();
  ExpandToInclude(vCorners, 8);
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> plBoundingBoxTemplate<Type>::GetClampedPoint(const plVec3Template<Type>& vPoint) const
{
  return vPoint.CompMin(m_vMax).CompMax(m_vMin);
}

template <typename Type>
Type plBoundingBoxTemplate<Type>::GetDistanceTo(const plVec3Template<Type>& vPoint) const
{
  const plVec3Template<Type> vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLength();
}

template <typename Type>
Type plBoundingBoxTemplate<Type>::GetDistanceSquaredTo(const plVec3Template<Type>& vPoint) const
{
  const plVec3Template<Type> vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLengthSquared();
}

template <typename Type>
Type plBoundingBoxTemplate<Type>::GetDistanceSquaredTo(const plBoundingBoxTemplate<Type>& rhs) const
{
  // This will return zero for overlapping boxes

  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  Type fDistSQR = 0.0f;

  {
    if (rhs.m_vMin.x > m_vMax.x)
    {
      fDistSQR += plMath::Square(rhs.m_vMin.x - m_vMax.x);
    }
    else if (rhs.m_vMax.x < m_vMin.x)
    {
      fDistSQR += plMath::Square(m_vMin.x - rhs.m_vMax.x);
    }
  }

  {
    if (rhs.m_vMin.y > m_vMax.y)
    {
      fDistSQR += plMath::Square(rhs.m_vMin.y - m_vMax.y);
    }
    else if (rhs.m_vMax.y < m_vMin.y)
    {
      fDistSQR += plMath::Square(m_vMin.y - rhs.m_vMax.y);
    }
  }

  {
    if (rhs.m_vMin.z > m_vMax.z)
    {
      fDistSQR += plMath::Square(rhs.m_vMin.z - m_vMax.z);
    }
    else if (rhs.m_vMax.z < m_vMin.z)
    {
      fDistSQR += plMath::Square(m_vMin.z - rhs.m_vMax.z);
    }
  }

  return fDistSQR;
}

template <typename Type>
Type plBoundingBoxTemplate<Type>::GetDistanceTo(const plBoundingBoxTemplate<Type>& rhs) const
{
  return plMath::Sqrt(GetDistanceSquaredTo(rhs));
}

template <typename Type>
bool plBoundingBoxTemplate<Type>::GetRayIntersection(const plVec3Template<Type>& vStartPos, const plVec3Template<Type>& vRayDir, Type* out_pIntersectionDistance, plVec3Template<Type>* out_pIntersection) const
{
  // This code was taken from: http://people.csail.mit.edu/amy/papers/box-jgt.pdf
  // "An Efficient and Robust Ray-Box Intersection Algorithm"
  // Contrary to previous implementation, this one actually works with ray/box configurations
  // that produce division by zero and multiplication with infinity (which can produce NaNs).

  PLASMA_ASSERT_DEBUG(plMath::SupportsInfinity<Type>(), "This type does not support infinite values, which is required for this algorithm.");
  PLASMA_ASSERT_DEBUG(vStartPos.IsValid(), "Ray start position must be valid.");
  PLASMA_ASSERT_DEBUG(vRayDir.IsValid(), "Ray direction must be valid.");

  PLASMA_NAN_ASSERT(this);

  float tMin, tMax;

  // Compare along X and Z axis, find intersection point
  {
    float tMinY, tMaxY;

    const float fDivX = 1.0f / vRayDir.x;
    const float fDivY = 1.0f / vRayDir.y;

    if (vRayDir.x >= 0.0f)
    {
      tMin = (m_vMin.x - vStartPos.x) * fDivX;
      tMax = (m_vMax.x - vStartPos.x) * fDivX;
    }
    else
    {
      tMin = (m_vMax.x - vStartPos.x) * fDivX;
      tMax = (m_vMin.x - vStartPos.x) * fDivX;
    }

    if (vRayDir.y >= 0.0f)
    {
      tMinY = (m_vMin.y - vStartPos.y) * fDivY;
      tMaxY = (m_vMax.y - vStartPos.y) * fDivY;
    }
    else
    {
      tMinY = (m_vMax.y - vStartPos.y) * fDivY;
      tMaxY = (m_vMin.y - vStartPos.y) * fDivY;
    }

    if (tMin > tMaxY || tMinY > tMax)
      return false;

    if (tMinY > tMin)
      tMin = tMinY;
    if (tMaxY < tMax)
      tMax = tMaxY;
  }

  // Compare along Z axis and previous result, find intersection point
  {
    float tMinZ, tMaxZ;

    const float fDivZ = 1.0f / vRayDir.z;

    if (vRayDir.z >= 0.0f)
    {
      tMinZ = (m_vMin.z - vStartPos.z) * fDivZ;
      tMaxZ = (m_vMax.z - vStartPos.z) * fDivZ;
    }
    else
    {
      tMinZ = (m_vMax.z - vStartPos.z) * fDivZ;
      tMaxZ = (m_vMin.z - vStartPos.z) * fDivZ;
    }

    if (tMin > tMaxZ || tMinZ > tMax)
      return false;

    if (tMinZ > tMin)
      tMin = tMinZ;
    if (tMaxZ < tMax)
      tMax = tMaxZ;
  }

  // rays that start inside the box are considered as not hitting the box
  if (tMax <= 0.0f)
    return false;

  if (out_pIntersectionDistance)
    *out_pIntersectionDistance = tMin;

  if (out_pIntersection)
    *out_pIntersection = vStartPos + tMin * vRayDir;

  return true;
}

template <typename Type>
bool plBoundingBoxTemplate<Type>::GetLineSegmentIntersection(const plVec3Template<Type>& vStartPos, const plVec3Template<Type>& vEndPos, Type* out_pLineFraction, plVec3Template<Type>* out_pIntersection) const
{
  const plVec3Template<Type> vRayDir = vEndPos - vStartPos;

  Type fIntersection = 0.0f;
  if (!GetRayIntersection(vStartPos, vRayDir, &fIntersection, out_pIntersection))
    return false;

  if (out_pLineFraction)
    *out_pLineFraction = fIntersection;

  return fIntersection <= 1.0f;
}



#include <Foundation/Math/Implementation/AllClasses_inl.h>

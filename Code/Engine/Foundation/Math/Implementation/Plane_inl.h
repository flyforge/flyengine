#pragma once

#include <Foundation/Math/Mat4.h>

template <typename Type>
PL_FORCE_INLINE plPlaneTemplate<Type>::plPlaneTemplate()
{
#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = plMath::NaN<Type>();
  m_vNormal.Set(TypeNaN);
  m_fNegDistance = TypeNaN;
#endif
}

template <typename Type>
plPlaneTemplate<Type> plPlaneTemplate<Type>::MakeInvalid()
{
  plPlaneTemplate<Type> res;
  res.m_vNormal.Set(0);
  res.m_fNegDistance = 0;
  return res;
}

template <typename Type>
plPlaneTemplate<Type> plPlaneTemplate<Type>::MakeFromNormalAndPoint(const plVec3Template<Type>& vNormal, const plVec3Template<Type>& vPointOnPlane)
{
  PL_ASSERT_DEV(vNormal.IsNormalized(), "Normal must be normalized.");

  plPlaneTemplate<Type> res;
  res.m_vNormal = vNormal;
  res.m_fNegDistance = -vNormal.Dot(vPointOnPlane);
  return res;
}

template <typename Type>
plPlaneTemplate<Type> plPlaneTemplate<Type>::MakeFromPoints(const plVec3Template<Type>& v1, const plVec3Template<Type>& v2, const plVec3Template<Type>& v3)
{
  plPlaneTemplate<Type> res;
  PL_VERIFY(res.m_vNormal.CalculateNormal(v1, v2, v3).Succeeded(), "The 3 provided points do not form a plane");

  res.m_fNegDistance = -res.m_vNormal.Dot(v1);
  return res;
}

template <typename Type>
plVec4Template<Type> plPlaneTemplate<Type>::GetAsVec4() const
{
  return plVec4(m_vNormal.x, m_vNormal.y, m_vNormal.z, m_fNegDistance);
}

template <typename Type>
plResult plPlaneTemplate<Type>::SetFromPoints(const plVec3Template<Type>& v1, const plVec3Template<Type>& v2, const plVec3Template<Type>& v3)
{
  if (m_vNormal.CalculateNormal(v1, v2, v3) == PL_FAILURE)
    return PL_FAILURE;

  m_fNegDistance = -m_vNormal.Dot(v1);
  return PL_SUCCESS;
}

template <typename Type>
plResult plPlaneTemplate<Type>::SetFromPoints(const plVec3Template<Type>* const pVertices)
{
  if (m_vNormal.CalculateNormal(pVertices[0], pVertices[1], pVertices[2]) == PL_FAILURE)
    return PL_FAILURE;

  m_fNegDistance = -m_vNormal.Dot(pVertices[0]);
  return PL_SUCCESS;
}

template <typename Type>
plResult plPlaneTemplate<Type>::SetFromDirections(const plVec3Template<Type>& vTangent1, const plVec3Template<Type>& vTangent2, const plVec3Template<Type>& vPointOnPlane)
{
  plVec3Template<Type> vNormal = vTangent1.CrossRH(vTangent2);
  plResult res = vNormal.NormalizeIfNotZero();

  m_vNormal = vNormal;
  m_fNegDistance = -vNormal.Dot(vPointOnPlane);
  return res;
}

template <typename Type>
void plPlaneTemplate<Type>::Transform(const plMat3Template<Type>& m)
{
  plVec3Template<Type> vPointOnPlane = m_vNormal * -m_fNegDistance;

  // rotate the normal, translate the point
  plVec3Template<Type> vTransformedNormal = m.TransformDirection(m_vNormal);

  // If the plane's distance is already infinite, there won't be any meaningful change
  // to it as a result of the transformation.
  if (!plMath::IsFinite(m_fNegDistance))
  {
    m_vNormal = vTransformedNormal;
  }
  else
  {
    *this = plPlane::MakeFromNormalAndPoint(vTransformedNormal, m * vPointOnPlane);
  }
}

template <typename Type>
void plPlaneTemplate<Type>::Transform(const plMat4Template<Type>& m)
{
  plVec3Template<Type> vPointOnPlane = m_vNormal * -m_fNegDistance;

  // rotate the normal, translate the point
  plVec3Template<Type> vTransformedNormal = m.TransformDirection(m_vNormal);

  // If the plane's distance is already infinite, there won't be any meaningful change
  // to it as a result of the transformation.
  if (!plMath::IsFinite(m_fNegDistance))
  {
    m_vNormal = vTransformedNormal;
  }
  else
  {
    *this = plPlane::MakeFromNormalAndPoint(vTransformedNormal, m * vPointOnPlane);
  }
}

template <typename Type>
PL_FORCE_INLINE void plPlaneTemplate<Type>::Flip()
{
  m_fNegDistance = -m_fNegDistance;
  m_vNormal = -m_vNormal;
}

template <typename Type>
PL_FORCE_INLINE Type plPlaneTemplate<Type>::GetDistanceTo(const plVec3Template<Type>& vPoint) const
{
  return (m_vNormal.Dot(vPoint) + m_fNegDistance);
}

template <typename Type>
PL_FORCE_INLINE plPositionOnPlane::Enum plPlaneTemplate<Type>::GetPointPosition(const plVec3Template<Type>& vPoint) const
{
  return (m_vNormal.Dot(vPoint) < -m_fNegDistance ? plPositionOnPlane::Back : plPositionOnPlane::Front);
}

template <typename Type>
plPositionOnPlane::Enum plPlaneTemplate<Type>::GetPointPosition(const plVec3Template<Type>& vPoint, Type fPlaneHalfWidth) const
{
  const Type f = m_vNormal.Dot(vPoint);

  if (f + fPlaneHalfWidth < -m_fNegDistance)
    return plPositionOnPlane::Back;

  if (f - fPlaneHalfWidth > -m_fNegDistance)
    return plPositionOnPlane::Front;

  return plPositionOnPlane::OnPlane;
}

template <typename Type>
PL_FORCE_INLINE const plVec3Template<Type> plPlaneTemplate<Type>::ProjectOntoPlane(const plVec3Template<Type>& vPoint) const
{
  return vPoint - m_vNormal * (m_vNormal.Dot(vPoint) + m_fNegDistance);
}

template <typename Type>
PL_FORCE_INLINE const plVec3Template<Type> plPlaneTemplate<Type>::Mirror(const plVec3Template<Type>& vPoint) const
{
  return vPoint - (Type)2 * GetDistanceTo(vPoint) * m_vNormal;
}

template <typename Type>
const plVec3Template<Type> plPlaneTemplate<Type>::GetCoplanarDirection(const plVec3Template<Type>& vDirection) const
{
  plVec3Template<Type> res = vDirection;
  res.MakeOrthogonalTo(m_vNormal);
  return res;
}

template <typename Type>
bool plPlaneTemplate<Type>::IsIdentical(const plPlaneTemplate& rhs) const
{
  return m_vNormal.IsIdentical(rhs.m_vNormal) && m_fNegDistance == rhs.m_fNegDistance;
}

template <typename Type>
bool plPlaneTemplate<Type>::IsEqual(const plPlaneTemplate& rhs, Type fEpsilon) const
{
  return m_vNormal.IsEqual(rhs.m_vNormal, fEpsilon) && plMath::IsEqual(m_fNegDistance, rhs.m_fNegDistance, fEpsilon);
}

template <typename Type>
PL_ALWAYS_INLINE bool operator==(const plPlaneTemplate<Type>& lhs, const plPlaneTemplate<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template <typename Type>
PL_ALWAYS_INLINE bool operator!=(const plPlaneTemplate<Type>& lhs, const plPlaneTemplate<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template <typename Type>
bool plPlaneTemplate<Type>::FlipIfNecessary(const plVec3Template<Type>& vPoint, bool bPlaneShouldFacePoint)
{
  if ((GetPointPosition(vPoint) == plPositionOnPlane::Front) != bPlaneShouldFacePoint)
  {
    Flip();
    return true;
  }

  return false;
}

template <typename Type>
bool plPlaneTemplate<Type>::IsValid() const
{
  return !IsNaN() && m_vNormal.IsNormalized(plMath::DefaultEpsilon<Type>());
}

template <typename Type>
bool plPlaneTemplate<Type>::IsNaN() const
{
  return plMath::IsNaN(m_fNegDistance) || m_vNormal.IsNaN();
}

template <typename Type>
bool plPlaneTemplate<Type>::IsFinite() const
{
  return m_vNormal.IsValid() && plMath::IsFinite(m_fNegDistance);
}

/*! The given vertices can be partially equal or lie on the same line. The algorithm will try to find 3 vertices, that
  form a plane, and deduce the normal from them. This algorithm is much slower, than all the other methods, so only
  use it, when you know, that your data can contain such configurations. */
template <typename Type>
plResult plPlaneTemplate<Type>::SetFromPoints(const plVec3Template<Type>* const pVertices, plUInt32 uiMaxVertices)
{
  plInt32 iPoints[3];

  if (FindSupportPoints(pVertices, uiMaxVertices, iPoints[0], iPoints[1], iPoints[2]) == PL_FAILURE)
  {
    SetFromPoints(pVertices).IgnoreResult();
    return PL_FAILURE;
  }

  SetFromPoints(pVertices[iPoints[0]], pVertices[iPoints[1]], pVertices[iPoints[2]]).IgnoreResult();
  return PL_SUCCESS;
}

template <typename Type>
plResult plPlaneTemplate<Type>::FindSupportPoints(const plVec3Template<Type>* const pVertices, int iMaxVertices, int& out_i1, int& out_i2, int& out_i3)
{
  const plVec3Template<Type> v1 = pVertices[0];

  bool bFoundSecond = false;

  int i = 1;
  while (i < iMaxVertices)
  {
    if (pVertices[i].IsEqual(v1, 0.001f) == false)
    {
      bFoundSecond = true;
      break;
    }

    ++i;
  }

  if (!bFoundSecond)
    return PL_FAILURE;

  const plVec3Template<Type> v2 = pVertices[i];

  const plVec3Template<Type> vDir1 = (v1 - v2).GetNormalized();

  out_i1 = 0;
  out_i2 = i;

  ++i;

  while (i < iMaxVertices)
  {
    // check for inequality, then for non-collinearity
    if ((pVertices[i].IsEqual(v2, 0.001f) == false) && (plMath::Abs((pVertices[i] - v2).GetNormalized().Dot(vDir1)) < (Type)0.999))
    {
      out_i3 = i;
      return PL_SUCCESS;
    }

    ++i;
  }

  return PL_FAILURE;
}

template <typename Type>
plPositionOnPlane::Enum plPlaneTemplate<Type>::GetObjectPosition(const plVec3Template<Type>* const pPoints, plUInt32 uiVertices) const
{
  bool bFront = false;
  bool bBack = false;

  for (plUInt32 i = 0; i < uiVertices; ++i)
  {
    switch (GetPointPosition(pPoints[i]))
    {
      case plPositionOnPlane::Front:
        if (bBack)
          return (plPositionOnPlane::Spanning);
        bFront = true;
        break;
      case plPositionOnPlane::Back:
        if (bFront)
          return (plPositionOnPlane::Spanning);
        bBack = true;
        break;

      default:
        break;
    }
  }

  return (bFront ? plPositionOnPlane::Front : plPositionOnPlane::Back);
}

template <typename Type>
plPositionOnPlane::Enum plPlaneTemplate<Type>::GetObjectPosition(const plVec3Template<Type>* const pPoints, plUInt32 uiVertices, Type fPlaneHalfWidth) const
{
  bool bFront = false;
  bool bBack = false;

  for (plUInt32 i = 0; i < uiVertices; ++i)
  {
    switch (GetPointPosition(pPoints[i], fPlaneHalfWidth))
    {
      case plPositionOnPlane::Front:
        if (bBack)
          return (plPositionOnPlane::Spanning);
        bFront = true;
        break;
      case plPositionOnPlane::Back:
        if (bFront)
          return (plPositionOnPlane::Spanning);
        bBack = true;
        break;

      default:
        break;
    }
  }

  if (bFront)
    return (plPositionOnPlane::Front);
  if (bBack)
    return (plPositionOnPlane::Back);

  return (plPositionOnPlane::OnPlane);
}

template <typename Type>
bool plPlaneTemplate<Type>::GetRayIntersection(const plVec3Template<Type>& vRayStartPos, const plVec3Template<Type>& vRayDir, Type* out_pIntersectionDistance, plVec3Template<Type>* out_pIntersection) const
{
  PL_ASSERT_DEBUG(vRayStartPos.IsValid(), "Ray start position must be valid.");
  PL_ASSERT_DEBUG(vRayDir.IsValid(), "Ray direction must be valid.");

  const Type fPlaneSide = GetDistanceTo(vRayStartPos);
  const Type fCosAlpha = m_vNormal.Dot(vRayDir);

  if (fCosAlpha == 0) // ray is orthogonal to plane
    return false;

  if (plMath::Sign(fPlaneSide) == plMath::Sign(fCosAlpha)) // ray points away from the plane
    return false;

  const Type fTime = -fPlaneSide / fCosAlpha;

  if (out_pIntersectionDistance)
    *out_pIntersectionDistance = fTime;

  if (out_pIntersection)
    *out_pIntersection = vRayStartPos + fTime * vRayDir;

  return true;
}

template <typename Type>
bool plPlaneTemplate<Type>::GetRayIntersectionBiDirectional(const plVec3Template<Type>& vRayStartPos, const plVec3Template<Type>& vRayDir, Type* out_pIntersectionDistance, plVec3Template<Type>* out_pIntersection) const
{
  PL_ASSERT_DEBUG(vRayStartPos.IsValid(), "Ray start position must be valid.");
  PL_ASSERT_DEBUG(vRayDir.IsValid(), "Ray direction must be valid.");

  const Type fPlaneSide = GetDistanceTo(vRayStartPos);
  const Type fCosAlpha = m_vNormal.Dot(vRayDir);

  if (fCosAlpha == 0) // ray is orthogonal to plane
    return false;

  const Type fTime = -fPlaneSide / fCosAlpha;

  if (out_pIntersectionDistance)
    *out_pIntersectionDistance = fTime;

  if (out_pIntersection)
    *out_pIntersection = vRayStartPos + fTime * vRayDir;

  return true;
}

template <typename Type>
bool plPlaneTemplate<Type>::GetLineSegmentIntersection(const plVec3Template<Type>& vLineStartPos, const plVec3Template<Type>& vLineEndPos, Type* out_pHitFraction, plVec3Template<Type>* out_pIntersection) const
{
  Type fTime = 0;

  if (!GetRayIntersection(vLineStartPos, vLineEndPos - vLineStartPos, &fTime, out_pIntersection))
    return false;

  if (out_pHitFraction)
    *out_pHitFraction = fTime;

  return (fTime <= 1);
}

template <typename Type>
Type plPlaneTemplate<Type>::GetMinimumDistanceTo(const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride /* = sizeof (plVec3Template<Type>) */) const
{
  PL_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  PL_ASSERT_DEBUG(uiStride >= sizeof(plVec3Template<Type>), "Stride must be at least sizeof(plVec3Template) to not have overlapping data.");
  PL_ASSERT_DEBUG(uiNumPoints >= 1, "Array must contain at least one point.");

  Type fMinDist = plMath::MaxValue<Type>();

  const plVec3Template<Type>* pCurPoint = pPoints;

  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    fMinDist = plMath::Min(m_vNormal.Dot(*pCurPoint), fMinDist);

    pCurPoint = plMemoryUtils::AddByteOffset(pCurPoint, uiStride);
  }

  return fMinDist + m_fNegDistance;
}

template <typename Type>
void plPlaneTemplate<Type>::GetMinMaxDistanceTo(Type& out_fMin, Type& out_fMax, const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride /* = sizeof (plVec3Template<Type>) */) const
{
  PL_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  PL_ASSERT_DEBUG(uiStride >= sizeof(plVec3Template<Type>), "Stride must be at least sizeof(plVec3Template) to not have overlapping data.");
  PL_ASSERT_DEBUG(uiNumPoints >= 1, "Array must contain at least one point.");

  out_fMin = plMath::MaxValue<Type>();
  out_fMax = -plMath::MaxValue<Type>();

  const plVec3Template<Type>* pCurPoint = pPoints;

  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type f = m_vNormal.Dot(*pCurPoint);

    out_fMin = plMath::Min(f, out_fMin);
    out_fMax = plMath::Max(f, out_fMax);

    pCurPoint = plMemoryUtils::AddByteOffset(pCurPoint, uiStride);
  }

  out_fMin += m_fNegDistance;
  out_fMax += m_fNegDistance;
}

template <typename Type>
plResult plPlaneTemplate<Type>::GetPlanesIntersectionPoint(const plPlaneTemplate& p0, const plPlaneTemplate& p1, const plPlaneTemplate& p2, plVec3Template<Type>& out_vResult)
{
  const plVec3Template<Type> n1(p0.m_vNormal);
  const plVec3Template<Type> n2(p1.m_vNormal);
  const plVec3Template<Type> n3(p2.m_vNormal);

  const Type det = n1.Dot(n2.CrossRH(n3));

  if (plMath::IsZero<Type>(det, plMath::LargeEpsilon<Type>()))
    return PL_FAILURE;

  out_vResult = (-p0.m_fNegDistance * n2.CrossRH(n3) + -p1.m_fNegDistance * n3.CrossRH(n1) + -p2.m_fNegDistance * n1.CrossRH(n2)) / det;

  return PL_SUCCESS;
}

#include <Foundation/Math/Implementation/AllClasses_inl.h>

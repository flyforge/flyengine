#pragma once

#include <Foundation/Math/Mat4.h>

template <typename Type>
PLASMA_FORCE_INLINE plBoundingSphereTemplate<Type>::plBoundingSphereTemplate()
{
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  // m_vCenter is already initialized to NaN by its own constructor.
  const Type TypeNaN = plMath::NaN<Type>();
  m_fRadius = TypeNaN;
#endif
}

template <typename Type>
PLASMA_FORCE_INLINE plBoundingSphereTemplate<Type>::plBoundingSphereTemplate(const plVec3Template<Type>& vCenter, Type fRadius)
  : m_vCenter(vCenter)
  , m_fRadius(fRadius)
{
}

template <typename Type>
void plBoundingSphereTemplate<Type>::SetZero()
{
  m_vCenter.SetZero();
  m_fRadius = 0.0f;
}

template <typename Type>
bool plBoundingSphereTemplate<Type>::IsZero(Type fEpsilon /* = plMath::DefaultEpsilon<Type>() */) const
{
  return m_vCenter.IsZero(fEpsilon) && plMath::IsZero(m_fRadius, fEpsilon);
}

template <typename Type>
void plBoundingSphereTemplate<Type>::SetInvalid()
{
  m_vCenter.SetZero();
  m_fRadius = -plMath::SmallEpsilon<Type>();
}

template <typename Type>
bool plBoundingSphereTemplate<Type>::IsValid() const
{
  return (m_vCenter.IsValid() && m_fRadius >= 0.0f);
}

template <typename Type>
bool plBoundingSphereTemplate<Type>::IsNaN() const
{
  return (m_vCenter.IsNaN() || plMath::IsNaN(m_fRadius));
}

template <typename Type>
PLASMA_FORCE_INLINE void plBoundingSphereTemplate<Type>::SetElements(const plVec3Template<Type>& vCenter, Type fRadius)
{
  m_vCenter = vCenter;
  m_fRadius = fRadius;

  PLASMA_ASSERT_DEBUG(IsValid(), "The sphere was created with invalid values.");
}

template <typename Type>
void plBoundingSphereTemplate<Type>::ExpandToInclude(const plVec3Template<Type>& vPoint)
{
  const Type fDistSQR = (vPoint - m_vCenter).GetLengthSquared();

  if (plMath::Square(m_fRadius) < fDistSQR)
    m_fRadius = plMath::Sqrt(fDistSQR);
}

template <typename Type>
void plBoundingSphereTemplate<Type>::ExpandToInclude(const plBoundingSphereTemplate<Type>& rhs)
{
  const Type fReqRadius = (rhs.m_vCenter - m_vCenter).GetLength() + rhs.m_fRadius;

  m_fRadius = plMath::Max(m_fRadius, fReqRadius);
}

template <typename Type>
PLASMA_FORCE_INLINE void plBoundingSphereTemplate<Type>::Grow(Type fDiff)
{
  PLASMA_ASSERT_DEBUG(IsValid(), "Cannot grow a sphere that is invalid.");

  m_fRadius += fDiff;

  PLASMA_ASSERT_DEBUG(IsValid(), "The grown sphere has become invalid.");
}

template <typename Type>
bool plBoundingSphereTemplate<Type>::IsIdentical(const plBoundingSphereTemplate<Type>& rhs) const
{
  return (m_vCenter.IsIdentical(rhs.m_vCenter) && m_fRadius == rhs.m_fRadius);
}

template <typename Type>
bool plBoundingSphereTemplate<Type>::IsEqual(const plBoundingSphereTemplate<Type>& rhs, Type fEpsilon) const
{
  return (m_vCenter.IsEqual(rhs.m_vCenter, fEpsilon) && plMath::IsEqual(m_fRadius, rhs.m_fRadius, fEpsilon));
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool operator==(const plBoundingSphereTemplate<Type>& lhs, const plBoundingSphereTemplate<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool operator!=(const plBoundingSphereTemplate<Type>& lhs, const plBoundingSphereTemplate<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plBoundingSphereTemplate<Type>::Translate(const plVec3Template<Type>& vTranslation)
{
  m_vCenter += vTranslation;
}

template <typename Type>
PLASMA_FORCE_INLINE void plBoundingSphereTemplate<Type>::ScaleFromCenter(Type fScale)
{
  PLASMA_ASSERT_DEBUG(fScale >= 0.0f, "Cannot invert the sphere.");

  m_fRadius *= fScale;

  PLASMA_NAN_ASSERT(this);
}

template <typename Type>
void plBoundingSphereTemplate<Type>::ScaleFromOrigin(const plVec3Template<Type>& vScale)
{
  PLASMA_ASSERT_DEBUG(vScale.x >= 0.0f, "Cannot invert the sphere.");
  PLASMA_ASSERT_DEBUG(vScale.y >= 0.0f, "Cannot invert the sphere.");
  PLASMA_ASSERT_DEBUG(vScale.z >= 0.0f, "Cannot invert the sphere.");

  m_vCenter = m_vCenter.CompMul(vScale);

  // scale the radius by the maximum scaling factor (the sphere cannot become an ellipsoid,
  // so to be a 'bounding' sphere, it should be as large as possible
  m_fRadius *= plMath::Max(vScale.x, vScale.y, vScale.z);
}

template <typename Type>
void plBoundingSphereTemplate<Type>::TransformFromOrigin(const plMat4Template<Type>& mTransform)
{
  m_vCenter = mTransform.TransformPosition(m_vCenter);

  const plVec3Template<Type> Scale = mTransform.GetScalingFactors();
  m_fRadius *= plMath::Max(Scale.x, Scale.y, Scale.z);
}

template <typename Type>
void plBoundingSphereTemplate<Type>::TransformFromCenter(const plMat4Template<Type>& mTransform)
{
  m_vCenter += mTransform.GetTranslationVector();

  const plVec3Template<Type> Scale = mTransform.GetScalingFactors();
  m_fRadius *= plMath::Max(Scale.x, Scale.y, Scale.z);
}

template <typename Type>
Type plBoundingSphereTemplate<Type>::GetDistanceTo(const plVec3Template<Type>& vPoint) const
{
  return (vPoint - m_vCenter).GetLength() - m_fRadius;
}

template <typename Type>
Type plBoundingSphereTemplate<Type>::GetDistanceTo(const plBoundingSphereTemplate<Type>& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLength() - m_fRadius - rhs.m_fRadius;
}

template <typename Type>
bool plBoundingSphereTemplate<Type>::Contains(const plVec3Template<Type>& vPoint) const
{
  return (vPoint - m_vCenter).GetLengthSquared() <= plMath::Square(m_fRadius);
}

template <typename Type>
bool plBoundingSphereTemplate<Type>::Contains(const plBoundingSphereTemplate<Type>& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLength() + rhs.m_fRadius <= m_fRadius;
}

template <typename Type>
bool plBoundingSphereTemplate<Type>::Overlaps(const plBoundingSphereTemplate<Type>& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLengthSquared() < plMath::Square(rhs.m_fRadius + m_fRadius);
}

template <typename Type>
const plVec3Template<Type> plBoundingSphereTemplate<Type>::GetClampedPoint(const plVec3Template<Type>& vPoint)
{
  const plVec3Template<Type> vDir = vPoint - m_vCenter;
  const Type fDistSQR = vDir.GetLengthSquared();

  // return the point, if it is already inside the sphere
  if (fDistSQR <= plMath::Square(m_fRadius))
    return vPoint;

  // otherwise return a point on the surface of the sphere

  const Type fLength = plMath::Sqrt(fDistSQR);

  return m_vCenter + m_fRadius * (vDir / fLength);
}

template <typename Type>
bool plBoundingSphereTemplate<Type>::Contains(
  const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride /* = sizeof(plVec3Template) */) const
{
  PLASMA_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  PLASMA_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");
  PLASMA_ASSERT_DEBUG(uiStride >= sizeof(plVec3Template<Type>), "The data must not overlap.");

  const Type fRadiusSQR = plMath::Square(m_fRadius);

  const plVec3Template<Type>* pCur = &pPoints[0];

  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if ((*pCur - m_vCenter).GetLengthSquared() > fRadiusSQR)
      return false;

    pCur = plMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return true;
}

template <typename Type>
bool plBoundingSphereTemplate<Type>::Overlaps(
  const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride /* = sizeof(plVec3Template) */) const
{
  PLASMA_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  PLASMA_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");
  PLASMA_ASSERT_DEBUG(uiStride >= sizeof(plVec3Template<Type>), "The data must not overlap.");

  const Type fRadiusSQR = plMath::Square(m_fRadius);

  const plVec3Template<Type>* pCur = &pPoints[0];

  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if ((*pCur - m_vCenter).GetLengthSquared() <= fRadiusSQR)
      return true;

    pCur = plMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return false;
}

template <typename Type>
void plBoundingSphereTemplate<Type>::SetFromPoints(
  const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride /* = sizeof(plVec3Template) */)
{
  PLASMA_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  PLASMA_ASSERT_DEBUG(uiStride >= sizeof(plVec3Template<Type>), "The data must not overlap.");
  PLASMA_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");

  const plVec3Template<Type>* pCur = &pPoints[0];

  plVec3Template<Type> vCenter(0.0f);

  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    vCenter += *pCur;
    pCur = plMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  vCenter /= (Type)uiNumPoints;

  Type fMaxDistSQR = 0.0f;

  pCur = &pPoints[0];
  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type fDistSQR = (*pCur - vCenter).GetLengthSquared();
    fMaxDistSQR = plMath::Max(fMaxDistSQR, fDistSQR);

    pCur = plMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  m_vCenter = vCenter;
  m_fRadius = plMath::Sqrt(fMaxDistSQR);

  PLASMA_ASSERT_DEBUG(IsValid(), "The point cloud contained corrupted data.");
}

template <typename Type>
void plBoundingSphereTemplate<Type>::ExpandToInclude(
  const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride /* = sizeof(plVec3Template) */)
{
  PLASMA_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  PLASMA_ASSERT_DEBUG(uiStride >= sizeof(plVec3Template<Type>), "The data must not overlap.");

  const plVec3Template<Type>* pCur = &pPoints[0];

  Type fMaxDistSQR = 0.0f;

  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type fDistSQR = (*pCur - m_vCenter).GetLengthSquared();
    fMaxDistSQR = plMath::Max(fMaxDistSQR, fDistSQR);

    pCur = plMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  if (plMath::Square(m_fRadius) < fMaxDistSQR)
    m_fRadius = plMath::Sqrt(fMaxDistSQR);
}

template <typename Type>
Type plBoundingSphereTemplate<Type>::GetDistanceTo(
  const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride /* = sizeof(plVec3Template) */) const
{
  PLASMA_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  PLASMA_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");
  PLASMA_ASSERT_DEBUG(uiStride >= sizeof(plVec3Template<Type>), "The data must not overlap.");

  const plVec3Template<Type>* pCur = &pPoints[0];

  Type fMinDistSQR = plMath::MaxValue<Type>();

  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type fDistSQR = (*pCur - m_vCenter).GetLengthSquared();

    fMinDistSQR = plMath::Min(fMinDistSQR, fDistSQR);

    pCur = plMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return plMath::Sqrt(fMinDistSQR);
}

template <typename Type>
bool plBoundingSphereTemplate<Type>::GetRayIntersection(const plVec3Template<Type>& vRayStartPos, const plVec3Template<Type>& vRayDirNormalized,
  Type* out_pIntersectionDistance /* = nullptr */, plVec3Template<Type>* out_pIntersection /* = nullptr */) const
{
  PLASMA_ASSERT_DEBUG(vRayDirNormalized.IsNormalized(), "The ray direction must be normalized.");

  // Ugly Code taken from 'Real Time Rendering First Edition' Page 299

  const Type fRadiusSQR = plMath::Square(m_fRadius);
  const plVec3Template<Type> vRelPos = m_vCenter - vRayStartPos;

  const Type d = vRelPos.Dot(vRayDirNormalized);
  const Type fRelPosLenSQR = vRelPos.GetLengthSquared();

  if (d < 0.0f && fRelPosLenSQR > fRadiusSQR)
    return false;

  const Type m2 = fRelPosLenSQR - plMath::Square(d);

  if (m2 > fRadiusSQR)
    return false;

  const Type q = plMath::Sqrt(fRadiusSQR - m2);

  Type fIntersectionTime;

  if (fRelPosLenSQR > fRadiusSQR)
    fIntersectionTime = d - q;
  else
    fIntersectionTime = d + q;

  if (out_pIntersectionDistance)
    *out_pIntersectionDistance = fIntersectionTime;
  if (out_pIntersection)
    *out_pIntersection = vRayStartPos + vRayDirNormalized * fIntersectionTime;

  return true;
}

template <typename Type>
bool plBoundingSphereTemplate<Type>::GetLineSegmentIntersection(const plVec3Template<Type>& vLineStartPos, const plVec3Template<Type>& vLineEndPos,
  Type* out_pHitFraction /* = nullptr */, plVec3Template<Type>* out_pIntersection /* = nullptr */) const
{
  Type fIntersection = 0.0f;

  const plVec3Template<Type> vDir = vLineEndPos - vLineStartPos;
  plVec3Template<Type> vDirNorm = vDir;
  const Type fLen = vDirNorm.GetLengthAndNormalize();

  if (!GetRayIntersection(vLineStartPos, vDirNorm, &fIntersection))
    return false;

  if (fIntersection > fLen)
    return false;

  if (out_pHitFraction)
    *out_pHitFraction = fIntersection / fLen;

  if (out_pIntersection)
    *out_pIntersection = vLineStartPos + vDirNorm * fIntersection;

  return true;
}

#include <Foundation/Math/Implementation/AllClasses_inl.h>

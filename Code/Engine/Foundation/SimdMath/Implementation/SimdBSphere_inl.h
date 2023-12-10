#pragma once

PLASMA_ALWAYS_INLINE plSimdBSphere::plSimdBSphere() = default;

PLASMA_ALWAYS_INLINE plSimdBSphere::plSimdBSphere(const plSimdVec4f& vCenter, const plSimdFloat& fRadius)
  : m_CenterAndRadius(vCenter)
{
  m_CenterAndRadius.SetW(fRadius);
}

PLASMA_ALWAYS_INLINE plSimdBSphere plSimdBSphere::MakeZero()
{
  plSimdBSphere res;
  res.m_CenterAndRadius = plSimdVec4f::MakeZero();
  return res;
}

PLASMA_ALWAYS_INLINE plSimdBSphere plSimdBSphere::MakeInvalid(const plSimdVec4f& vCenter /*= plSimdVec4f::MakeZero()*/)
{
  plSimdBSphere res;
  res.m_CenterAndRadius.Set(0.0f, 0.0f, 0.0f, -plMath::SmallEpsilon<float>());
  return res;
}

PLASMA_ALWAYS_INLINE plSimdBSphere plSimdBSphere::MakeFromCenterAndRadius(const plSimdVec4f& vCenter, const plSimdFloat& fRadius)
{
  return plSimdBSphere(vCenter, fRadius);
}

inline plSimdBSphere plSimdBSphere::MakeFromPoints(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride /*= sizeof(plSimdVec4f)*/)
{
  PLASMA_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  PLASMA_ASSERT_DEBUG(uiStride >= sizeof(plSimdVec4f), "The data must not overlap.");
  PLASMA_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");

  plSimdBSphere res;

  const plSimdVec4f* pCur = pPoints;

  plSimdVec4f vCenter = plSimdVec4f::MakeZero();
  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    vCenter += *pCur;
    pCur = plMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  res.m_CenterAndRadius = vCenter / plSimdFloat(uiNumPoints);

  pCur = pPoints;

  plSimdFloat fMaxDistSquare = plSimdFloat::MakeZero();
  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const plSimdFloat fDistSQR = (*pCur - res.m_CenterAndRadius).GetLengthSquared<3>();
    fMaxDistSquare = fMaxDistSquare.Max(fDistSQR);

    pCur = plMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  res.m_CenterAndRadius.SetW(fMaxDistSquare.GetSqrt());

  return res;
}

PLASMA_ALWAYS_INLINE void plSimdBSphere::SetInvalid()
{
  m_CenterAndRadius.Set(0.0f, 0.0f, 0.0f, -plMath::SmallEpsilon<float>());
}

PLASMA_ALWAYS_INLINE bool plSimdBSphere::IsValid() const
{
  return m_CenterAndRadius.IsValid<4>() && GetRadius() >= plSimdFloat::MakeZero();
}

PLASMA_ALWAYS_INLINE bool plSimdBSphere::IsNaN() const
{
  return m_CenterAndRadius.IsNaN<4>();
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdBSphere::GetCenter() const
{
  return m_CenterAndRadius;
}

PLASMA_ALWAYS_INLINE plSimdFloat plSimdBSphere::GetRadius() const
{
  return m_CenterAndRadius.w();
}

PLASMA_ALWAYS_INLINE void plSimdBSphere::SetFromPoints(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride)
{
  *this = MakeFromPoints(pPoints, uiNumPoints, uiStride);
}

PLASMA_ALWAYS_INLINE void plSimdBSphere::ExpandToInclude(const plSimdVec4f& vPoint)
{
  const plSimdFloat fDist = (vPoint - m_CenterAndRadius).GetLength<3>();

  m_CenterAndRadius.SetW(fDist.Max(GetRadius()));
}

inline void plSimdBSphere::ExpandToInclude(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride)
{
  PLASMA_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  PLASMA_ASSERT_DEBUG(uiStride >= sizeof(plSimdVec4f), "The data must not overlap.");

  const plSimdVec4f* pCur = pPoints;

  plSimdFloat fMaxDistSquare = plSimdFloat::MakeZero();

  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const plSimdFloat fDistSQR = (*pCur - m_CenterAndRadius).GetLengthSquared<3>();
    fMaxDistSquare = fMaxDistSquare.Max(fDistSQR);

    pCur = plMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  m_CenterAndRadius.SetW(fMaxDistSquare.GetSqrt().Max(GetRadius()));
}

PLASMA_ALWAYS_INLINE void plSimdBSphere::ExpandToInclude(const plSimdBSphere& rhs)
{
  const plSimdFloat fReqRadius = (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() + rhs.GetRadius();

  m_CenterAndRadius.SetW(fReqRadius.Max(GetRadius()));
}

inline void plSimdBSphere::Transform(const plSimdTransform& t)
{
  plSimdVec4f newCenterAndRadius = t.TransformPosition(m_CenterAndRadius);
  newCenterAndRadius.SetW(t.GetMaxScale() * GetRadius());

  m_CenterAndRadius = newCenterAndRadius;
}

inline void plSimdBSphere::Transform(const plSimdMat4f& mMat)
{
  plSimdFloat radius = m_CenterAndRadius.w();
  m_CenterAndRadius = mMat.TransformPosition(m_CenterAndRadius);

  plSimdFloat maxRadius = mMat.m_col0.Dot<3>(mMat.m_col0);
  maxRadius = maxRadius.Max(mMat.m_col1.Dot<3>(mMat.m_col1));
  maxRadius = maxRadius.Max(mMat.m_col2.Dot<3>(mMat.m_col2));
  radius *= maxRadius.GetSqrt();

  m_CenterAndRadius.SetW(radius);
}

PLASMA_ALWAYS_INLINE plSimdFloat plSimdBSphere::GetDistanceTo(const plSimdVec4f& vPoint) const
{
  return (vPoint - m_CenterAndRadius).GetLength<3>() - GetRadius();
}

PLASMA_ALWAYS_INLINE plSimdFloat plSimdBSphere::GetDistanceTo(const plSimdBSphere& rhs) const
{
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() - GetRadius() - rhs.GetRadius();
}

PLASMA_ALWAYS_INLINE bool plSimdBSphere::Contains(const plSimdVec4f& vPoint) const
{
  plSimdFloat radius = GetRadius();
  return (vPoint - m_CenterAndRadius).GetLengthSquared<3>() <= (radius * radius);
}

PLASMA_ALWAYS_INLINE bool plSimdBSphere::Contains(const plSimdBSphere& rhs) const
{
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() + rhs.GetRadius() <= GetRadius();
}

PLASMA_ALWAYS_INLINE bool plSimdBSphere::Overlaps(const plSimdBSphere& rhs) const
{
  plSimdFloat radius = (rhs.m_CenterAndRadius + m_CenterAndRadius).w();
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLengthSquared<3>() < (radius * radius);
}

inline plSimdVec4f plSimdBSphere::GetClampedPoint(const plSimdVec4f& vPoint)
{
  plSimdVec4f vDir = vPoint - m_CenterAndRadius;
  plSimdFloat fDist = vDir.GetLengthAndNormalize<3>().Min(GetRadius());

  return m_CenterAndRadius + (vDir * fDist);
}

PLASMA_ALWAYS_INLINE bool plSimdBSphere::operator==(const plSimdBSphere& rhs) const
{
  return (m_CenterAndRadius == rhs.m_CenterAndRadius).AllSet();
}

PLASMA_ALWAYS_INLINE bool plSimdBSphere::operator!=(const plSimdBSphere& rhs) const
{
  return (m_CenterAndRadius != rhs.m_CenterAndRadius).AnySet();
}

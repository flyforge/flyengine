#pragma once

PLASMA_ALWAYS_INLINE plSimdBBoxSphere::plSimdBBoxSphere() = default;

PLASMA_ALWAYS_INLINE plSimdBBoxSphere::plSimdBBoxSphere(const plSimdVec4f& vCenter, const plSimdVec4f& vBoxHalfExtents, const plSimdFloat& fSphereRadius)
  : m_CenterAndRadius(vCenter)
  , m_BoxHalfExtents(vBoxHalfExtents)
{
  m_CenterAndRadius.SetW(fSphereRadius);
}

inline plSimdBBoxSphere::plSimdBBoxSphere(const plSimdBBox& box, const plSimdBSphere& sphere)
  : m_CenterAndRadius(box.GetCenter())
  , m_BoxHalfExtents(m_CenterAndRadius - box.m_Min)
{
  m_CenterAndRadius.SetW(m_BoxHalfExtents.GetLength<3>().Min((sphere.GetCenter() - m_CenterAndRadius).GetLength<3>() + sphere.GetRadius()));
}

inline plSimdBBoxSphere::plSimdBBoxSphere(const plSimdBBox& box)
  : m_CenterAndRadius(box.GetCenter())
  , m_BoxHalfExtents(m_CenterAndRadius - box.m_Min)
{
  m_CenterAndRadius.SetW(m_BoxHalfExtents.GetLength<3>());
}

PLASMA_ALWAYS_INLINE plSimdBBoxSphere::plSimdBBoxSphere(const plSimdBSphere& sphere)
  : m_CenterAndRadius(sphere.m_CenterAndRadius)
  , m_BoxHalfExtents(plSimdVec4f(sphere.GetRadius()))
{
}

PLASMA_ALWAYS_INLINE void plSimdBBoxSphere::SetInvalid()
{
  m_CenterAndRadius.Set(0.0f, 0.0f, 0.0f, -plMath::SmallEpsilon<float>());
  m_BoxHalfExtents.Set(-plMath::MaxValue<float>());
}

PLASMA_ALWAYS_INLINE bool plSimdBBoxSphere::IsValid() const
{
  return m_CenterAndRadius.IsValid<4>() && m_CenterAndRadius.w() >= plSimdFloat::Zero() && m_BoxHalfExtents.IsValid<3>() &&
         (m_BoxHalfExtents >= plSimdVec4f::ZeroVector()).AllSet<3>();
}

inline bool plSimdBBoxSphere::IsNaN() const
{
  return m_CenterAndRadius.IsNaN<4>() || m_BoxHalfExtents.IsNaN<3>();
}

inline void plSimdBBoxSphere::SetFromPoints(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride)
{
  plSimdBBox box;
  box.SetFromPoints(pPoints, uiNumPoints, uiStride);

  m_CenterAndRadius = box.GetCenter();
  m_BoxHalfExtents = m_CenterAndRadius - box.m_Min;

  plSimdBSphere sphere(m_CenterAndRadius, plSimdFloat::Zero());
  sphere.ExpandToInclude(pPoints, uiNumPoints, uiStride);

  m_CenterAndRadius.SetW(sphere.GetRadius());
}

PLASMA_ALWAYS_INLINE plSimdBBox plSimdBBoxSphere::GetBox() const
{
  plSimdBBox box;
  box.SetCenterAndHalfExtents(m_CenterAndRadius, m_BoxHalfExtents);
  return box;
}

PLASMA_ALWAYS_INLINE plSimdBSphere plSimdBBoxSphere::GetSphere() const
{
  plSimdBSphere sphere;
  sphere.m_CenterAndRadius = m_CenterAndRadius;
  return sphere;
}

inline void plSimdBBoxSphere::ExpandToInclude(const plSimdBBoxSphere& rhs)
{
  plSimdBBox box = GetBox();
  box.ExpandToInclude(rhs.GetBox());

  plSimdVec4f center = box.GetCenter();
  plSimdVec4f boxHalfExtents = center - box.m_Min;
  plSimdFloat tmpRadius = boxHalfExtents.GetLength<3>();

  const plSimdFloat fSphereRadiusA = (m_CenterAndRadius - center).GetLength<3>() + m_CenterAndRadius.w();
  const plSimdFloat fSphereRadiusB = (rhs.m_CenterAndRadius - center).GetLength<3>() + rhs.m_CenterAndRadius.w();

  m_CenterAndRadius = center;
  m_CenterAndRadius.SetW(tmpRadius.Min(fSphereRadiusA.Max(fSphereRadiusB)));
  m_BoxHalfExtents = boxHalfExtents;
}

PLASMA_ALWAYS_INLINE void plSimdBBoxSphere::Transform(const plSimdTransform& t)
{
  Transform(t.GetAsMat4());
}

PLASMA_ALWAYS_INLINE void plSimdBBoxSphere::Transform(const plSimdMat4f& mMat)
{
  plSimdFloat radius = m_CenterAndRadius.w();
  m_CenterAndRadius = mMat.TransformPosition(m_CenterAndRadius);

  plSimdFloat maxRadius = mMat.m_col0.Dot<3>(mMat.m_col0);
  maxRadius = maxRadius.Max(mMat.m_col1.Dot<3>(mMat.m_col1));
  maxRadius = maxRadius.Max(mMat.m_col2.Dot<3>(mMat.m_col2));
  radius *= maxRadius.GetSqrt();

  m_CenterAndRadius.SetW(radius);

  plSimdVec4f newHalfExtents = mMat.m_col0.Abs() * m_BoxHalfExtents.x();
  newHalfExtents += mMat.m_col1.Abs() * m_BoxHalfExtents.y();
  newHalfExtents += mMat.m_col2.Abs() * m_BoxHalfExtents.z();

  m_BoxHalfExtents = newHalfExtents.CompMin(plSimdVec4f(radius));
}

PLASMA_ALWAYS_INLINE bool plSimdBBoxSphere::operator==(const plSimdBBoxSphere& rhs) const
{
  return (m_CenterAndRadius == rhs.m_CenterAndRadius).AllSet<4>() && (m_BoxHalfExtents == rhs.m_BoxHalfExtents).AllSet<3>();
}

PLASMA_ALWAYS_INLINE bool plSimdBBoxSphere::operator!=(const plSimdBBoxSphere& rhs) const
{
  return !(*this == rhs);
}

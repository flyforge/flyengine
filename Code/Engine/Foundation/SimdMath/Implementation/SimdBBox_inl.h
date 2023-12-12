#pragma once

PLASMA_ALWAYS_INLINE plSimdBBox::plSimdBBox() = default;

PLASMA_ALWAYS_INLINE plSimdBBox::plSimdBBox(const plSimdVec4f& vMin, const plSimdVec4f& vMax)
  : m_Min(vMin)
  , m_Max(vMax)
{
}

PLASMA_ALWAYS_INLINE void plSimdBBox::SetInvalid()
{
  m_Min.Set(plMath::MaxValue<float>());
  m_Max.Set(-plMath::MaxValue<float>());
}

PLASMA_ALWAYS_INLINE void plSimdBBox::SetCenterAndHalfExtents(const plSimdVec4f& vCenter, const plSimdVec4f& vHalfExtents)
{
  m_Min = vCenter - vHalfExtents;
  m_Max = vCenter + vHalfExtents;
}

PLASMA_ALWAYS_INLINE void plSimdBBox::SetFromPoints(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride)
{
  SetInvalid();
  ExpandToInclude(pPoints, uiNumPoints, uiStride);
}

PLASMA_ALWAYS_INLINE bool plSimdBBox::IsValid() const
{
  return m_Min.IsValid<3>() && m_Max.IsValid<3>() && (m_Min <= m_Max).AllSet<3>();
}

PLASMA_ALWAYS_INLINE bool plSimdBBox::IsNaN() const
{
  return m_Min.IsNaN<3>() || m_Max.IsNaN<3>();
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdBBox::GetCenter() const
{
  return (m_Min + m_Max) * plSimdFloat(0.5f);
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdBBox::GetExtents() const
{
  return m_Max - m_Min;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdBBox::GetHalfExtents() const
{
  return (m_Max - m_Min) * plSimdFloat(0.5f);
}

PLASMA_ALWAYS_INLINE void plSimdBBox::ExpandToInclude(const plSimdVec4f& vPoint)
{
  m_Min = m_Min.CompMin(vPoint);
  m_Max = m_Max.CompMax(vPoint);
}

inline void plSimdBBox::ExpandToInclude(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride)
{
  PLASMA_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  PLASMA_ASSERT_DEBUG(uiStride >= sizeof(plSimdVec4f), "Data may not overlap.");

  const plSimdVec4f* pCur = pPoints;

  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    ExpandToInclude(*pCur);

    pCur = plMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

PLASMA_ALWAYS_INLINE void plSimdBBox::ExpandToInclude(const plSimdBBox& rhs)
{
  m_Min = m_Min.CompMin(rhs.m_Min);
  m_Max = m_Max.CompMax(rhs.m_Max);
}

inline void plSimdBBox::ExpandToCube()
{
  const plSimdVec4f center = GetCenter();
  const plSimdVec4f halfExtents = center - m_Min;

  SetCenterAndHalfExtents(center, plSimdVec4f(halfExtents.HorizontalMax<3>()));
}

PLASMA_ALWAYS_INLINE bool plSimdBBox::Contains(const plSimdVec4f& vPoint) const
{
  return ((vPoint >= m_Min) && (vPoint <= m_Max)).AllSet<3>();
}

PLASMA_ALWAYS_INLINE bool plSimdBBox::Contains(const plSimdBBox& rhs) const
{
  return Contains(rhs.m_Min) && Contains(rhs.m_Max);
}

inline bool plSimdBBox::Contains(const plSimdBSphere& rhs) const
{
  plSimdBBox otherBox;
  otherBox.SetCenterAndHalfExtents(rhs.GetCenter(), plSimdVec4f(rhs.GetRadius()));

  return Contains(otherBox);
}

PLASMA_ALWAYS_INLINE bool plSimdBBox::Overlaps(const plSimdBBox& rhs) const
{
  return ((m_Max > rhs.m_Min) && (m_Min < rhs.m_Max)).AllSet<3>();
}

inline bool plSimdBBox::Overlaps(const plSimdBSphere& rhs) const
{
  // check whether the closest point between box and sphere is inside the sphere (it is definitely inside the box)
  return rhs.Contains(GetClampedPoint(rhs.GetCenter()));
}

PLASMA_ALWAYS_INLINE void plSimdBBox::Grow(const plSimdVec4f& vDiff)
{
  m_Max += vDiff;
  m_Min -= vDiff;
}

PLASMA_ALWAYS_INLINE void plSimdBBox::Translate(const plSimdVec4f& vDiff)
{
  m_Min += vDiff;
  m_Max += vDiff;
}

PLASMA_ALWAYS_INLINE void plSimdBBox::Transform(const plSimdTransform& t)
{
  Transform(t.GetAsMat4());
}

PLASMA_ALWAYS_INLINE void plSimdBBox::Transform(const plSimdMat4f& mMat)
{
  const plSimdVec4f center = GetCenter();
  const plSimdVec4f halfExtents = center - m_Min;

  const plSimdVec4f newCenter = mMat.TransformPosition(center);

  plSimdVec4f newHalfExtents = mMat.m_col0.Abs() * halfExtents.x();
  newHalfExtents += mMat.m_col1.Abs() * halfExtents.y();
  newHalfExtents += mMat.m_col2.Abs() * halfExtents.z();

  SetCenterAndHalfExtents(newCenter, newHalfExtents);
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdBBox::GetClampedPoint(const plSimdVec4f& vPoint) const
{
  return vPoint.CompMin(m_Max).CompMax(m_Min);
}

inline plSimdFloat plSimdBBox::GetDistanceSquaredTo(const plSimdVec4f& vPoint) const
{
  const plSimdVec4f vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLengthSquared<3>();
}

inline plSimdFloat plSimdBBox::GetDistanceTo(const plSimdVec4f& vPoint) const
{
  const plSimdVec4f vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLength<3>();
}

PLASMA_ALWAYS_INLINE bool plSimdBBox::operator==(const plSimdBBox& rhs) const
{
  return ((m_Min == rhs.m_Min) && (m_Max == rhs.m_Max)).AllSet<3>();
}

PLASMA_ALWAYS_INLINE bool plSimdBBox::operator!=(const plSimdBBox& rhs) const
{
  return ((m_Min != rhs.m_Min) || (m_Max != rhs.m_Max)).AnySet<3>();
}

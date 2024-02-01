#pragma once

PL_ALWAYS_INLINE plSimdBBox::plSimdBBox() = default;

PL_ALWAYS_INLINE plSimdBBox::plSimdBBox(const plSimdVec4f& vMin, const plSimdVec4f& vMax)
  : m_Min(vMin)
  , m_Max(vMax)
{
}

PL_ALWAYS_INLINE plSimdBBox plSimdBBox::MakeZero()
{
  return plSimdBBox(plSimdVec4f::MakeZero(), plSimdVec4f::MakeZero());
}

PL_ALWAYS_INLINE plSimdBBox plSimdBBox::MakeInvalid()
{
  return plSimdBBox(plSimdVec4f(plMath::MaxValue<float>()), plSimdVec4f(-plMath::MaxValue<float>()));
}

PL_ALWAYS_INLINE plSimdBBox plSimdBBox::MakeFromCenterAndHalfExtents(const plSimdVec4f& vCenter, const plSimdVec4f& vHalfExtents)
{
  return plSimdBBox(vCenter - vHalfExtents, vCenter + vHalfExtents);
}

PL_ALWAYS_INLINE plSimdBBox plSimdBBox::MakeFromMinMax(const plSimdVec4f& vMin, const plSimdVec4f& vMax)
{
  return plSimdBBox(vMin, vMax);
}

PL_ALWAYS_INLINE plSimdBBox plSimdBBox::MakeFromPoints(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride /*= sizeof(plSimdVec4f)*/)
{
  plSimdBBox box = plSimdBBox::MakeInvalid();
  box.ExpandToInclude(pPoints, uiNumPoints, uiStride);
  return box;
}

PL_ALWAYS_INLINE void plSimdBBox::SetInvalid()
{
  m_Min.Set(plMath::MaxValue<float>());
  m_Max.Set(-plMath::MaxValue<float>());
}

PL_ALWAYS_INLINE void plSimdBBox::SetCenterAndHalfExtents(const plSimdVec4f& vCenter, const plSimdVec4f& vHalfExtents)
{
  m_Min = vCenter - vHalfExtents;
  m_Max = vCenter + vHalfExtents;
}

PL_ALWAYS_INLINE void plSimdBBox::SetFromPoints(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride)
{
  *this = MakeInvalid();
  ExpandToInclude(pPoints, uiNumPoints, uiStride);
}

PL_ALWAYS_INLINE bool plSimdBBox::IsValid() const
{
  return m_Min.IsValid<3>() && m_Max.IsValid<3>() && (m_Min <= m_Max).AllSet<3>();
}

PL_ALWAYS_INLINE bool plSimdBBox::IsNaN() const
{
  return m_Min.IsNaN<3>() || m_Max.IsNaN<3>();
}

PL_ALWAYS_INLINE plSimdVec4f plSimdBBox::GetCenter() const
{
  return (m_Min + m_Max) * plSimdFloat(0.5f);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdBBox::GetExtents() const
{
  return m_Max - m_Min;
}

PL_ALWAYS_INLINE plSimdVec4f plSimdBBox::GetHalfExtents() const
{
  return (m_Max - m_Min) * plSimdFloat(0.5f);
}

PL_ALWAYS_INLINE void plSimdBBox::ExpandToInclude(const plSimdVec4f& vPoint)
{
  m_Min = m_Min.CompMin(vPoint);
  m_Max = m_Max.CompMax(vPoint);
}

inline void plSimdBBox::ExpandToInclude(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride)
{
  PL_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  PL_ASSERT_DEBUG(uiStride >= sizeof(plSimdVec4f), "Data may not overlap.");

  const plSimdVec4f* pCur = pPoints;

  for (plUInt32 i = 0; i < uiNumPoints; ++i)
  {
    ExpandToInclude(*pCur);

    pCur = plMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

PL_ALWAYS_INLINE void plSimdBBox::ExpandToInclude(const plSimdBBox& rhs)
{
  m_Min = m_Min.CompMin(rhs.m_Min);
  m_Max = m_Max.CompMax(rhs.m_Max);
}

inline void plSimdBBox::ExpandToCube()
{
  const plSimdVec4f center = GetCenter();
  const plSimdVec4f halfExtents = center - m_Min;

  *this = plSimdBBox::MakeFromCenterAndHalfExtents(center, plSimdVec4f(halfExtents.HorizontalMax<3>()));
}

PL_ALWAYS_INLINE bool plSimdBBox::Contains(const plSimdVec4f& vPoint) const
{
  return ((vPoint >= m_Min) && (vPoint <= m_Max)).AllSet<3>();
}

PL_ALWAYS_INLINE bool plSimdBBox::Contains(const plSimdBBox& rhs) const
{
  return Contains(rhs.m_Min) && Contains(rhs.m_Max);
}

inline bool plSimdBBox::Contains(const plSimdBSphere& rhs) const
{
  const plSimdBBox otherBox = plSimdBBox::MakeFromCenterAndHalfExtents(rhs.GetCenter(), plSimdVec4f(rhs.GetRadius()));

  return Contains(otherBox);
}

PL_ALWAYS_INLINE bool plSimdBBox::Overlaps(const plSimdBBox& rhs) const
{
  return ((m_Max > rhs.m_Min) && (m_Min < rhs.m_Max)).AllSet<3>();
}

inline bool plSimdBBox::Overlaps(const plSimdBSphere& rhs) const
{
  // check whether the closest point between box and sphere is inside the sphere (it is definitely inside the box)
  return rhs.Contains(GetClampedPoint(rhs.GetCenter()));
}

PL_ALWAYS_INLINE void plSimdBBox::Grow(const plSimdVec4f& vDiff)
{
  m_Max += vDiff;
  m_Min -= vDiff;
}

PL_ALWAYS_INLINE void plSimdBBox::Translate(const plSimdVec4f& vDiff)
{
  m_Min += vDiff;
  m_Max += vDiff;
}

PL_ALWAYS_INLINE void plSimdBBox::Transform(const plSimdTransform& t)
{
  Transform(t.GetAsMat4());
}

PL_ALWAYS_INLINE void plSimdBBox::Transform(const plSimdMat4f& mMat)
{
  const plSimdVec4f center = GetCenter();
  const plSimdVec4f halfExtents = center - m_Min;

  const plSimdVec4f newCenter = mMat.TransformPosition(center);

  plSimdVec4f newHalfExtents = mMat.m_col0.Abs() * halfExtents.x();
  newHalfExtents += mMat.m_col1.Abs() * halfExtents.y();
  newHalfExtents += mMat.m_col2.Abs() * halfExtents.z();

  *this = plSimdBBox::MakeFromCenterAndHalfExtents(newCenter, newHalfExtents);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdBBox::GetClampedPoint(const plSimdVec4f& vPoint) const
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

PL_ALWAYS_INLINE bool plSimdBBox::operator==(const plSimdBBox& rhs) const
{
  return ((m_Min == rhs.m_Min) && (m_Max == rhs.m_Max)).AllSet<3>();
}

PL_ALWAYS_INLINE bool plSimdBBox::operator!=(const plSimdBBox& rhs) const
{
  return ((m_Min != rhs.m_Min) || (m_Max != rhs.m_Max)).AnySet<3>();
}

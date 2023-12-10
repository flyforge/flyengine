#pragma once

template <typename Type>
PLASMA_FORCE_INLINE plBoundingBoxSphereTemplate<Type>::plBoundingBoxSphereTemplate()
{
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  // m_vOrigin and m_vBoxHalfExtends are already initialized to NaN by their own constructor.
  const Type TypeNaN = plMath::NaN<Type>();
  m_fSphereRadius = TypeNaN;
#endif
}

template <typename Type>
PLASMA_FORCE_INLINE plBoundingBoxSphereTemplate<Type>::plBoundingBoxSphereTemplate(const plBoundingBoxSphereTemplate& rhs)
{
  m_vCenter = rhs.m_vCenter;
  m_fSphereRadius = rhs.m_fSphereRadius;
  m_vBoxHalfExtends = rhs.m_vBoxHalfExtends;
}

template <typename Type>
void plBoundingBoxSphereTemplate<Type>::operator=(const plBoundingBoxSphereTemplate& rhs)
{
  m_vCenter = rhs.m_vCenter;
  m_fSphereRadius = rhs.m_fSphereRadius;
  m_vBoxHalfExtends = rhs.m_vBoxHalfExtends;
}

template <typename Type>
plBoundingBoxSphereTemplate<Type>::plBoundingBoxSphereTemplate(const plBoundingBoxTemplate<Type>& box)
  : m_vCenter(box.GetCenter())
{
  m_vBoxHalfExtends = box.GetHalfExtents();
  m_fSphereRadius = m_vBoxHalfExtends.GetLength();
}

template <typename Type>
plBoundingBoxSphereTemplate<Type>::plBoundingBoxSphereTemplate(const plBoundingSphereTemplate<Type>& sphere)
  : m_vCenter(sphere.m_vCenter)
  , m_fSphereRadius(sphere.m_fRadius)
{
  m_vBoxHalfExtends.Set(m_fSphereRadius);
}


template <typename Type>
PLASMA_FORCE_INLINE plBoundingBoxSphereTemplate<Type> plBoundingBoxSphereTemplate<Type>::MakeZero()
{
  plBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter.SetZero();
  res.m_fSphereRadius = 0;
  res.m_vBoxHalfExtends.SetZero();
  return res;
}

template <typename Type>
PLASMA_FORCE_INLINE plBoundingBoxSphereTemplate<Type> plBoundingBoxSphereTemplate<Type>::MakeInvalid()
{
  plBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter.SetZero();
  res.m_fSphereRadius = -plMath::SmallEpsilon<Type>(); // has to be very small for ExpandToInclude to work
  res.m_vBoxHalfExtends.Set(-plMath::MaxValue<Type>());
  return res;
}

template <typename Type>
PLASMA_FORCE_INLINE plBoundingBoxSphereTemplate<Type> plBoundingBoxSphereTemplate<Type>::MakeFromCenterExtents(const plVec3Template<Type>& vCenter, const plVec3Template<Type>& vBoxHalfExtents, Type fSphereRadius)
{
  plBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = vCenter;
  res.m_fSphereRadius = fSphereRadius;
  res.m_vBoxHalfExtends = vBoxHalfExtents;
  return res;
}

template <typename Type>
plBoundingBoxSphereTemplate<Type> plBoundingBoxSphereTemplate<Type>::MakeFromPoints(const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride /*= sizeof(plVec3Template<Type>)*/)
{
  plBoundingBoxTemplate<Type> box = plBoundingBoxTemplate<Type>::MakeFromPoints(pPoints, uiNumPoints, uiStride);

  plBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = box.GetCenter();
  res.m_vBoxHalfExtends = box.GetHalfExtents();

  plBoundingSphereTemplate<Type> sphere = plBoundingSphereTemplate<Type>::MakeFromCenterAndRadius(res.m_vCenter, 0.0f);
  sphere.ExpandToInclude(pPoints, uiNumPoints, uiStride);

  res.m_fSphereRadius = sphere.m_fRadius;
  return res;
}

template <typename Type>
plBoundingBoxSphereTemplate<Type> plBoundingBoxSphereTemplate<Type>::MakeFromBox(const plBoundingBoxTemplate<Type>& box)
{
  plBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = box.GetCenter();
  res.m_vBoxHalfExtends = box.GetHalfExtents();
  res.m_fSphereRadius = res.m_vBoxHalfExtends.GetLength();
  return res;
}

template <typename Type>
plBoundingBoxSphereTemplate<Type> plBoundingBoxSphereTemplate<Type>::MakeFromSphere(const plBoundingSphereTemplate<Type>& sphere)
{
  plBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = sphere.m_vCenter;
  res.m_fSphereRadius = sphere.m_fRadius;
  res.m_vBoxHalfExtends.Set(res.m_fSphereRadius);
  return res;
}

template <typename Type>
plBoundingBoxSphereTemplate<Type> plBoundingBoxSphereTemplate<Type>::MakeFromBoxAndSphere(const plBoundingBoxTemplate<Type>& box, const plBoundingSphereTemplate<Type>& sphere)
{
  plBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = box.GetCenter();
  res.m_vBoxHalfExtends = box.GetHalfExtents();
  res.m_fSphereRadius = plMath::Min(res.m_vBoxHalfExtends.GetLength(), (sphere.m_vCenter - res.m_vCenter).GetLength() + sphere.m_fRadius);
  return res;
}

template <typename Type>
PLASMA_FORCE_INLINE bool plBoundingBoxSphereTemplate<Type>::IsValid() const
{
  return (m_vCenter.IsValid() && m_fSphereRadius >= 0.0f && m_vBoxHalfExtends.IsValid());
}

template <typename Type>
PLASMA_FORCE_INLINE bool plBoundingBoxSphereTemplate<Type>::IsNaN() const
{
  return (m_vCenter.IsNaN() || plMath::IsNaN(m_fSphereRadius) || m_vBoxHalfExtends.IsNaN());
}

template <typename Type>
PLASMA_FORCE_INLINE const plBoundingBoxTemplate<Type> plBoundingBoxSphereTemplate<Type>::GetBox() const
{
  return plBoundingBoxTemplate<Type>::MakeFromMinMax(m_vCenter - m_vBoxHalfExtends, m_vCenter + m_vBoxHalfExtends);
}

template <typename Type>
PLASMA_FORCE_INLINE const plBoundingSphereTemplate<Type> plBoundingBoxSphereTemplate<Type>::GetSphere() const
{
  return plBoundingSphereTemplate<Type>::MakeFromCenterAndRadius(m_vCenter, m_fSphereRadius);
}

template <typename Type>
void plBoundingBoxSphereTemplate<Type>::ExpandToInclude(const plBoundingBoxSphereTemplate& rhs)
{
  plBoundingBoxTemplate<Type> box;
  box.m_vMin = m_vCenter - m_vBoxHalfExtends;
  box.m_vMax = m_vCenter + m_vBoxHalfExtends;
  box.ExpandToInclude(rhs.GetBox());

  plBoundingBoxSphereTemplate<Type> result = plBoundingBoxSphereTemplate<Type>::MakeFromBox(box);

  const float fSphereRadiusA = (m_vCenter - result.m_vCenter).GetLength() + m_fSphereRadius;
  const float fSphereRadiusB = (rhs.m_vCenter - result.m_vCenter).GetLength() + rhs.m_fSphereRadius;

  m_vCenter = result.m_vCenter;
  m_fSphereRadius = plMath::Min(result.m_fSphereRadius, plMath::Max(fSphereRadiusA, fSphereRadiusB));
  m_vBoxHalfExtends = result.m_vBoxHalfExtends;
}

template <typename Type>
void plBoundingBoxSphereTemplate<Type>::Transform(const plMat4Template<Type>& mTransform)
{
  m_vCenter = mTransform.TransformPosition(m_vCenter);
  const plVec3Template<Type> Scale = mTransform.GetScalingFactors();
  m_fSphereRadius *= plMath::Max(Scale.x, Scale.y, Scale.z);

  plMat3Template<Type> mAbsRotation = mTransform.GetRotationalPart();
  for (plUInt32 i = 0; i < 9; ++i)
  {
    mAbsRotation.m_fElementsCM[i] = plMath::Abs(mAbsRotation.m_fElementsCM[i]);
  }

  m_vBoxHalfExtends = mAbsRotation.TransformDirection(m_vBoxHalfExtends).CompMin(plVec3(m_fSphereRadius));
}

template <typename Type>
PLASMA_FORCE_INLINE bool operator==(const plBoundingBoxSphereTemplate<Type>& lhs, const plBoundingBoxSphereTemplate<Type>& rhs)
{
  return lhs.m_vCenter == rhs.m_vCenter && lhs.m_vBoxHalfExtends == rhs.m_vBoxHalfExtends && lhs.m_fSphereRadius == rhs.m_fSphereRadius;
}

/// \brief Checks whether this box and the other are not identical.
template <typename Type>
PLASMA_ALWAYS_INLINE bool operator!=(const plBoundingBoxSphereTemplate<Type>& lhs, const plBoundingBoxSphereTemplate<Type>& rhs)
{
  return !(lhs == rhs);
}

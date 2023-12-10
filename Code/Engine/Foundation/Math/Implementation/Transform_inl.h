#pragma once

#include <Foundation/Math/Transform.h>

template <typename Type>
inline plTransformTemplate<Type>::plTransformTemplate(const plVec3Template<Type>& vPosition,
  const plQuatTemplate<Type>& qRotation, const plVec3Template<Type>& vScale)
  : m_vPosition(vPosition)
  , m_qRotation(qRotation)
  , m_vScale(vScale)
{
}

template <typename Type>
inline plTransformTemplate<Type> plTransformTemplate<Type>::Make(const plVec3Template<Type>& vPosition, const plQuatTemplate<Type>& qRotation /*= plQuatTemplate<Type>::IdentityQuaternion()*/, const plVec3Template<Type>& vScale /*= plVec3Template<Type>(1)*/)
{
  plTransformTemplate<Type> res;
  res.m_vPosition = vPosition;
  res.m_qRotation = qRotation;
  res.m_vScale = vScale;
  return res;
}

template <typename Type>
inline plTransformTemplate<Type> plTransformTemplate<Type>::MakeIdentity()
{
  plTransformTemplate<Type> res;
  res.m_vPosition.SetZero();
  res.m_qRotation = plQuatTemplate<Type>::MakeIdentity();
  res.m_vScale.Set(1.0f);
  return res;
}

template <typename Type>
plTransformTemplate<Type> plTransformTemplate<Type>::MakeFromMat4(const plMat4Template<Type>& mMat)
{
  plMat3Template<Type> mRot = mMat.GetRotationalPart();

  plTransformTemplate<Type> res;
  res.m_vPosition = mMat.GetTranslationVector();
  res.m_vScale = mRot.GetScalingFactors();
  mRot.SetScalingFactors(plVec3Template<Type>(1)).IgnoreResult();
  res.m_qRotation = plQuat::MakeFromMat3(mRot);
  return res;
}

template <typename Type>
plTransformTemplate<Type> plTransformTemplate<Type>::MakeLocalTransform(const plTransformTemplate& globalTransformParent, const plTransformTemplate& globalTransformChild)
{
  const auto invRot = globalTransformParent.m_qRotation.GetInverse();
  const auto invScale = plVec3Template<Type>(1).CompDiv(globalTransformParent.m_vScale);

  plTransformTemplate<Type> res;
  res.m_vPosition = (invRot * (globalTransformChild.m_vPosition - globalTransformParent.m_vPosition)).CompMul(invScale);
  res.m_qRotation = invRot * globalTransformChild.m_qRotation;
  res.m_vScale = invScale.CompMul(globalTransformChild.m_vScale);
  return res;
}

template <typename Type>
PLASMA_ALWAYS_INLINE plTransformTemplate<Type> plTransformTemplate<Type>::MakeGlobalTransform(const plTransformTemplate& globalTransformParent, const plTransformTemplate& localTransformChild)
{
  return globalTransformParent * localTransformChild;
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plTransformTemplate<Type>::SetIdentity()
{
  *this = MakeIdentity();
}

template <typename Type>
PLASMA_ALWAYS_INLINE Type plTransformTemplate<Type>::GetMaxScale() const
{
  auto absScale = m_vScale.Abs();
  return plMath::Max(absScale.x, plMath::Max(absScale.y, absScale.z));
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool plTransformTemplate<Type>::ContainsNegativeScale() const
{
  return (m_vScale.x * m_vScale.y * m_vScale.z) < 0.0f;
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool plTransformTemplate<Type>::ContainsUniformScale() const
{
  const Type fEpsilon = plMath::DefaultEpsilon<Type>();
  return plMath::IsEqual(m_vScale.x, m_vScale.y, fEpsilon) && plMath::IsEqual(m_vScale.x, m_vScale.z, fEpsilon);
}

template <typename Type>
inline bool plTransformTemplate<Type>::IsIdentical(const plTransformTemplate<Type>& rhs) const
{
  return m_vPosition.IsIdentical(rhs.m_vPosition) && (m_qRotation == rhs.m_qRotation) && m_vScale.IsIdentical(rhs.m_vScale);
}

template <typename Type>
inline bool plTransformTemplate<Type>::IsEqual(const plTransformTemplate<Type>& rhs, Type fEpsilon) const
{
  return m_vPosition.IsEqual(rhs.m_vPosition, fEpsilon) && m_qRotation.IsEqualRotation(rhs.m_qRotation, fEpsilon) && m_vScale.IsEqual(rhs.m_vScale, fEpsilon);
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plMat4Template<Type> plTransformTemplate<Type>::GetAsMat4() const
{
  auto result = m_qRotation.GetAsMat4();

  result.m_fElementsCM[0] *= m_vScale.x;
  result.m_fElementsCM[1] *= m_vScale.x;
  result.m_fElementsCM[2] *= m_vScale.x;

  result.m_fElementsCM[4] *= m_vScale.y;
  result.m_fElementsCM[5] *= m_vScale.y;
  result.m_fElementsCM[6] *= m_vScale.y;

  result.m_fElementsCM[8] *= m_vScale.z;
  result.m_fElementsCM[9] *= m_vScale.z;
  result.m_fElementsCM[10] *= m_vScale.z;

  result.m_fElementsCM[12] = m_vPosition.x;
  result.m_fElementsCM[13] = m_vPosition.y;
  result.m_fElementsCM[14] = m_vPosition.z;

  return result;
}


template <typename Type>
PLASMA_ALWAYS_INLINE void plTransformTemplate<Type>::operator+=(const plVec3Template<Type>& v)
{
  m_vPosition += v;
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plTransformTemplate<Type>::operator-=(const plVec3Template<Type>& v)
{
  m_vPosition -= v;
}

template <typename Type>
PLASMA_ALWAYS_INLINE plVec3Template<Type> plTransformTemplate<Type>::TransformPosition(const plVec3Template<Type>& v) const
{
  const auto scaled = m_vScale.CompMul(v);
  const auto rotated = m_qRotation * scaled;
  return m_vPosition + rotated;
}

template <typename Type>
PLASMA_ALWAYS_INLINE plVec3Template<Type> plTransformTemplate<Type>::TransformDirection(const plVec3Template<Type>& v) const
{
  const auto scaled = m_vScale.CompMul(v);
  const auto rotated = m_qRotation * scaled;
  return rotated;
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plTransformTemplate<Type> operator*(const plQuatTemplate<Type>& q, const plTransformTemplate<Type>& t)
{
  plTransform r;

  r.m_vPosition = t.m_vPosition;
  r.m_qRotation = q * t.m_qRotation;
  r.m_vScale = t.m_vScale;

  return r;
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plTransformTemplate<Type> operator*(const plTransformTemplate<Type>& t, const plQuatTemplate<Type>& q)
{
  plTransform r;

  r.m_vPosition = t.m_vPosition;
  r.m_qRotation = t.m_qRotation * q;
  r.m_vScale = t.m_vScale;

  return r;
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plTransformTemplate<Type> operator+(const plTransformTemplate<Type>& t, const plVec3Template<Type>& v)
{
  return plTransformTemplate<Type>(t.m_vPosition + v, t.m_qRotation, t.m_vScale);
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plTransformTemplate<Type> operator-(const plTransformTemplate<Type>& t, const plVec3Template<Type>& v)
{
  return plTransformTemplate<Type>(t.m_vPosition - v, t.m_qRotation, t.m_vScale);
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plVec3Template<Type> operator*(const plTransformTemplate<Type>& t, const plVec3Template<Type>& v)
{
  return t.TransformPosition(v);
}

template <typename Type>
inline const plTransformTemplate<Type> operator*(const plTransformTemplate<Type>& t1, const plTransformTemplate<Type>& t2)
{
  plTransformTemplate<Type> t;

  t.m_vPosition = (t1.m_qRotation * t2.m_vPosition.CompMul(t1.m_vScale)) + t1.m_vPosition;
  t.m_qRotation = t1.m_qRotation * t2.m_qRotation;
  t.m_vScale = t1.m_vScale.CompMul(t2.m_vScale);

  return t;
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool operator==(const plTransformTemplate<Type>& t1, const plTransformTemplate<Type>& t2)
{
  return t1.IsIdentical(t2);
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool operator!=(const plTransformTemplate<Type>& t1, const plTransformTemplate<Type>& t2)
{
  return !t1.IsIdentical(t2);
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plTransformTemplate<Type>::Invert()
{
  (*this) = GetInverse();
}

template <typename Type>
inline const plTransformTemplate<Type> plTransformTemplate<Type>::GetInverse() const
{
  const auto invRot = m_qRotation.GetInverse();
  const auto invScale = plVec3Template<Type>(1).CompDiv(m_vScale);
  const auto invPos = invRot * (invScale.CompMul(-m_vPosition));

  return plTransformTemplate<Type>(invPos, invRot, invScale);
}

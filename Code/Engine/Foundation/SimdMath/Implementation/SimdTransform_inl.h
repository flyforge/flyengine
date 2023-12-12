#pragma once

PLASMA_ALWAYS_INLINE plSimdTransform::plSimdTransform() = default;

PLASMA_ALWAYS_INLINE plSimdTransform::plSimdTransform(const plSimdVec4f& vPosition, const plSimdQuat& qRotation, const plSimdVec4f& vScale)
  : m_Position(vPosition)
  , m_Rotation(qRotation)
  , m_Scale(vScale)
{
}

PLASMA_ALWAYS_INLINE plSimdTransform::plSimdTransform(const plSimdQuat& qRotation)
  : m_Rotation(qRotation)
{
  m_Position.SetZero();
  m_Scale.Set(1.0f);
}

PLASMA_ALWAYS_INLINE void plSimdTransform::SetIdentity()
{
  m_Position.SetZero();
  m_Rotation.SetIdentity();
  m_Scale.Set(1.0f);
}

// static
PLASMA_ALWAYS_INLINE plSimdTransform plSimdTransform::IdentityTransform()
{
  plSimdTransform result;
  result.SetIdentity();
  return result;
}

PLASMA_ALWAYS_INLINE plSimdFloat plSimdTransform::GetMaxScale() const
{
  return m_Scale.Abs().HorizontalMax<3>();
}

PLASMA_ALWAYS_INLINE bool plSimdTransform::ContainsNegativeScale() const
{
  return (m_Scale.x() * m_Scale.y() * m_Scale.z()) < plSimdFloat::Zero();
}

PLASMA_ALWAYS_INLINE bool plSimdTransform::ContainsUniformScale() const
{
  const plSimdFloat fEpsilon = plMath::DefaultEpsilon<float>();
  return m_Scale.x().IsEqual(m_Scale.y(), fEpsilon) && m_Scale.x().IsEqual(m_Scale.z(), fEpsilon);
}

PLASMA_ALWAYS_INLINE bool plSimdTransform::IsEqual(const plSimdTransform& rhs, const plSimdFloat& fEpsilon) const
{
  return m_Position.IsEqual(rhs.m_Position, fEpsilon).AllSet<3>() && m_Rotation.IsEqualRotation(rhs.m_Rotation, fEpsilon) &&
         m_Scale.IsEqual(rhs.m_Scale, fEpsilon).AllSet<3>();
}

PLASMA_ALWAYS_INLINE void plSimdTransform::Invert()
{
  (*this) = GetInverse();
}

PLASMA_ALWAYS_INLINE plSimdTransform plSimdTransform::GetInverse() const
{
  plSimdQuat invRot = -m_Rotation;
  plSimdVec4f invScale = m_Scale.GetReciprocal();
  plSimdVec4f invPos = invRot * (invScale.CompMul(-m_Position));

  return plSimdTransform(invPos, invRot, invScale);
}

inline void plSimdTransform::SetLocalTransform(const plSimdTransform& globalTransformParent, const plSimdTransform& globalTransformChild)
{
  plSimdQuat invRot = -globalTransformParent.m_Rotation;
  plSimdVec4f invScale = globalTransformParent.m_Scale.GetReciprocal();

  m_Position = (invRot * (globalTransformChild.m_Position - globalTransformParent.m_Position)).CompMul(invScale);
  m_Rotation = invRot * globalTransformChild.m_Rotation;
  m_Scale = invScale.CompMul(globalTransformChild.m_Scale);
}

PLASMA_ALWAYS_INLINE void plSimdTransform::SetGlobalTransform(const plSimdTransform& globalTransformParent, const plSimdTransform& localTransformChild)
{
  *this = globalTransformParent * localTransformChild;
}

PLASMA_FORCE_INLINE plSimdMat4f plSimdTransform::GetAsMat4() const
{
  plSimdMat4f result = m_Rotation.GetAsMat4();

  result.m_col0 *= m_Scale.x();
  result.m_col1 *= m_Scale.y();
  result.m_col2 *= m_Scale.z();
  result.m_col3 = m_Position;
  result.m_col3.SetW(1.0f);

  return result;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdTransform::TransformPosition(const plSimdVec4f& v) const
{
  const plSimdVec4f scaled = m_Scale.CompMul(v);
  const plSimdVec4f rotated = m_Rotation * scaled;
  return m_Position + rotated;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdTransform::TransformDirection(const plSimdVec4f& v) const
{
  const plSimdVec4f scaled = m_Scale.CompMul(v);
  return m_Rotation * scaled;
}

PLASMA_ALWAYS_INLINE const plSimdVec4f operator*(const plSimdTransform& t, const plSimdVec4f& v)
{
  return t.TransformPosition(v);
}

inline const plSimdTransform operator*(const plSimdTransform& lhs, const plSimdTransform& rhs)
{
  plSimdTransform t;

  t.m_Position = (lhs.m_Rotation * rhs.m_Position.CompMul(lhs.m_Scale)) + lhs.m_Position;
  t.m_Rotation = lhs.m_Rotation * rhs.m_Rotation;
  t.m_Scale = lhs.m_Scale.CompMul(rhs.m_Scale);

  return t;
}

PLASMA_ALWAYS_INLINE void plSimdTransform::operator*=(const plSimdTransform& other)
{
  (*this) = (*this) * other;
}

PLASMA_ALWAYS_INLINE const plSimdTransform operator*(const plSimdTransform& lhs, const plSimdQuat& q)
{
  plSimdTransform t;
  t.m_Position = lhs.m_Position;
  t.m_Rotation = lhs.m_Rotation * q;
  t.m_Scale = lhs.m_Scale;
  return t;
}

PLASMA_ALWAYS_INLINE const plSimdTransform operator*(const plSimdQuat& q, const plSimdTransform& rhs)
{
  plSimdTransform t;
  t.m_Position = rhs.m_Position;
  t.m_Rotation = q * rhs.m_Rotation;
  t.m_Scale = rhs.m_Scale;
  return t;
}

PLASMA_ALWAYS_INLINE void plSimdTransform::operator*=(const plSimdQuat& q)
{
  m_Rotation = m_Rotation * q;
}

PLASMA_ALWAYS_INLINE const plSimdTransform operator+(const plSimdTransform& lhs, const plSimdVec4f& v)
{
  plSimdTransform t;

  t.m_Position = lhs.m_Position + v;
  t.m_Rotation = lhs.m_Rotation;
  t.m_Scale = lhs.m_Scale;

  return t;
}

PLASMA_ALWAYS_INLINE const plSimdTransform operator-(const plSimdTransform& lhs, const plSimdVec4f& v)
{
  plSimdTransform t;

  t.m_Position = lhs.m_Position - v;
  t.m_Rotation = lhs.m_Rotation;
  t.m_Scale = lhs.m_Scale;

  return t;
}

PLASMA_ALWAYS_INLINE void plSimdTransform::operator+=(const plSimdVec4f& v)
{
  m_Position += v;
}

PLASMA_ALWAYS_INLINE void plSimdTransform::operator-=(const plSimdVec4f& v)
{
  m_Position -= v;
}

PLASMA_ALWAYS_INLINE bool operator==(const plSimdTransform& lhs, const plSimdTransform& rhs)
{
  return (lhs.m_Position == rhs.m_Position).AllSet<3>() && lhs.m_Rotation == rhs.m_Rotation && (lhs.m_Scale == rhs.m_Scale).AllSet<3>();
}

PLASMA_ALWAYS_INLINE bool operator!=(const plSimdTransform& lhs, const plSimdTransform& rhs)
{
  return !(lhs == rhs);
}

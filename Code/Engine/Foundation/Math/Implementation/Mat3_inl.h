#pragma once

template <typename Type>
plMat3Template<Type>::plMat3Template()
{
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = plMath::NaN<Type>();
  SetElements(TypeNaN, TypeNaN, TypeNaN, TypeNaN, TypeNaN, TypeNaN, TypeNaN, TypeNaN, TypeNaN);
#endif
}

template <typename Type>
plMat3Template<Type>::plMat3Template(const Type* const pData, plMatrixLayout::Enum layout)
{
  SetFromArray(pData, layout);
}

template <typename Type>
plMat3Template<Type>::plMat3Template(Type c1r1, Type c2r1, Type c3r1, Type c1r2, Type c2r2, Type c3r2, Type c1r3, Type c2r3, Type c3r3)
{
  SetElements(c1r1, c2r1, c3r1, c1r2, c2r2, c3r2, c1r3, c2r3, c3r3);
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plMat3Template<Type> plMat3Template<Type>::IdentityMatrix()
{
  return plMat3Template<Type>(1, 0, 0, 0, 1, 0, 0, 0, 1);
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plMat3Template<Type> plMat3Template<Type>::ZeroMatrix()
{
  return plMat3Template<Type>(0, 0, 0, 0, 0, 0, 0, 0, 0);
}

template <typename Type>
void plMat3Template<Type>::SetFromArray(const Type* const pData, plMatrixLayout::Enum layout)
{
  if (layout == plMatrixLayout::ColumnMajor)
  {
    plMemoryUtils::Copy(m_fElementsCM, pData, 9);
  }
  else
  {
    for (int i = 0; i < 3; ++i)
    {
      Element(0, i) = pData[i * 3 + 0];
      Element(1, i) = pData[i * 3 + 1];
      Element(2, i) = pData[i * 3 + 2];
    }
  }
}

template <typename Type>
void plMat3Template<Type>::GetAsArray(Type* out_pData, plMatrixLayout::Enum layout) const
{
  PLASMA_NAN_ASSERT(this);

  if (layout == plMatrixLayout::ColumnMajor)
  {
    plMemoryUtils::Copy(out_pData, m_fElementsCM, 9);
  }
  else
  {
    for (int i = 0; i < 3; ++i)
    {
      out_pData[i * 3 + 0] = Element(0, i);
      out_pData[i * 3 + 1] = Element(1, i);
      out_pData[i * 3 + 2] = Element(2, i);
    }
  }
}

template <typename Type>
void plMat3Template<Type>::SetElements(Type c1r1, Type c2r1, Type c3r1, Type c1r2, Type c2r2, Type c3r2, Type c1r3, Type c2r3, Type c3r3)
{
  Element(0, 0) = c1r1;
  Element(1, 0) = c2r1;
  Element(2, 0) = c3r1;
  Element(0, 1) = c1r2;
  Element(1, 1) = c2r2;
  Element(2, 1) = c3r2;
  Element(0, 2) = c1r3;
  Element(1, 2) = c2r3;
  Element(2, 2) = c3r3;
}

template <typename Type>
void plMat3Template<Type>::SetZero()
{
  SetElements(0, 0, 0, 0, 0, 0, 0, 0, 0);
}

template <typename Type>
void plMat3Template<Type>::SetIdentity()
{
  SetElements(1, 0, 0, 0, 1, 0, 0, 0, 1);
}

template <typename Type>
void plMat3Template<Type>::SetScalingMatrix(const plVec3Template<Type>& s)
{
  SetElements(s.x, 0, 0, 0, s.y, 0, 0, 0, s.z);
}

template <typename Type>
void plMat3Template<Type>::SetRotationMatrixX(plAngle angle)
{
  const Type fSin = plMath::Sin(angle);
  const Type fCos = plMath::Cos(angle);

  SetElements(1.0f, 0.0f, 0.0f, 0.0f, fCos, -fSin, 0.0f, fSin, fCos);
}

template <typename Type>
void plMat3Template<Type>::SetRotationMatrixY(plAngle angle)
{
  const Type fSin = plMath::Sin(angle);
  const Type fCos = plMath::Cos(angle);


  SetElements(fCos, 0.0f, fSin, 0.0f, 1.0f, 0.0f, -fSin, 0.0f, fCos);
}

template <typename Type>
void plMat3Template<Type>::SetRotationMatrixZ(plAngle angle)
{
  const Type fSin = plMath::Sin(angle);
  const Type fCos = plMath::Cos(angle);

  SetElements(fCos, -fSin, 0.0f, fSin, fCos, 0.0f, 0.0f, 0.0f, 1.0f);
}

template <typename Type>
void plMat3Template<Type>::Transpose()
{
  plMath::Swap(Element(0, 1), Element(1, 0));
  plMath::Swap(Element(0, 2), Element(2, 0));
  plMath::Swap(Element(1, 2), Element(2, 1));
}

template <typename Type>
const plMat3Template<Type> plMat3Template<Type>::GetTranspose() const
{
  return plMat3Template(m_fElementsCM, plMatrixLayout::RowMajor);
}

template <typename Type>
const plMat3Template<Type> plMat3Template<Type>::GetInverse(Type fEpsilon) const
{
  plMat3Template<Type> Inverse = *this;
  plResult res = Inverse.Invert(fEpsilon);
  PLASMA_ASSERT_DEBUG(res.Succeeded(), "Could not invert the given Mat3.");
  PLASMA_IGNORE_UNUSED(res);
  return Inverse;
}

template <typename Type>
plVec3Template<Type> plMat3Template<Type>::GetRow(plUInt32 uiRow) const
{
  PLASMA_ASSERT_DEBUG(uiRow <= 2, "Invalid Row Index {0}", uiRow);

  plVec3Template<Type> r;
  r.x = Element(0, uiRow);
  r.y = Element(1, uiRow);
  r.z = Element(2, uiRow);

  PLASMA_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
void plMat3Template<Type>::SetRow(plUInt32 uiRow, const plVec3Template<Type>& vRow)
{
  PLASMA_ASSERT_DEBUG(uiRow <= 2, "Invalid Row Index {0}", uiRow);

  Element(0, uiRow) = vRow.x;
  Element(1, uiRow) = vRow.y;
  Element(2, uiRow) = vRow.z;
}

template <typename Type>
plVec3Template<Type> plMat3Template<Type>::GetColumn(plUInt32 uiColumn) const
{
  PLASMA_ASSERT_DEBUG(uiColumn <= 2, "Invalid Column Index {0}", uiColumn);

  plVec3Template<Type> r;
  r.x = Element(uiColumn, 0);
  r.y = Element(uiColumn, 1);
  r.z = Element(uiColumn, 2);

  PLASMA_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
void plMat3Template<Type>::SetColumn(plUInt32 uiColumn, const plVec3Template<Type>& vColumn)
{
  PLASMA_ASSERT_DEBUG(uiColumn <= 2, "Invalid Column Index {0}", uiColumn);

  Element(uiColumn, 0) = vColumn.x;
  Element(uiColumn, 1) = vColumn.y;
  Element(uiColumn, 2) = vColumn.z;
}

template <typename Type>
plVec3Template<Type> plMat3Template<Type>::GetDiagonal() const
{
  PLASMA_NAN_ASSERT(this);

  return plVec3Template<Type>(Element(0, 0), Element(1, 1), Element(2, 2));
}

template <typename Type>
void plMat3Template<Type>::SetDiagonal(const plVec3Template<Type>& vDiag)
{
  Element(0, 0) = vDiag.x;
  Element(1, 1) = vDiag.y;
  Element(2, 2) = vDiag.z;
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> plMat3Template<Type>::TransformDirection(const plVec3Template<Type>& v) const
{
  plVec3Template<Type> r;
  r.x = Element(0, 0) * v.x + Element(1, 0) * v.y + Element(2, 0) * v.z;
  r.y = Element(0, 1) * v.x + Element(1, 1) * v.y + Element(2, 1) * v.z;
  r.z = Element(0, 2) * v.x + Element(1, 2) * v.y + Element(2, 2) * v.z;

  PLASMA_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
PLASMA_FORCE_INLINE void plMat3Template<Type>::operator*=(Type f)
{
  for (plInt32 i = 0; i < 9; ++i)
    m_fElementsCM[i] *= f;

  PLASMA_NAN_ASSERT(this);
}

template <typename Type>
PLASMA_FORCE_INLINE void plMat3Template<Type>::operator/=(Type f)
{
  const Type fInv = plMath::Invert(f);

  operator*=(fInv);
}

template <typename Type>
const plMat3Template<Type> operator*(const plMat3Template<Type>& m1, const plMat3Template<Type>& m2)
{
  plMat3Template<Type> r;
  for (plInt32 i = 0; i < 3; ++i)
  {
    r.Element(0, i) = m1.Element(0, i) * m2.Element(0, 0) + m1.Element(1, i) * m2.Element(0, 1) + m1.Element(2, i) * m2.Element(0, 2);
    r.Element(1, i) = m1.Element(0, i) * m2.Element(1, 0) + m1.Element(1, i) * m2.Element(1, 1) + m1.Element(2, i) * m2.Element(1, 2);
    r.Element(2, i) = m1.Element(0, i) * m2.Element(2, 0) + m1.Element(1, i) * m2.Element(2, 1) + m1.Element(2, i) * m2.Element(2, 2);
  }

  PLASMA_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plVec3Template<Type> operator*(const plMat3Template<Type>& m, const plVec3Template<Type>& v)
{
  return m.TransformDirection(v);
}



// *** free functions ***

template <typename Type>
PLASMA_ALWAYS_INLINE const plMat3Template<Type> operator*(Type f, const plMat3Template<Type>& m1)
{
  return operator*(m1, f);
}

template <typename Type>
const plMat3Template<Type> operator*(const plMat3Template<Type>& m1, Type f)
{
  plMat3Template<Type> r;

  for (plUInt32 i = 0; i < 9; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] * f;

  PLASMA_NAN_ASSERT(&r);

  return r;
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plMat3Template<Type> operator/(const plMat3Template<Type>& m1, Type f)
{
  return operator*(m1, plMath::Invert(f));
}

template <typename Type>
const plMat3Template<Type> operator+(const plMat3Template<Type>& m1, const plMat3Template<Type>& m2)
{
  plMat3Template<Type> r;

  for (plUInt32 i = 0; i < 9; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] + m2.m_fElementsCM[i];

  PLASMA_NAN_ASSERT(&r);

  return r;
}

template <typename Type>
const plMat3Template<Type> operator-(const plMat3Template<Type>& m1, const plMat3Template<Type>& m2)
{
  plMat3Template<Type> r;

  for (plUInt32 i = 0; i < 9; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] - m2.m_fElementsCM[i];

  PLASMA_NAN_ASSERT(&r);

  return r;
}

template <typename Type>
bool plMat3Template<Type>::IsIdentical(const plMat3Template<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  for (plUInt32 i = 0; i < 9; ++i)
  {
    if (m_fElementsCM[i] != rhs.m_fElementsCM[i])
      return false;
  }

  return true;
}

template <typename Type>
bool plMat3Template<Type>::IsEqual(const plMat3Template<Type>& rhs, Type fEpsilon) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  PLASMA_ASSERT_DEBUG(fEpsilon >= 0.0f, "Epsilon may not be negative.");

  for (plUInt32 i = 0; i < 9; ++i)
  {
    if (!plMath::IsEqual(m_fElementsCM[i], rhs.m_fElementsCM[i], fEpsilon))
      return false;
  }

  return true;
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool operator==(const plMat3Template<Type>& lhs, const plMat3Template<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool operator!=(const plMat3Template<Type>& lhs, const plMat3Template<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template <typename Type>
bool plMat3Template<Type>::IsZero(Type fEpsilon) const
{
  PLASMA_NAN_ASSERT(this);

  for (plUInt32 i = 0; i < 9; ++i)
  {
    if (!plMath::IsZero(m_fElementsCM[i], fEpsilon))
      return false;
  }

  return true;
}

template <typename Type>
bool plMat3Template<Type>::IsIdentity(Type fEpsilon) const
{
  PLASMA_NAN_ASSERT(this);

  if (!plMath::IsEqual(Element(0, 0), (Type)1, fEpsilon))
    return false;
  if (!plMath::IsEqual(Element(0, 1), (Type)0, fEpsilon))
    return false;
  if (!plMath::IsEqual(Element(0, 2), (Type)0, fEpsilon))
    return false;

  if (!plMath::IsEqual(Element(1, 0), (Type)0, fEpsilon))
    return false;
  if (!plMath::IsEqual(Element(1, 1), (Type)1, fEpsilon))
    return false;
  if (!plMath::IsEqual(Element(1, 2), (Type)0, fEpsilon))
    return false;

  if (!plMath::IsEqual(Element(2, 0), (Type)0, fEpsilon))
    return false;
  if (!plMath::IsEqual(Element(2, 1), (Type)0, fEpsilon))
    return false;
  if (!plMath::IsEqual(Element(2, 2), (Type)1, fEpsilon))
    return false;

  return true;
}

template <typename Type>
bool plMat3Template<Type>::IsValid() const
{
  for (plUInt32 i = 0; i < 9; ++i)
  {
    if (!plMath::IsFinite(m_fElementsCM[i]))
      return false;
  }

  return true;
}

template <typename Type>
bool plMat3Template<Type>::IsNaN() const
{
  for (plUInt32 i = 0; i < 9; ++i)
  {
    if (plMath::IsNaN(m_fElementsCM[i]))
      return true;
  }

  return false;
}

template <typename Type>
const plVec3Template<Type> plMat3Template<Type>::GetScalingFactors() const
{
  plVec3Template<Type> v;

  v.x = plVec3Template<Type>(Element(0, 0), Element(0, 1), Element(0, 2)).GetLength();
  v.y = plVec3Template<Type>(Element(1, 0), Element(1, 1), Element(1, 2)).GetLength();
  v.z = plVec3Template<Type>(Element(2, 0), Element(2, 1), Element(2, 2)).GetLength();

  PLASMA_NAN_ASSERT(&v);
  return v;
}

template <typename Type>
plResult plMat3Template<Type>::SetScalingFactors(const plVec3Template<Type>& vXYZ, Type fEpsilon /* = plMath::DefaultEpsilon<Type>() */)
{
  plVec3Template<Type> tx(Element(0, 0), Element(0, 1), Element(0, 2));
  plVec3Template<Type> ty(Element(1, 0), Element(1, 1), Element(1, 2));
  plVec3Template<Type> tz(Element(2, 0), Element(2, 1), Element(2, 2));

  if (tx.SetLength(vXYZ.x, fEpsilon) == PLASMA_FAILURE)
    return PLASMA_FAILURE;
  if (ty.SetLength(vXYZ.y, fEpsilon) == PLASMA_FAILURE)
    return PLASMA_FAILURE;
  if (tz.SetLength(vXYZ.z, fEpsilon) == PLASMA_FAILURE)
    return PLASMA_FAILURE;


  Element(0, 0) = tx.x;
  Element(0, 1) = tx.y;
  Element(0, 2) = tx.z;
  Element(1, 0) = ty.x;
  Element(1, 1) = ty.y;
  Element(1, 2) = ty.z;
  Element(2, 0) = tz.x;
  Element(2, 1) = tz.y;
  Element(2, 2) = tz.z;

  return PLASMA_SUCCESS;
}

template <typename Type>
Type plMat3Template<Type>::GetDeterminant() const
{
  // Using rule of Sarrus
  Type fDeterminant = 0;
  for (int i = 0; i < 3; i++)
  {
    fDeterminant += Element(i, 0) * Element((i + 1) % 3, 1) * Element((i + 2) % 3, 2);
    fDeterminant -= Element(i, 2) * Element((i + 1) % 3, 1) * Element((i + 2) % 3, 0);
  }
  return fDeterminant;
}

#include <Foundation/Math/Implementation/AllClasses_inl.h>

#pragma once

PLASMA_ALWAYS_INLINE plSimdMat4f::plSimdMat4f() = default;

PLASMA_ALWAYS_INLINE plSimdMat4f::plSimdMat4f(const float* const pData, plMatrixLayout::Enum layout)
{
  SetFromArray(pData, layout);
}

PLASMA_ALWAYS_INLINE plSimdMat4f::plSimdMat4f(const plSimdVec4f& vCol0, const plSimdVec4f& vCol1, const plSimdVec4f& vCol2, const plSimdVec4f& vCol3)
  : m_col0(vCol0)
  , m_col1(vCol1)
  , m_col2(vCol2)
  , m_col3(vCol3)
{
}

PLASMA_ALWAYS_INLINE plSimdMat4f::plSimdMat4f(float f1r1, float f2r1, float f3r1, float f4r1, float f1r2, float f2r2, float f3r2, float f4r2, float f1r3,
  float f2r3, float f3r3, float f4r3, float f1r4, float f2r4, float f3r4, float f4r4)
{
  m_col0.Set(f1r1, f1r2, f1r3, f1r4);
  m_col1.Set(f2r1, f2r2, f2r3, f2r4);
  m_col2.Set(f3r1, f3r2, f3r3, f3r4);
  m_col3.Set(f4r1, f4r2, f4r3, f4r4);
}

inline void plSimdMat4f::SetFromArray(const float* const pData, plMatrixLayout::Enum layout)
{
  m_col0.Load<4>(pData + 0);
  m_col1.Load<4>(pData + 4);
  m_col2.Load<4>(pData + 8);
  m_col3.Load<4>(pData + 12);

  if (layout == plMatrixLayout::RowMajor)
  {
    Transpose();
  }
}

inline void plSimdMat4f::GetAsArray(float* out_pData, plMatrixLayout::Enum layout) const
{
  plSimdMat4f tmp = *this;

  if (layout == plMatrixLayout::RowMajor)
  {
    tmp.Transpose();
  }

  tmp.m_col0.Store<4>(out_pData + 0);
  tmp.m_col1.Store<4>(out_pData + 4);
  tmp.m_col2.Store<4>(out_pData + 8);
  tmp.m_col3.Store<4>(out_pData + 12);
}

PLASMA_ALWAYS_INLINE void plSimdMat4f::SetIdentity()
{
  m_col0.Set(1, 0, 0, 0);
  m_col1.Set(0, 1, 0, 0);
  m_col2.Set(0, 0, 1, 0);
  m_col3.Set(0, 0, 0, 1);
}

// static
PLASMA_ALWAYS_INLINE plSimdMat4f plSimdMat4f::IdentityMatrix()
{
  plSimdMat4f result;
  result.SetIdentity();
  return result;
}

PLASMA_ALWAYS_INLINE plSimdMat4f plSimdMat4f::GetTranspose() const
{
  plSimdMat4f result = *this;
  result.Transpose();
  return result;
}

PLASMA_ALWAYS_INLINE plSimdMat4f plSimdMat4f::GetInverse(const plSimdFloat& fEpsilon) const
{
  plSimdMat4f result = *this;
  result.Invert(fEpsilon).IgnoreResult();
  return result;
}

inline bool plSimdMat4f::IsEqual(const plSimdMat4f& rhs, const plSimdFloat& fEpsilon) const
{
  return (m_col0.IsEqual(rhs.m_col0, fEpsilon) && m_col1.IsEqual(rhs.m_col1, fEpsilon) && m_col2.IsEqual(rhs.m_col2, fEpsilon) &&
          m_col3.IsEqual(rhs.m_col3, fEpsilon))
    .AllSet<4>();
}

inline bool plSimdMat4f::IsIdentity(const plSimdFloat& fEpsilon) const
{
  return (m_col0.IsEqual(plSimdVec4f(1, 0, 0, 0), fEpsilon) && m_col1.IsEqual(plSimdVec4f(0, 1, 0, 0), fEpsilon) &&
          m_col2.IsEqual(plSimdVec4f(0, 0, 1, 0), fEpsilon) && m_col3.IsEqual(plSimdVec4f(0, 0, 0, 1), fEpsilon))
    .AllSet<4>();
}

inline bool plSimdMat4f::IsValid() const
{
  return m_col0.IsValid<4>() && m_col1.IsValid<4>() && m_col2.IsValid<4>() && m_col3.IsValid<4>();
}

inline bool plSimdMat4f::IsNaN() const
{
  return m_col0.IsNaN<4>() || m_col1.IsNaN<4>() || m_col2.IsNaN<4>() || m_col3.IsNaN<4>();
}

PLASMA_ALWAYS_INLINE void plSimdMat4f::SetRows(const plSimdVec4f& vRow0, const plSimdVec4f& vRow1, const plSimdVec4f& vRow2, const plSimdVec4f& vRow3)
{
  m_col0 = vRow0;
  m_col1 = vRow1;
  m_col2 = vRow2;
  m_col3 = vRow3;

  Transpose();
}

PLASMA_ALWAYS_INLINE void plSimdMat4f::GetRows(plSimdVec4f& ref_vRow0, plSimdVec4f& ref_vRow1, plSimdVec4f& ref_vRow2, plSimdVec4f& ref_vRow3) const
{
  plSimdMat4f tmp = *this;
  tmp.Transpose();

  ref_vRow0 = tmp.m_col0;
  ref_vRow1 = tmp.m_col1;
  ref_vRow2 = tmp.m_col2;
  ref_vRow3 = tmp.m_col3;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdMat4f::TransformPosition(const plSimdVec4f& v) const
{
  plSimdVec4f result;
  result = m_col0 * v.x();
  result += m_col1 * v.y();
  result += m_col2 * v.z();
  result += m_col3;

  return result;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdMat4f::TransformDirection(const plSimdVec4f& v) const
{
  plSimdVec4f result;
  result = m_col0 * v.x();
  result += m_col1 * v.y();
  result += m_col2 * v.z();

  return result;
}

PLASMA_ALWAYS_INLINE plSimdMat4f plSimdMat4f::operator*(const plSimdMat4f& rhs) const
{
  plSimdMat4f result;

  result.m_col0 = m_col0 * rhs.m_col0.x();
  result.m_col0 += m_col1 * rhs.m_col0.y();
  result.m_col0 += m_col2 * rhs.m_col0.z();
  result.m_col0 += m_col3 * rhs.m_col0.w();

  result.m_col1 = m_col0 * rhs.m_col1.x();
  result.m_col1 += m_col1 * rhs.m_col1.y();
  result.m_col1 += m_col2 * rhs.m_col1.z();
  result.m_col1 += m_col3 * rhs.m_col1.w();

  result.m_col2 = m_col0 * rhs.m_col2.x();
  result.m_col2 += m_col1 * rhs.m_col2.y();
  result.m_col2 += m_col2 * rhs.m_col2.z();
  result.m_col2 += m_col3 * rhs.m_col2.w();

  result.m_col3 = m_col0 * rhs.m_col3.x();
  result.m_col3 += m_col1 * rhs.m_col3.y();
  result.m_col3 += m_col2 * rhs.m_col3.z();
  result.m_col3 += m_col3 * rhs.m_col3.w();

  return result;
}

PLASMA_ALWAYS_INLINE void plSimdMat4f::operator*=(const plSimdMat4f& rhs)
{
  *this = *this * rhs;
}

PLASMA_ALWAYS_INLINE bool plSimdMat4f::operator==(const plSimdMat4f& other) const
{
  return (m_col0 == other.m_col0 && m_col1 == other.m_col1 && m_col2 == other.m_col2 && m_col3 == other.m_col3).AllSet<4>();
}

PLASMA_ALWAYS_INLINE bool plSimdMat4f::operator!=(const plSimdMat4f& other) const
{
  return !(*this == other);
}

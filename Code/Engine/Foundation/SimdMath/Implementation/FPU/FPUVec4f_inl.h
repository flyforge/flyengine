#pragma once

PLASMA_ALWAYS_INLINE plSimdVec4f::plSimdVec4f() {}

PLASMA_ALWAYS_INLINE plSimdVec4f::plSimdVec4f(float xyzw)
{
  m_v.Set(xyzw);
}

PLASMA_ALWAYS_INLINE plSimdVec4f::plSimdVec4f(const plSimdFloat& xyzw)
{
  m_v = xyzw.m_v;
}

PLASMA_ALWAYS_INLINE plSimdVec4f::plSimdVec4f(float x, float y, float z, float w)
{
  m_v.Set(x, y, z, w);
}

PLASMA_ALWAYS_INLINE void plSimdVec4f::Set(float xyzw)
{
  m_v.Set(xyzw);
}

PLASMA_ALWAYS_INLINE void plSimdVec4f::Set(float x, float y, float z, float w)
{
  m_v.Set(x, y, z, w);
}

PLASMA_ALWAYS_INLINE void plSimdVec4f::SetX(const plSimdFloat& f)
{
  m_v.x = f.m_v.x;
}

PLASMA_ALWAYS_INLINE void plSimdVec4f::SetY(const plSimdFloat& f)
{
  m_v.y = f.m_v.x;
}

PLASMA_ALWAYS_INLINE void plSimdVec4f::SetZ(const plSimdFloat& f)
{
  m_v.z = f.m_v.x;
}

PLASMA_ALWAYS_INLINE void plSimdVec4f::SetW(const plSimdFloat& f)
{
  m_v.w = f.m_v.x;
}

PLASMA_ALWAYS_INLINE void plSimdVec4f::SetZero()
{
  m_v.SetZero();
}

template <int N>
PLASMA_ALWAYS_INLINE void plSimdVec4f::Load(const float* pFloats)
{
  m_v.SetZero();
  for (int i = 0; i < N; ++i)
  {
    (&m_v.x)[i] = pFloats[i];
  }
}

template <int N>
PLASMA_ALWAYS_INLINE void plSimdVec4f::Store(float* pFloats) const
{
  for (int i = 0; i < N; ++i)
  {
    pFloats[i] = (&m_v.x)[i];
  }
}

template <plMathAcc::Enum acc>
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetReciprocal() const
{
  return plVec4(1.0f).CompDiv(m_v);
}

template <plMathAcc::Enum acc>
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetSqrt() const
{
  plSimdVec4f result;
  result.m_v.x = plMath::Sqrt(m_v.x);
  result.m_v.y = plMath::Sqrt(m_v.y);
  result.m_v.z = plMath::Sqrt(m_v.z);
  result.m_v.w = plMath::Sqrt(m_v.w);

  return result;
}

template <plMathAcc::Enum acc>
plSimdVec4f plSimdVec4f::GetInvSqrt() const
{
  plSimdVec4f result;
  result.m_v.x = 1.0f / plMath::Sqrt(m_v.x);
  result.m_v.y = 1.0f / plMath::Sqrt(m_v.y);
  result.m_v.z = 1.0f / plMath::Sqrt(m_v.z);
  result.m_v.w = 1.0f / plMath::Sqrt(m_v.w);

  return result;
}

template <int N, plMathAcc::Enum acc>
void plSimdVec4f::NormalizeIfNotZero(const plSimdFloat& fEpsilon)
{
  plSimdFloat sqLength = GetLengthSquared<N>();
  m_v *= sqLength.GetInvSqrt<acc>();
  m_v = sqLength > fEpsilon.m_v ? m_v : plVec4::ZeroVector();
}

template <int N>
PLASMA_ALWAYS_INLINE bool plSimdVec4f::IsZero() const
{
  for (int i = 0; i < N; ++i)
  {
    if ((&m_v.x)[i] != 0.0f)
      return false;
  }

  return true;
}

template <int N>
PLASMA_ALWAYS_INLINE bool plSimdVec4f::IsZero(const plSimdFloat& fEpsilon) const
{
  for (int i = 0; i < N; ++i)
  {
    if (!plMath::IsZero((&m_v.x)[i], (float)fEpsilon))
      return false;
  }

  return true;
}

template <int N>
PLASMA_ALWAYS_INLINE bool plSimdVec4f::IsNaN() const
{
  for (int i = 0; i < N; ++i)
  {
    if (plMath::IsNaN((&m_v.x)[i]))
      return true;
  }

  return false;
}

template <int N>
PLASMA_ALWAYS_INLINE bool plSimdVec4f::IsValid() const
{
  for (int i = 0; i < N; ++i)
  {
    if (!plMath::IsFinite((&m_v.x)[i]))
      return false;
  }

  return true;
}

template <int N>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::GetComponent() const
{
  return (&m_v.x)[N];
}

PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::x() const
{
  return m_v.x;
}

PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::y() const
{
  return m_v.y;
}

PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::z() const
{
  return m_v.z;
}

PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::w() const
{
  return m_v.w;
}

template <plSwizzle::Enum s>
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Get() const
{
  plSimdVec4f result;

  const float* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

template <plSwizzle::Enum s>
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetCombined(const plSimdVec4f& other) const
{
  plSimdVec4f result;

  const float* v = &m_v.x;
  const float* o = &other.m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = o[(s & 0x0030) >> 4];
  result.m_v.w = o[(s & 0x0003)];

  return result;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator-() const
{
  return -m_v;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator+(const plSimdVec4f& v) const
{
  return m_v + v.m_v;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator-(const plSimdVec4f& v) const
{
  return m_v - v.m_v;
}


PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator*(const plSimdFloat& f) const
{
  return m_v * f.m_v.x;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator/(const plSimdFloat& f) const
{
  return m_v / f.m_v.x;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompMul(const plSimdVec4f& v) const
{
  return m_v.CompMul(v.m_v);
}

template <plMathAcc::Enum acc>
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompDiv(const plSimdVec4f& v) const
{
  return m_v.CompDiv(v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompMin(const plSimdVec4f& v) const
{
  return m_v.CompMin(v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompMax(const plSimdVec4f& v) const
{
  return m_v.CompMax(v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Abs() const
{
  return m_v.Abs();
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Round() const
{
  plSimdVec4f result;
  result.m_v.x = plMath::Round(m_v.x);
  result.m_v.y = plMath::Round(m_v.y);
  result.m_v.z = plMath::Round(m_v.z);
  result.m_v.w = plMath::Round(m_v.w);

  return result;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Floor() const
{
  plSimdVec4f result;
  result.m_v.x = plMath::Floor(m_v.x);
  result.m_v.y = plMath::Floor(m_v.y);
  result.m_v.z = plMath::Floor(m_v.z);
  result.m_v.w = plMath::Floor(m_v.w);

  return result;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Ceil() const
{
  plSimdVec4f result;
  result.m_v.x = plMath::Ceil(m_v.x);
  result.m_v.y = plMath::Ceil(m_v.y);
  result.m_v.z = plMath::Ceil(m_v.z);
  result.m_v.w = plMath::Ceil(m_v.w);

  return result;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Trunc() const
{
  plSimdVec4f result;
  result.m_v.x = plMath::Trunc(m_v.x);
  result.m_v.y = plMath::Trunc(m_v.y);
  result.m_v.z = plMath::Trunc(m_v.z);
  result.m_v.w = plMath::Trunc(m_v.w);

  return result;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::FlipSign(const plSimdVec4b& cmp) const
{
  plSimdVec4f result;
  result.m_v.x = cmp.m_v.x ? -m_v.x : m_v.x;
  result.m_v.y = cmp.m_v.y ? -m_v.y : m_v.y;
  result.m_v.z = cmp.m_v.z ? -m_v.z : m_v.z;
  result.m_v.w = cmp.m_v.w ? -m_v.w : m_v.w;

  return result;
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Select(const plSimdVec4b& cmp, const plSimdVec4f& ifTrue, const plSimdVec4f& ifFalse)
{
  plSimdVec4f result;
  result.m_v.x = cmp.m_v.x ? ifTrue.m_v.x : ifFalse.m_v.x;
  result.m_v.y = cmp.m_v.y ? ifTrue.m_v.y : ifFalse.m_v.y;
  result.m_v.z = cmp.m_v.z ? ifTrue.m_v.z : ifFalse.m_v.z;
  result.m_v.w = cmp.m_v.w ? ifTrue.m_v.w : ifFalse.m_v.w;

  return result;
}

PLASMA_ALWAYS_INLINE plSimdVec4f& plSimdVec4f::operator+=(const plSimdVec4f& v)
{
  m_v += v.m_v;
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4f& plSimdVec4f::operator-=(const plSimdVec4f& v)
{
  m_v -= v.m_v;
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4f& plSimdVec4f::operator*=(const plSimdFloat& f)
{
  m_v *= f.m_v.x;
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4f& plSimdVec4f::operator/=(const plSimdFloat& f)
{
  m_v /= f.m_v.x;
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator==(const plSimdVec4f& v) const
{
  bool result[4];
  result[0] = m_v.x == v.m_v.x;
  result[1] = m_v.y == v.m_v.y;
  result[2] = m_v.z == v.m_v.z;
  result[3] = m_v.w == v.m_v.w;

  return plSimdVec4b(result[0], result[1], result[2], result[3]);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator!=(const plSimdVec4f& v) const
{
  return !(*this == v);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator<=(const plSimdVec4f& v) const
{
  return !(*this > v);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator<(const plSimdVec4f& v) const
{
  bool result[4];
  result[0] = m_v.x < v.m_v.x;
  result[1] = m_v.y < v.m_v.y;
  result[2] = m_v.z < v.m_v.z;
  result[3] = m_v.w < v.m_v.w;

  return plSimdVec4b(result[0], result[1], result[2], result[3]);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator>=(const plSimdVec4f& v) const
{
  return !(*this < v);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator>(const plSimdVec4f& v) const
{
  bool result[4];
  result[0] = m_v.x > v.m_v.x;
  result[1] = m_v.y > v.m_v.y;
  result[2] = m_v.z > v.m_v.z;
  result[3] = m_v.w > v.m_v.w;

  return plSimdVec4b(result[0], result[1], result[2], result[3]);
}

template <>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalSum<2>() const
{
  return m_v.x + m_v.y;
}

template <>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalSum<3>() const
{
  return (float)HorizontalSum<2>() + m_v.z;
}

template <>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalSum<4>() const
{
  return (float)HorizontalSum<3>() + m_v.w;
}

template <>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMin<2>() const
{
  return plMath::Min(m_v.x, m_v.y);
}

template <>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMin<3>() const
{
  return plMath::Min((float)HorizontalMin<2>(), m_v.z);
}

template <>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMin<4>() const
{
  return plMath::Min((float)HorizontalMin<3>(), m_v.w);
}

template <>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMax<2>() const
{
  return plMath::Max(m_v.x, m_v.y);
}

template <>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMax<3>() const
{
  return plMath::Max((float)HorizontalMax<2>(), m_v.z);
}

template <>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMax<4>() const
{
  return plMath::Max((float)HorizontalMax<3>(), m_v.w);
}

template <int N>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::Dot(const plSimdVec4f& v) const
{
  float result = 0.0f;

  for (int i = 0; i < N; ++i)
  {
    result += (&m_v.x)[i] * (&v.m_v.x)[i];
  }

  return result;
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CrossRH(const plSimdVec4f& v) const
{
  return m_v.GetAsVec3().CrossRH(v.m_v.GetAsVec3()).GetAsVec4(0.0f);
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetOrthogonalVector() const
{
  if (plMath::Abs(m_v.y) < 0.99f)
  {
    return plVec4(-m_v.z, 0.0f, m_v.x, 0.0f);
  }
  else
  {
    return plVec4(0.0f, m_v.z, -m_v.y, 0.0f);
  }
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::ZeroVector()
{
  return plVec4::ZeroVector();
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::MulAdd(const plSimdVec4f& a, const plSimdVec4f& b, const plSimdVec4f& c)
{
  return a.CompMul(b) + c;
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::MulAdd(const plSimdVec4f& a, const plSimdFloat& b, const plSimdVec4f& c)
{
  return a * b + c;
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::MulSub(const plSimdVec4f& a, const plSimdVec4f& b, const plSimdVec4f& c)
{
  return a.CompMul(b) - c;
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::MulSub(const plSimdVec4f& a, const plSimdFloat& b, const plSimdVec4f& c)
{
  return a * b - c;
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CopySign(const plSimdVec4f& magnitude, const plSimdVec4f& sign)
{
  plSimdVec4f result;
  result.m_v.x = sign.m_v.x < 0.0f ? -magnitude.m_v.x : magnitude.m_v.x;
  result.m_v.y = sign.m_v.y < 0.0f ? -magnitude.m_v.y : magnitude.m_v.y;
  result.m_v.z = sign.m_v.z < 0.0f ? -magnitude.m_v.z : magnitude.m_v.z;
  result.m_v.w = sign.m_v.w < 0.0f ? -magnitude.m_v.w : magnitude.m_v.w;

  return result;
}

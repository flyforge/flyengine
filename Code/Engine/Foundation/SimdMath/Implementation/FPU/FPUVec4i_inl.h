#pragma once

PL_ALWAYS_INLINE plSimdVec4i::plSimdVec4i()
{
#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
  m_v.Set(0xCDCDCDCD);
#endif
}

PL_ALWAYS_INLINE plSimdVec4i::plSimdVec4i(plInt32 xyzw)
{
  m_v.Set(xyzw);
}

PL_ALWAYS_INLINE plSimdVec4i::plSimdVec4i(plInt32 x, plInt32 y, plInt32 z, plInt32 w)
{
  m_v.Set(x, y, z, w);
}

PL_ALWAYS_INLINE plSimdVec4i::plSimdVec4i(plInternal::QuadInt v)
{
  m_v = v;
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::MakeZero()
{
  return plSimdVec4i(0);
}

PL_ALWAYS_INLINE void plSimdVec4i::Set(plInt32 xyzw)
{
  m_v.Set(xyzw);
}

PL_ALWAYS_INLINE void plSimdVec4i::Set(plInt32 x, plInt32 y, plInt32 z, plInt32 w)
{
  m_v.Set(x, y, z, w);
}

PL_ALWAYS_INLINE void plSimdVec4i::SetZero()
{
  m_v.SetZero();
}

template <int N>
PL_ALWAYS_INLINE void plSimdVec4i::Load(const plInt32* pInts)
{
  m_v.SetZero();
  for (int i = 0; i < N; ++i)
  {
    (&m_v.x)[i] = pInts[i];
  }
}

template <int N>
PL_ALWAYS_INLINE void plSimdVec4i::Store(plInt32* pInts) const
{
  for (int i = 0; i < N; ++i)
  {
    pInts[i] = (&m_v.x)[i];
  }
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4i::ToFloat() const
{
  plSimdVec4f result;
  result.m_v.x = (float)m_v.x;
  result.m_v.y = (float)m_v.y;
  result.m_v.z = (float)m_v.z;
  result.m_v.w = (float)m_v.w;

  return result;
}

// static
PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::Truncate(const plSimdVec4f& f)
{
  plSimdVec4i result;
  result.m_v.x = (plInt32)f.m_v.x;
  result.m_v.y = (plInt32)f.m_v.y;
  result.m_v.z = (plInt32)f.m_v.z;
  result.m_v.w = (plInt32)f.m_v.w;

  return result;
}

template <int N>
PL_ALWAYS_INLINE plInt32 plSimdVec4i::GetComponent() const
{
  return (&m_v.x)[N];
}

PL_ALWAYS_INLINE plInt32 plSimdVec4i::x() const
{
  return m_v.x;
}

PL_ALWAYS_INLINE plInt32 plSimdVec4i::y() const
{
  return m_v.y;
}

PL_ALWAYS_INLINE plInt32 plSimdVec4i::z() const
{
  return m_v.z;
}

PL_ALWAYS_INLINE plInt32 plSimdVec4i::w() const
{
  return m_v.w;
}

template <plSwizzle::Enum s>
PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::Get() const
{
  plSimdVec4i result;

  const plInt32* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator-() const
{
  return -m_v;
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator+(const plSimdVec4i& v) const
{
  return m_v + v.m_v;
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator-(const plSimdVec4i& v) const
{
  return m_v - v.m_v;
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::CompMul(const plSimdVec4i& v) const
{
  return m_v.CompMul(v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::CompDiv(const plSimdVec4i& v) const
{
  return m_v.CompDiv(v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator|(const plSimdVec4i& v) const
{
  plSimdVec4i result;
  result.m_v.x = m_v.x | v.m_v.x;
  result.m_v.y = m_v.y | v.m_v.y;
  result.m_v.z = m_v.z | v.m_v.z;
  result.m_v.w = m_v.w | v.m_v.w;

  return result;
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator&(const plSimdVec4i& v) const
{
  plSimdVec4i result;
  result.m_v.x = m_v.x & v.m_v.x;
  result.m_v.y = m_v.y & v.m_v.y;
  result.m_v.z = m_v.z & v.m_v.z;
  result.m_v.w = m_v.w & v.m_v.w;

  return result;
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator^(const plSimdVec4i& v) const
{
  plSimdVec4i result;
  result.m_v.x = m_v.x ^ v.m_v.x;
  result.m_v.y = m_v.y ^ v.m_v.y;
  result.m_v.z = m_v.z ^ v.m_v.z;
  result.m_v.w = m_v.w ^ v.m_v.w;

  return result;
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator~() const
{
  plSimdVec4i result;
  result.m_v.x = ~m_v.x;
  result.m_v.y = ~m_v.y;
  result.m_v.z = ~m_v.z;
  result.m_v.w = ~m_v.w;

  return result;
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator<<(plUInt32 uiShift) const
{
  plSimdVec4i result;
  result.m_v.x = m_v.x << uiShift;
  result.m_v.y = m_v.y << uiShift;
  result.m_v.z = m_v.z << uiShift;
  result.m_v.w = m_v.w << uiShift;

  return result;
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator>>(plUInt32 uiShift) const
{
  plSimdVec4i result;
  result.m_v.x = m_v.x >> uiShift;
  result.m_v.y = m_v.y >> uiShift;
  result.m_v.z = m_v.z >> uiShift;
  result.m_v.w = m_v.w >> uiShift;

  return result;
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator<<(const plSimdVec4i& v) const
{
  plSimdVec4i result;
  result.m_v.x = m_v.x << v.m_v.x;
  result.m_v.y = m_v.y << v.m_v.y;
  result.m_v.z = m_v.z << v.m_v.z;
  result.m_v.w = m_v.w << v.m_v.w;

  return result;
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator>>(const plSimdVec4i& v) const
{
  plSimdVec4i result;
  result.m_v.x = m_v.x >> v.m_v.x;
  result.m_v.y = m_v.y >> v.m_v.y;
  result.m_v.z = m_v.z >> v.m_v.z;
  result.m_v.w = m_v.w >> v.m_v.w;

  return result;
}

PL_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator+=(const plSimdVec4i& v)
{
  m_v += v.m_v;
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator-=(const plSimdVec4i& v)
{
  m_v -= v.m_v;
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator|=(const plSimdVec4i& v)
{
  m_v.x |= v.m_v.x;
  m_v.y |= v.m_v.y;
  m_v.z |= v.m_v.z;
  m_v.w |= v.m_v.w;
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator&=(const plSimdVec4i& v)
{
  m_v.x &= v.m_v.x;
  m_v.y &= v.m_v.y;
  m_v.z &= v.m_v.z;
  m_v.w &= v.m_v.w;
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator^=(const plSimdVec4i& v)
{
  m_v.x ^= v.m_v.x;
  m_v.y ^= v.m_v.y;
  m_v.z ^= v.m_v.z;
  m_v.w ^= v.m_v.w;
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator<<=(plUInt32 uiShift)
{
  m_v.x <<= uiShift;
  m_v.y <<= uiShift;
  m_v.z <<= uiShift;
  m_v.w <<= uiShift;
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator>>=(plUInt32 uiShift)
{
  m_v.x >>= uiShift;
  m_v.y >>= uiShift;
  m_v.z >>= uiShift;
  m_v.w >>= uiShift;
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::CompMin(const plSimdVec4i& v) const
{
  return m_v.CompMin(v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::CompMax(const plSimdVec4i& v) const
{
  return m_v.CompMax(v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::Abs() const
{
  plSimdVec4i result;
  result.m_v.x = plMath::Abs(m_v.x);
  result.m_v.y = plMath::Abs(m_v.y);
  result.m_v.z = plMath::Abs(m_v.z);
  result.m_v.w = plMath::Abs(m_v.w);

  return result;
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator==(const plSimdVec4i& v) const
{
  bool result[4];
  result[0] = m_v.x == v.m_v.x;
  result[1] = m_v.y == v.m_v.y;
  result[2] = m_v.z == v.m_v.z;
  result[3] = m_v.w == v.m_v.w;

  return plSimdVec4b(result[0], result[1], result[2], result[3]);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator!=(const plSimdVec4i& v) const
{
  return !(*this == v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator<=(const plSimdVec4i& v) const
{
  return !(*this > v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator<(const plSimdVec4i& v) const
{
  bool result[4];
  result[0] = m_v.x < v.m_v.x;
  result[1] = m_v.y < v.m_v.y;
  result[2] = m_v.z < v.m_v.z;
  result[3] = m_v.w < v.m_v.w;

  return plSimdVec4b(result[0], result[1], result[2], result[3]);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator>=(const plSimdVec4i& v) const
{
  return !(*this < v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator>(const plSimdVec4i& v) const
{
  bool result[4];
  result[0] = m_v.x > v.m_v.x;
  result[1] = m_v.y > v.m_v.y;
  result[2] = m_v.z > v.m_v.z;
  result[3] = m_v.w > v.m_v.w;

  return plSimdVec4b(result[0], result[1], result[2], result[3]);
}

// static
PL_ALWAYS_INLINE plSimdVec4i plSimdVec4i::Select(const plSimdVec4b& cmp, const plSimdVec4i& ifTrue, const plSimdVec4i& ifFalse)
{
  plSimdVec4i result;
  result.m_v.x = cmp.m_v.x ? ifTrue.m_v.x : ifFalse.m_v.x;
  result.m_v.y = cmp.m_v.y ? ifTrue.m_v.y : ifFalse.m_v.y;
  result.m_v.z = cmp.m_v.z ? ifTrue.m_v.z : ifFalse.m_v.z;
  result.m_v.w = cmp.m_v.w ? ifTrue.m_v.w : ifFalse.m_v.w;

  return result;
}

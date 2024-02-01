#pragma once

PL_ALWAYS_INLINE plSimdVec4u::plSimdVec4u()
{
#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
  m_v.Set(0xCDCDCDCD);
#endif
}

PL_ALWAYS_INLINE plSimdVec4u::plSimdVec4u(plUInt32 xyzw)
{
  m_v.Set(xyzw);
}

PL_ALWAYS_INLINE plSimdVec4u::plSimdVec4u(plUInt32 x, plUInt32 y, plUInt32 z, plUInt32 w)
{
  m_v.Set(x, y, z, w);
}

PL_ALWAYS_INLINE plSimdVec4u::plSimdVec4u(plInternal::QuadUInt v)
{
  m_v = v;
}

PL_ALWAYS_INLINE void plSimdVec4u::Set(plUInt32 xyzw)
{
  m_v.Set(xyzw);
}

PL_ALWAYS_INLINE void plSimdVec4u::Set(plUInt32 x, plUInt32 y, plUInt32 z, plUInt32 w)
{
  m_v.Set(x, y, z, w);
}

PL_ALWAYS_INLINE void plSimdVec4u::SetZero()
{
  m_v.SetZero();
}

// needs to be implemented here because of include dependencies
PL_ALWAYS_INLINE plSimdVec4i::plSimdVec4i(const plSimdVec4u& u)
  : m_v(u.m_v.x, u.m_v.y, u.m_v.z, u.m_v.w)
{
}

PL_ALWAYS_INLINE plSimdVec4u::plSimdVec4u(const plSimdVec4i& i)
  : m_v(i.m_v.x, i.m_v.y, i.m_v.z, i.m_v.w)
{
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4u::ToFloat() const
{
  plSimdVec4f result;
  result.m_v.x = (float)m_v.x;
  result.m_v.y = (float)m_v.y;
  result.m_v.z = (float)m_v.z;
  result.m_v.w = (float)m_v.w;

  return result;
}

// static
PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::Truncate(const plSimdVec4f& f)
{
  plSimdVec4f clampedF = f.CompMax(plSimdVec4f::MakeZero());

  plSimdVec4u result;
  result.m_v.x = (plUInt32)clampedF.m_v.x;
  result.m_v.y = (plUInt32)clampedF.m_v.y;
  result.m_v.z = (plUInt32)clampedF.m_v.z;
  result.m_v.w = (plUInt32)clampedF.m_v.w;

  return result;
}

template <int N>
PL_ALWAYS_INLINE plUInt32 plSimdVec4u::GetComponent() const
{
  return (&m_v.x)[N];
}

PL_ALWAYS_INLINE plUInt32 plSimdVec4u::x() const
{
  return m_v.x;
}

PL_ALWAYS_INLINE plUInt32 plSimdVec4u::y() const
{
  return m_v.y;
}

PL_ALWAYS_INLINE plUInt32 plSimdVec4u::z() const
{
  return m_v.z;
}

PL_ALWAYS_INLINE plUInt32 plSimdVec4u::w() const
{
  return m_v.w;
}

template <plSwizzle::Enum s>
PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::Get() const
{
  plSimdVec4u result;

  const plUInt32* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator+(const plSimdVec4u& v) const
{
  return m_v + v.m_v;
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator-(const plSimdVec4u& v) const
{
  return m_v - v.m_v;
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::CompMul(const plSimdVec4u& v) const
{
  return m_v.CompMul(v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator|(const plSimdVec4u& v) const
{
  plSimdVec4u result;
  result.m_v.x = m_v.x | v.m_v.x;
  result.m_v.y = m_v.y | v.m_v.y;
  result.m_v.z = m_v.z | v.m_v.z;
  result.m_v.w = m_v.w | v.m_v.w;

  return result;
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator&(const plSimdVec4u& v) const
{
  plSimdVec4u result;
  result.m_v.x = m_v.x & v.m_v.x;
  result.m_v.y = m_v.y & v.m_v.y;
  result.m_v.z = m_v.z & v.m_v.z;
  result.m_v.w = m_v.w & v.m_v.w;

  return result;
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator^(const plSimdVec4u& v) const
{
  plSimdVec4u result;
  result.m_v.x = m_v.x ^ v.m_v.x;
  result.m_v.y = m_v.y ^ v.m_v.y;
  result.m_v.z = m_v.z ^ v.m_v.z;
  result.m_v.w = m_v.w ^ v.m_v.w;

  return result;
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator~() const
{
  plSimdVec4u result;
  result.m_v.x = ~m_v.x;
  result.m_v.y = ~m_v.y;
  result.m_v.z = ~m_v.z;
  result.m_v.w = ~m_v.w;

  return result;
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator<<(plUInt32 uiShift) const
{
  plSimdVec4u result;
  result.m_v.x = m_v.x << uiShift;
  result.m_v.y = m_v.y << uiShift;
  result.m_v.z = m_v.z << uiShift;
  result.m_v.w = m_v.w << uiShift;

  return result;
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator>>(plUInt32 uiShift) const
{
  plSimdVec4u result;
  result.m_v.x = m_v.x >> uiShift;
  result.m_v.y = m_v.y >> uiShift;
  result.m_v.z = m_v.z >> uiShift;
  result.m_v.w = m_v.w >> uiShift;

  return result;
}

PL_ALWAYS_INLINE plSimdVec4u& plSimdVec4u::operator+=(const plSimdVec4u& v)
{
  m_v += v.m_v;
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4u& plSimdVec4u::operator-=(const plSimdVec4u& v)
{
  m_v -= v.m_v;
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4u& plSimdVec4u::operator|=(const plSimdVec4u& v)
{
  m_v.x |= v.m_v.x;
  m_v.y |= v.m_v.y;
  m_v.z |= v.m_v.z;
  m_v.w |= v.m_v.w;
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4u& plSimdVec4u::operator&=(const plSimdVec4u& v)
{
  m_v.x &= v.m_v.x;
  m_v.y &= v.m_v.y;
  m_v.z &= v.m_v.z;
  m_v.w &= v.m_v.w;
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4u& plSimdVec4u::operator^=(const plSimdVec4u& v)
{
  m_v.x ^= v.m_v.x;
  m_v.y ^= v.m_v.y;
  m_v.z ^= v.m_v.z;
  m_v.w ^= v.m_v.w;
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4u& plSimdVec4u::operator<<=(plUInt32 uiShift)
{
  m_v.x <<= uiShift;
  m_v.y <<= uiShift;
  m_v.z <<= uiShift;
  m_v.w <<= uiShift;
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4u& plSimdVec4u::operator>>=(plUInt32 uiShift)
{
  m_v.x >>= uiShift;
  m_v.y >>= uiShift;
  m_v.z >>= uiShift;
  m_v.w >>= uiShift;
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::CompMin(const plSimdVec4u& v) const
{
  return m_v.CompMin(v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::CompMax(const plSimdVec4u& v) const
{
  return m_v.CompMax(v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4u::operator==(const plSimdVec4u& v) const
{
  bool result[4];
  result[0] = m_v.x == v.m_v.x;
  result[1] = m_v.y == v.m_v.y;
  result[2] = m_v.z == v.m_v.z;
  result[3] = m_v.w == v.m_v.w;

  return plSimdVec4b(result[0], result[1], result[2], result[3]);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4u::operator!=(const plSimdVec4u& v) const
{
  return !(*this == v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4u::operator<=(const plSimdVec4u& v) const
{
  return !(*this > v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4u::operator<(const plSimdVec4u& v) const
{
  bool result[4];
  result[0] = m_v.x < v.m_v.x;
  result[1] = m_v.y < v.m_v.y;
  result[2] = m_v.z < v.m_v.z;
  result[3] = m_v.w < v.m_v.w;

  return plSimdVec4b(result[0], result[1], result[2], result[3]);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4u::operator>=(const plSimdVec4u& v) const
{
  return !(*this < v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4u::operator>(const plSimdVec4u& v) const
{
  bool result[4];
  result[0] = m_v.x > v.m_v.x;
  result[1] = m_v.y > v.m_v.y;
  result[2] = m_v.z > v.m_v.z;
  result[3] = m_v.w > v.m_v.w;

  return plSimdVec4b(result[0], result[1], result[2], result[3]);
}

// static
PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::MakeZero()
{
  return plVec4U32::MakeZero();
}

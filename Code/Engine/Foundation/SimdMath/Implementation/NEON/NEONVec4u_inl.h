#pragma once

PL_ALWAYS_INLINE plSimdVec4u::plSimdVec4u()
{
  PL_CHECK_SIMD_ALIGNMENT(this);

#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
  m_v = vmovq_n_u32(0xCDCDCDCD);
#endif
}

PL_ALWAYS_INLINE plSimdVec4u::plSimdVec4u(plUInt32 xyzw)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_u32(xyzw);
}

PL_ALWAYS_INLINE plSimdVec4u::plSimdVec4u(plUInt32 x, plUInt32 y, plUInt32 z, plUInt32 w)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) plUInt32 values[4] = {x, y, z, w};
  m_v = vld1q_u32(values);
}

PL_ALWAYS_INLINE plSimdVec4u::plSimdVec4u(plInternal::QuadUInt v)
{
  m_v = v;
}

PL_ALWAYS_INLINE void plSimdVec4u::Set(plUInt32 xyzw)
{
  m_v = vmovq_n_u32(xyzw);
}

PL_ALWAYS_INLINE void plSimdVec4u::Set(plUInt32 x, plUInt32 y, plUInt32 z, plUInt32 w)
{
  alignas(16) plUInt32 values[4] = {x, y, z, w};
  m_v = vld1q_u32(values);
}

PL_ALWAYS_INLINE void plSimdVec4u::SetZero()
{
  m_v = vmovq_n_u32(0);
}

// needs to be implemented here because of include dependencies
PL_ALWAYS_INLINE plSimdVec4i::plSimdVec4i(const plSimdVec4u& u)
  : m_v(u.m_v)
{
}

PL_ALWAYS_INLINE plSimdVec4u::plSimdVec4u(const plSimdVec4i& i)
  : m_v(i.m_v)
{
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4u::ToFloat() const
{
  return vcvtq_f32_u32(m_v);
}

// static
PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::Truncate(const plSimdVec4f& f)
{
  return vcvtq_u32_f32(f.m_v);
}

template <int N>
PL_ALWAYS_INLINE plUInt32 plSimdVec4u::GetComponent() const
{
  return vgetq_lane_u32(m_v, N);
}

PL_ALWAYS_INLINE plUInt32 plSimdVec4u::x() const
{
  return GetComponent<0>();
}

PL_ALWAYS_INLINE plUInt32 plSimdVec4u::y() const
{
  return GetComponent<1>();
}

PL_ALWAYS_INLINE plUInt32 plSimdVec4u::z() const
{
  return GetComponent<2>();
}

PL_ALWAYS_INLINE plUInt32 plSimdVec4u::w() const
{
  return GetComponent<3>();
}

template <plSwizzle::Enum s>
PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::Get() const
{
  return __builtin_shufflevector(m_v, m_v, PL_TO_SHUFFLE(s));
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator+(const plSimdVec4u& v) const
{
  return vaddq_u32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator-(const plSimdVec4u& v) const
{
  return vsubq_u32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::CompMul(const plSimdVec4u& v) const
{
  return vmulq_u32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator|(const plSimdVec4u& v) const
{
  return vorrq_u32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator&(const plSimdVec4u& v) const
{
  return vandq_u32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator^(const plSimdVec4u& v) const
{
  return veorq_u32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator~() const
{
  return vmvnq_u32(m_v);
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator<<(plUInt32 uiShift) const
{
  return vshlq_u32(m_v, vmovq_n_u32(uiShift));
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::operator>>(plUInt32 uiShift) const
{
  return vshlq_u32(m_v, vmovq_n_u32(-uiShift));
}

PL_ALWAYS_INLINE plSimdVec4u& plSimdVec4u::operator+=(const plSimdVec4u& v)
{
  m_v = vaddq_u32(m_v, v.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4u& plSimdVec4u::operator-=(const plSimdVec4u& v)
{
  m_v = vsubq_u32(m_v, v.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4u& plSimdVec4u::operator|=(const plSimdVec4u& v)
{
  m_v = vorrq_u32(m_v, v.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4u& plSimdVec4u::operator&=(const plSimdVec4u& v)
{
  m_v = vandq_u32(m_v, v.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4u& plSimdVec4u::operator^=(const plSimdVec4u& v)
{
  m_v = veorq_u32(m_v, v.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4u& plSimdVec4u::operator<<=(plUInt32 uiShift)
{
  m_v = vshlq_u32(m_v, vmovq_n_u32(uiShift));
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4u& plSimdVec4u::operator>>=(plUInt32 uiShift)
{
  m_v = vshlq_u32(m_v, vmovq_n_u32(-uiShift));
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::CompMin(const plSimdVec4u& v) const
{
  return vminq_u32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::CompMax(const plSimdVec4u& v) const
{
  return vmaxq_u32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4u::operator==(const plSimdVec4u& v) const
{
  return vceqq_u32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4u::operator!=(const plSimdVec4u& v) const
{
  return vmvnq_u32(vceqq_u32(m_v, v.m_v));
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4u::operator<=(const plSimdVec4u& v) const
{
  return vcleq_u32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4u::operator<(const plSimdVec4u& v) const
{
  return vcltq_u32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4u::operator>=(const plSimdVec4u& v) const
{
  return vcgeq_u32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4u::operator>(const plSimdVec4u& v) const
{
  return vcgtq_u32(m_v, v.m_v);
}

// static
PL_ALWAYS_INLINE plSimdVec4u plSimdVec4u::MakeZero()
{
  return vmovq_n_u32(0);
}

// not needed atm
#if 0
void plSimdVec4u::Transpose(plSimdVec4u& v0, plSimdVec4u& v1, plSimdVec4u& v2, plSimdVec4u& v3)
{
  uint32x4x2_t P0 = vzipq_u32(v0.m_v, v2.m_v);
  uint32x4x2_t P1 = vzipq_u32(v1.m_v, v3.m_v);

  uint32x4x2_t T0 = vzipq_u32(P0.val[0], P1.val[0]);
  uint32x4x2_t T1 = vzipq_u32(P0.val[1], P1.val[1]);

  v0.m_v = T0.val[0];
  v1.m_v = T0.val[1];
  v2.m_v = T1.val[0];
  v3.m_v = T1.val[1];
}
#endif

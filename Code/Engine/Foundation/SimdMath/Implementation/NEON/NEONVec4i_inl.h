#pragma once

PLASMA_ALWAYS_INLINE plSimdVec4i::plSimdVec4i()
{
  PLASMA_CHECK_SIMD_ALIGNMENT(this);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  m_v = vmovq_n_u32(0xCDCDCDCD);
#endif
}

PLASMA_ALWAYS_INLINE plSimdVec4i::plSimdVec4i(plInt32 xyzw)
{
  PLASMA_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_s32(xyzw);
}

PLASMA_ALWAYS_INLINE plSimdVec4i::plSimdVec4i(plInt32 x, plInt32 y, plInt32 z, plInt32 w)
{
  PLASMA_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) plInt32 values[4] = {x, y, z, w};
  m_v = vld1q_s32(values);
}

PLASMA_ALWAYS_INLINE plSimdVec4i::plSimdVec4i(plInternal::QuadInt v)
{
  m_v = v;
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::MakeZero()
{
  return vmovq_n_s32(0);
}

PLASMA_ALWAYS_INLINE void plSimdVec4i::Set(plInt32 xyzw)
{
  m_v = vmovq_n_s32(xyzw);
}

PLASMA_ALWAYS_INLINE void plSimdVec4i::Set(plInt32 x, plInt32 y, plInt32 z, plInt32 w)
{
  alignas(16) plInt32 values[4] = {x, y, z, w};
  m_v = vld1q_s32(values);
}

PLASMA_ALWAYS_INLINE void plSimdVec4i::SetZero()
{
  m_v = vmovq_n_s32(0);
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Load<1>(const plInt32* pInts)
{
  m_v = vld1q_lane_s32(pInts, vmovq_n_s32(0), 0);
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Load<2>(const plInt32* pInts)
{
  m_v = vreinterpretq_s32_s64(vld1q_lane_s64(reinterpret_cast<const int64_t*>(pInts), vmovq_n_s64(0), 0));
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Load<3>(const plInt32* pInts)
{
  m_v = vcombine_s32(vld1_s32(pInts), vld1_lane_s32(pInts + 2, vmov_n_s32(0), 0));
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Load<4>(const plInt32* pInts)
{
  m_v = vld1q_s32(pInts);
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Store<1>(plInt32* pInts) const
{
  vst1q_lane_s32(pInts, m_v, 0);
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Store<2>(plInt32* pInts) const
{
  vst1q_lane_s64(reinterpret_cast<int64_t*>(pInts), vreinterpretq_s64_s32(m_v), 0);
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Store<3>(plInt32* pInts) const
{
  vst1q_lane_s64(reinterpret_cast<int64_t*>(pInts), vreinterpretq_s64_s32(m_v), 0);
  vst1q_lane_s32(pInts + 2, m_v, 2);
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Store<4>(plInt32* pInts) const
{
  vst1q_s32(pInts, m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4i::ToFloat() const
{
  return vcvtq_f32_s32(m_v);
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::Truncate(const plSimdVec4f& f)
{
  return vcvtq_s32_f32(f.m_v);
}

template <int N>
PLASMA_ALWAYS_INLINE plInt32 plSimdVec4i::GetComponent() const
{
  return vgetq_lane_s32(m_v, N);
}

PLASMA_ALWAYS_INLINE plInt32 plSimdVec4i::x() const
{
  return GetComponent<0>();
}

PLASMA_ALWAYS_INLINE plInt32 plSimdVec4i::y() const
{
  return GetComponent<1>();
}

PLASMA_ALWAYS_INLINE plInt32 plSimdVec4i::z() const
{
  return GetComponent<2>();
}

PLASMA_ALWAYS_INLINE plInt32 plSimdVec4i::w() const
{
  return GetComponent<3>();
}

template <plSwizzle::Enum s>
PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::Get() const
{
  return __builtin_shufflevector(m_v, m_v, PLASMA_TO_SHUFFLE(s));
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator-() const
{
  return vnegq_s32(m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator+(const plSimdVec4i& v) const
{
  return vaddq_s32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator-(const plSimdVec4i& v) const
{
  return vsubq_s32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::CompMul(const plSimdVec4i& v) const
{
  return vmulq_s32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::CompDiv(const plSimdVec4i& v) const
{
  int a[4];
  int b[4];
  Store<4>(a);
  v.Store<4>(b);

  for (plUInt32 i = 0; i < 4; ++i)
  {
    a[i] = a[i] / b[i];
  }

  plSimdVec4i r;
  r.Load<4>(a);
  return r;
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator|(const plSimdVec4i& v) const
{
  return vorrq_s32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator&(const plSimdVec4i& v) const
{
  return vandq_s32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator^(const plSimdVec4i& v) const
{
  return veorq_s32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator~() const
{
  return vmvnq_s32(m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator<<(plUInt32 uiShift) const
{
  return vshlq_s32(m_v, vmovq_n_s32(uiShift));
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator>>(plUInt32 uiShift) const
{
  return vshlq_s32(m_v, vmovq_n_s32(-uiShift));
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator<<(const plSimdVec4i& v) const
{
  return vshlq_s32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator>>(const plSimdVec4i& v) const
{
  return vshlq_s32(m_v, vnegq_s32(v.m_v));
}

PLASMA_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator+=(const plSimdVec4i& v)
{
  m_v = vaddq_s32(m_v, v.m_v);
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator-=(const plSimdVec4i& v)
{
  m_v = vsubq_s32(m_v, v.m_v);
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator|=(const plSimdVec4i& v)
{
  m_v = vorrq_s32(m_v, v.m_v);
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator&=(const plSimdVec4i& v)
{
  m_v = vandq_s32(m_v, v.m_v);
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator^=(const plSimdVec4i& v)
{
  m_v = veorq_s32(m_v, v.m_v);
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator<<=(plUInt32 uiShift)
{
  m_v = vshlq_s32(m_v, vmovq_n_s32(uiShift));
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator>>=(plUInt32 uiShift)
{
  m_v = vshlq_s32(m_v, vmovq_n_s32(-uiShift));
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::CompMin(const plSimdVec4i& v) const
{
  return vminq_s32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::CompMax(const plSimdVec4i& v) const
{
  return vmaxq_s32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::Abs() const
{
  return vabsq_s32(m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator==(const plSimdVec4i& v) const
{
  return vceqq_s32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator!=(const plSimdVec4i& v) const
{
  return vmvnq_u32(vceqq_s32(m_v, v.m_v));
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator<=(const plSimdVec4i& v) const
{
  return vcleq_s32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator<(const plSimdVec4i& v) const
{
  return vcltq_s32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator>=(const plSimdVec4i& v) const
{
  return vcgeq_s32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator>(const plSimdVec4i& v) const
{
  return vcgtq_s32(m_v, v.m_v);
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::Select(const plSimdVec4b& vCmp, const plSimdVec4i& vTrue, const plSimdVec4i& vFalse)
{
  return vbslq_s32(vCmp.m_v, vTrue.m_v, vFalse.m_v);
}

// not needed atm
#if 0
void plSimdVec4i::Transpose(plSimdVec4i& v0, plSimdVec4i& v1, plSimdVec4i& v2, plSimdVec4i& v3)
{
  int32x4x2_t P0 = vzipq_s32(v0.m_v, v2.m_v);
  int32x4x2_t P1 = vzipq_s32(v1.m_v, v3.m_v);

  int32x4x2_t T0 = vzipq_s32(P0.val[0], P1.val[0]);
  int32x4x2_t T1 = vzipq_s32(P0.val[1], P1.val[1]);

  v0.m_v = T0.val[0];
  v1.m_v = T0.val[1];
  v2.m_v = T1.val[0];
  v3.m_v = T1.val[1];
}
#endif

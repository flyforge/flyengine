#pragma once

PL_ALWAYS_INLINE plSimdVec4f::plSimdVec4f()
{
  PL_CHECK_SIMD_ALIGNMENT(this);

#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = vmovq_n_f32(plMath::NaN<float>());
#endif
}

PL_ALWAYS_INLINE plSimdVec4f::plSimdVec4f(float xyzw)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_f32(xyzw);
}

PL_ALWAYS_INLINE plSimdVec4f::plSimdVec4f(const plSimdFloat& xyzw)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  m_v = xyzw.m_v;
}

PL_ALWAYS_INLINE plSimdVec4f::plSimdVec4f(float x, float y, float z, float w)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) float values[4] = {x, y, z, w};
  m_v = vld1q_f32(values);
}

PL_ALWAYS_INLINE void plSimdVec4f::Set(float xyzw)
{
  m_v = vmovq_n_f32(xyzw);
}

PL_ALWAYS_INLINE void plSimdVec4f::Set(float x, float y, float z, float w)
{
  alignas(16) float values[4] = {x, y, z, w};
  m_v = vld1q_f32(values);
}

PL_ALWAYS_INLINE void plSimdVec4f::SetX(const plSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 0);
}

PL_ALWAYS_INLINE void plSimdVec4f::SetY(const plSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 1);
}

PL_ALWAYS_INLINE void plSimdVec4f::SetZ(const plSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 2);
}

PL_ALWAYS_INLINE void plSimdVec4f::SetW(const plSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 3);
}

PL_ALWAYS_INLINE void plSimdVec4f::SetZero()
{
  m_v = vmovq_n_f32(0.0f);
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Load<1>(const float* pFloat)
{
  m_v = vld1q_lane_f32(pFloat, vmovq_n_f32(0.0f), 0);
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Load<2>(const float* pFloat)
{
  m_v = vreinterpretq_f32_f64(vld1q_lane_f64(reinterpret_cast<const float64_t*>(pFloat), vmovq_n_f64(0.0), 0));
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Load<3>(const float* pFloat)
{
  m_v = vcombine_f32(vld1_f32(pFloat), vld1_lane_f32(pFloat + 2, vmov_n_f32(0.0f), 0));
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Load<4>(const float* pFloat)
{
  m_v = vld1q_f32(pFloat);
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Store<1>(float* pFloat) const
{
  vst1q_lane_f32(pFloat, m_v, 0);
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Store<2>(float* pFloat) const
{
  vst1q_lane_f64(reinterpret_cast<float64_t*>(pFloat), vreinterpretq_f64_f32(m_v), 0);
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Store<3>(float* pFloat) const
{
  vst1q_lane_f64(reinterpret_cast<float64_t*>(pFloat), vreinterpretq_f64_f32(m_v), 0);
  vst1q_lane_f32(pFloat + 2, m_v, 2);
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Store<4>(float* pFloat) const
{
  vst1q_f32(pFloat, m_v);
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetReciprocal<plMathAcc::BITS_12>() const
{
  float32x4_t x0 = vrecpeq_f32(m_v);

  // One iteration of Newton-Raphson
  float32x4_t x1 = vmulq_f32(vrecpsq_f32(m_v, x0), x0);

  return x1;
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetReciprocal<plMathAcc::BITS_23>() const
{
  float32x4_t x0 = vrecpeq_f32(m_v);

  // Two iterations of Newton-Raphson
  float32x4_t x1 = vmulq_f32(vrecpsq_f32(m_v, x0), x0);
  float32x4_t x2 = vmulq_f32(vrecpsq_f32(m_v, x1), x1);

  return x2;
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetReciprocal<plMathAcc::FULL>() const
{
  return vdivq_f32(vmovq_n_f32(1.0f), m_v);
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetInvSqrt<plMathAcc::FULL>() const
{
  return vdivq_f32(vmovq_n_f32(1.0f), vsqrtq_f32(m_v));
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetInvSqrt<plMathAcc::BITS_23>() const
{
  const float32x4_t x0 = vrsqrteq_f32(m_v);

  // Two iterations of Newton-Raphson
  const float32x4_t x1 = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x0, m_v), x0), x0);
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x1, m_v), x1), x1);
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetInvSqrt<plMathAcc::BITS_12>() const
{
  const float32x4_t x0 = vrsqrteq_f32(m_v);

  // One iteration of Newton-Raphson
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x0, m_v), x0), x0);
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetSqrt<plMathAcc::BITS_12>() const
{
  return CompMul(GetInvSqrt<plMathAcc::BITS_12>());
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetSqrt<plMathAcc::BITS_23>() const
{
  return CompMul(GetInvSqrt<plMathAcc::BITS_23>());
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetSqrt<plMathAcc::FULL>() const
{
  return vsqrtq_f32(m_v);
}

template <int N, plMathAcc::Enum acc>
void plSimdVec4f::NormalizeIfNotZero(const plSimdFloat& fEpsilon)
{
  plSimdFloat sqLength = GetLengthSquared<N>();
  uint32x4_t isNotZero = vcgtq_f32(sqLength.m_v, fEpsilon.m_v);
  m_v = vmulq_f32(m_v, sqLength.GetInvSqrt<acc>().m_v);
  m_v = vreinterpretq_f32_u32(vandq_u32(isNotZero, vreinterpretq_u32_f32(m_v)));
}

template <int N>
PL_ALWAYS_INLINE bool plSimdVec4f::IsZero() const
{
  const int mask = PL_BIT(N) - 1;
  return (plInternal::NeonMoveMask(vceqzq_f32(m_v)) & mask) == mask;
}

template <int N>
PL_ALWAYS_INLINE bool plSimdVec4f::IsZero(const plSimdFloat& fEpsilon) const
{
  const int mask = PL_BIT(N) - 1;
  float32x4_t absVal = Abs().m_v;
  return (plInternal::NeonMoveMask(vcltq_f32(absVal, fEpsilon.m_v)) & mask) == mask;
}

template <int N>
inline bool plSimdVec4f::IsNaN() const
{
  const int mask = PL_BIT(N) - 1;
  return (plInternal::NeonMoveMask(vceqq_f32(m_v, m_v)) & mask) != mask;
}

template <int N>
PL_ALWAYS_INLINE bool plSimdVec4f::IsValid() const
{
  const int mask = PL_BIT(N) - 1;
  return (plInternal::NeonMoveMask(vcgeq_u32(vreinterpretq_u32_f32(m_v), vmovq_n_u32(0x7f800000))) & mask) == 0;
}

template <int N>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::GetComponent() const
{
  return vdupq_laneq_f32(m_v, N);
}

PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::x() const
{
  return GetComponent<0>();
}

PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::y() const
{
  return GetComponent<1>();
}

PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::z() const
{
  return GetComponent<2>();
}

PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::w() const
{
  return GetComponent<3>();
}

template <plSwizzle::Enum s>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Get() const
{
  return __builtin_shufflevector(m_v, m_v, PL_TO_SHUFFLE(s));
}

template <plSwizzle::Enum s>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetCombined(const plSimdVec4f& other) const
{
  return __builtin_shufflevector(m_v, other.m_v, PL_TO_SHUFFLE(s));
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator-() const
{
  return vnegq_f32(m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator+(const plSimdVec4f& v) const
{
  return vaddq_f32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator-(const plSimdVec4f& v) const
{
  return vsubq_f32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator*(const plSimdFloat& f) const
{
  return vmulq_f32(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator/(const plSimdFloat& f) const
{
  return vdivq_f32(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompMul(const plSimdVec4f& v) const
{
  return vmulq_f32(m_v, v.m_v);
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompDiv<plMathAcc::FULL>(const plSimdVec4f& v) const
{
  return vdivq_f32(m_v, v.m_v);
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompDiv<plMathAcc::BITS_23>(const plSimdVec4f& v) const
{
  return CompMul(v.GetReciprocal<plMathAcc::BITS_23>());
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompDiv<plMathAcc::BITS_12>(const plSimdVec4f& v) const
{
  return CompMul(v.GetReciprocal<plMathAcc::BITS_12>());
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompMin(const plSimdVec4f& v) const
{
  return vminq_f32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompMax(const plSimdVec4f& v) const
{
  return vmaxq_f32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Abs() const
{
  return vabsq_f32(m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Round() const
{
  return vrndnq_f32(m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Floor() const
{
  return vrndmq_f32(m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Ceil() const
{
  return vrndpq_f32(m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Trunc() const
{
  return vrndq_f32(m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::FlipSign(const plSimdVec4b& cmp) const
{
  return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(m_v), vshlq_n_u32(cmp.m_v, 31)));
}

// static
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Select(const plSimdVec4b& cmp, const plSimdVec4f& ifTrue, const plSimdVec4f& ifFalse)
{
  return vbslq_f32(cmp.m_v, ifTrue.m_v, ifFalse.m_v);
}

PL_ALWAYS_INLINE plSimdVec4f& plSimdVec4f::operator+=(const plSimdVec4f& v)
{
  m_v = vaddq_f32(m_v, v.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4f& plSimdVec4f::operator-=(const plSimdVec4f& v)
{
  m_v = vsubq_f32(m_v, v.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4f& plSimdVec4f::operator*=(const plSimdFloat& f)
{
  m_v = vmulq_f32(m_v, f.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4f& plSimdVec4f::operator/=(const plSimdFloat& f)
{
  m_v = vdivq_f32(m_v, f.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator==(const plSimdVec4f& v) const
{
  return vceqq_f32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator!=(const plSimdVec4f& v) const
{
  return vmvnq_u32(vceqq_f32(m_v, v.m_v));
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator<=(const plSimdVec4f& v) const
{
  return vcleq_f32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator<(const plSimdVec4f& v) const
{
  return vcltq_f32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator>=(const plSimdVec4f& v) const
{
  return vcgeq_f32(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator>(const plSimdVec4f& v) const
{
  return vcgtq_f32(m_v, v.m_v);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalSum<2>() const
{
  return vpadds_f32(vget_low_f32(m_v));
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalSum<3>() const
{
  return HorizontalSum<2>() + GetComponent<2>();
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalSum<4>() const
{
  float32x2_t x0 = vpadd_f32(vget_low_f32(m_v), vget_high_f32(m_v));
  return vpadds_f32(x0);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMin<2>() const
{
  return vpmins_f32(vget_low_f32(m_v));
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMin<3>() const
{
  return vminq_f32(vmovq_n_f32(vpmins_f32(vget_low_f32(m_v))), vdupq_laneq_f32(m_v, 2));
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMin<4>() const
{
  return vpmins_f32(vpmin_f32(vget_low_f32(m_v), vget_high_f32(m_v)));
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMax<2>() const
{
  return vpmaxs_f32(vget_low_f32(m_v));
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMax<3>() const
{
  return vmaxq_f32(vmovq_n_f32(vpmaxs_f32(vget_low_f32(m_v))), vdupq_laneq_f32(m_v, 2));
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMax<4>() const
{
  return vpmaxs_f32(vpmax_f32(vget_low_f32(m_v), vget_high_f32(m_v)));
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::Dot<1>(const plSimdVec4f& v) const
{
  return vdupq_laneq_f32(vmulq_f32(m_v, v.m_v), 0);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::Dot<2>(const plSimdVec4f& v) const
{
  return vpadds_f32(vmul_f32(vget_low_f32(m_v), vget_low_f32(v.m_v)));
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::Dot<3>(const plSimdVec4f& v) const
{
  return CompMul(v).HorizontalSum<3>();
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::Dot<4>(const plSimdVec4f& v) const
{
  return CompMul(v).HorizontalSum<4>();
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CrossRH(const plSimdVec4f& v) const
{
  float32x4_t a = vmulq_f32(m_v, __builtin_shufflevector(v.m_v, v.m_v, PL_TO_SHUFFLE(plSwizzle::YZXW)));
  float32x4_t b = vmulq_f32(v.m_v, __builtin_shufflevector(m_v, m_v, PL_TO_SHUFFLE(plSwizzle::YZXW)));
  float32x4_t c = vsubq_f32(a, b);

  return __builtin_shufflevector(c, c, PL_TO_SHUFFLE(plSwizzle::YZXW));
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetOrthogonalVector() const
{
  // See http://blog.selfshadow.com/2011/10/17/perp-vectors/ - this is Stark's first variant, SIMDified.
  return CrossRH(vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(m_v), vceqq_f32(m_v, HorizontalMin<3>().m_v))));
}

// static
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::MulAdd(const plSimdVec4f& a, const plSimdVec4f& b, const plSimdVec4f& c)
{
  return vfmaq_f32(c.m_v, a.m_v, b.m_v);
}

// static
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::MulAdd(const plSimdVec4f& a, const plSimdFloat& b, const plSimdVec4f& c)
{
  return vfmaq_f32(c.m_v, a.m_v, b.m_v);
}

// static
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::MulSub(const plSimdVec4f& a, const plSimdVec4f& b, const plSimdVec4f& c)
{
  return vnegq_f32(vfmsq_f32(c.m_v, a.m_v, b.m_v));
}

// static
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::MulSub(const plSimdVec4f& a, const plSimdFloat& b, const plSimdVec4f& c)
{
  return vnegq_f32(vfmsq_f32(c.m_v, a.m_v, b.m_v));
}

// static
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CopySign(const plSimdVec4f& magnitude, const plSimdVec4f& sign)
{
  return vbslq_f32(vmovq_n_u32(0x80000000), sign.m_v, magnitude.m_v);
}

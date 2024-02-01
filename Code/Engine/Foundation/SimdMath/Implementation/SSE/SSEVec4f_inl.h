#pragma once

PL_ALWAYS_INLINE plSimdVec4f::plSimdVec4f()
{
  PL_CHECK_SIMD_ALIGNMENT(this);

#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = _mm_set1_ps(plMath::NaN<float>());
#endif
}

PL_ALWAYS_INLINE plSimdVec4f::plSimdVec4f(float fXyzw)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_set1_ps(fXyzw);
}

PL_ALWAYS_INLINE plSimdVec4f::plSimdVec4f(const plSimdFloat& fXyzw)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  m_v = fXyzw.m_v;
}

PL_ALWAYS_INLINE plSimdVec4f::plSimdVec4f(float x, float y, float z, float w)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_setr_ps(x, y, z, w);
}

PL_ALWAYS_INLINE void plSimdVec4f::Set(float fXyzw)
{
  m_v = _mm_set1_ps(fXyzw);
}

PL_ALWAYS_INLINE void plSimdVec4f::Set(float x, float y, float z, float w)
{
  m_v = _mm_setr_ps(x, y, z, w);
}

PL_ALWAYS_INLINE void plSimdVec4f::SetX(const plSimdFloat& f)
{
  m_v = _mm_move_ss(m_v, f.m_v);
}

PL_ALWAYS_INLINE void plSimdVec4f::SetY(const plSimdFloat& f)
{
  m_v = _mm_shuffle_ps(_mm_unpacklo_ps(m_v, f.m_v), m_v, PL_TO_SHUFFLE(plSwizzle::XYZW));
}

PL_ALWAYS_INLINE void plSimdVec4f::SetZ(const plSimdFloat& f)
{
  m_v = _mm_shuffle_ps(m_v, _mm_unpackhi_ps(f.m_v, m_v), PL_TO_SHUFFLE(plSwizzle::XYZW));
}

PL_ALWAYS_INLINE void plSimdVec4f::SetW(const plSimdFloat& f)
{
  m_v = _mm_shuffle_ps(m_v, _mm_unpackhi_ps(m_v, f.m_v), PL_TO_SHUFFLE(plSwizzle::XYXY));
}

PL_ALWAYS_INLINE void plSimdVec4f::SetZero()
{
  m_v = _mm_setzero_ps();
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Load<1>(const float* pFloat)
{
  m_v = _mm_load_ss(pFloat);
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Load<2>(const float* pFloat)
{
  m_v = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(pFloat)));
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Load<3>(const float* pFloat)
{
// There is a compiler bug in GCC where GCC will incorrectly optimize the alternative faster implementation.
#if PL_ENABLED(PL_COMPILER_GCC)
  m_v = _mm_set_ps(0.0f, pFloat[2], pFloat[1], pFloat[0]);
#else
  m_v = _mm_movelh_ps(_mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(pFloat))), _mm_load_ss(pFloat + 2));
#endif
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Load<4>(const float* pFloat)
{
  m_v = _mm_loadu_ps(pFloat);
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Store<1>(float* pFloat) const
{
  _mm_store_ss(pFloat, m_v);
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Store<2>(float* pFloat) const
{
  _mm_store_sd(reinterpret_cast<double*>(pFloat), _mm_castps_pd(m_v));
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Store<3>(float* pFloat) const
{
  _mm_store_sd(reinterpret_cast<double*>(pFloat), _mm_castps_pd(m_v));
  _mm_store_ss(pFloat + 2, _mm_movehl_ps(m_v, m_v));
}

template <>
PL_ALWAYS_INLINE void plSimdVec4f::Store<4>(float* pFloat) const
{
  _mm_storeu_ps(pFloat, m_v);
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetReciprocal<plMathAcc::BITS_12>() const
{
  return _mm_rcp_ps(m_v);
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetReciprocal<plMathAcc::BITS_23>() const
{
  __m128 x0 = _mm_rcp_ps(m_v);

  // One Newton-Raphson iteration
  __m128 x1 = _mm_mul_ps(x0, _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(m_v, x0)));

  return x1;
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetReciprocal<plMathAcc::FULL>() const
{
  return _mm_div_ps(_mm_set1_ps(1.0f), m_v);
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetSqrt<plMathAcc::BITS_12>() const
{
  return _mm_mul_ps(m_v, _mm_rsqrt_ps(m_v));
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetSqrt<plMathAcc::BITS_23>() const
{
  __m128 x0 = _mm_rsqrt_ps(m_v);

  // One iteration of Newton-Raphson
  __m128 x1 = _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(0.5f), x0), _mm_sub_ps(_mm_set1_ps(3.0f), _mm_mul_ps(_mm_mul_ps(m_v, x0), x0)));

  return _mm_mul_ps(m_v, x1);
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetSqrt<plMathAcc::FULL>() const
{
  return _mm_sqrt_ps(m_v);
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetInvSqrt<plMathAcc::FULL>() const
{
  return _mm_div_ps(_mm_set1_ps(1.0f), _mm_sqrt_ps(m_v));
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetInvSqrt<plMathAcc::BITS_23>() const
{
  const __m128 x0 = _mm_rsqrt_ps(m_v);

  // One iteration of Newton-Raphson
  return _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(0.5f), x0), _mm_sub_ps(_mm_set1_ps(3.0f), _mm_mul_ps(_mm_mul_ps(m_v, x0), x0)));
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetInvSqrt<plMathAcc::BITS_12>() const
{
  return _mm_rsqrt_ps(m_v);
}

template <int N, plMathAcc::Enum acc>
void plSimdVec4f::NormalizeIfNotZero(const plSimdFloat& fEpsilon)
{
  plSimdFloat sqLength = GetLengthSquared<N>();
  __m128 isNotZero = _mm_cmpgt_ps(sqLength.m_v, fEpsilon.m_v);
  m_v = _mm_mul_ps(m_v, sqLength.GetInvSqrt<acc>().m_v);
  m_v = _mm_and_ps(isNotZero, m_v);
}

template <int N>
PL_ALWAYS_INLINE bool plSimdVec4f::IsZero() const
{
  const int mask = PL_BIT(N) - 1;
  return (_mm_movemask_ps(_mm_cmpeq_ps(m_v, _mm_setzero_ps())) & mask) == mask;
}

template <int N>
PL_ALWAYS_INLINE bool plSimdVec4f::IsZero(const plSimdFloat& fEpsilon) const
{
  const int mask = PL_BIT(N) - 1;
  __m128 absVal = Abs().m_v;
  return (_mm_movemask_ps(_mm_cmplt_ps(absVal, fEpsilon.m_v)) & mask) == mask;
}

template <int N>
inline bool plSimdVec4f::IsNaN() const
{
  // NAN -> (exponent = all 1, mantissa = non-zero)

  alignas(16) const plUInt32 s_exponentMask[4] = {0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000};
  alignas(16) const plUInt32 s_mantissaMask[4] = {0x7FFFFF, 0x7FFFFF, 0x7FFFFF, 0x7FFFFF};

  __m128 exponentMask = _mm_load_ps(reinterpret_cast<const float*>(s_exponentMask));
  __m128 mantissaMask = _mm_load_ps(reinterpret_cast<const float*>(s_mantissaMask));

  __m128 exponentAll1 = _mm_cmpeq_ps(_mm_and_ps(m_v, exponentMask), exponentMask);
  __m128 mantissaNon0 = _mm_cmpneq_ps(_mm_and_ps(m_v, mantissaMask), _mm_setzero_ps());

  const int mask = PL_BIT(N) - 1;
  return (_mm_movemask_ps(_mm_and_ps(exponentAll1, mantissaNon0)) & mask) != 0;
}

template <int N>
PL_ALWAYS_INLINE bool plSimdVec4f::IsValid() const
{
  // Check the 8 exponent bits.
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // INF -> (exponent = all 1, mantissa = zero)

  alignas(16) const plUInt32 s_exponentMask[4] = {0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000};

  __m128 exponentMask = _mm_load_ps(reinterpret_cast<const float*>(s_exponentMask));

  __m128 exponentNot1 = _mm_cmpneq_ps(_mm_and_ps(m_v, exponentMask), exponentMask);

  const int mask = PL_BIT(N) - 1;
  return (_mm_movemask_ps(exponentNot1) & mask) == mask;
}

template <int N>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::GetComponent() const
{
  return _mm_shuffle_ps(m_v, m_v, PL_SHUFFLE(N, N, N, N));
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
  return _mm_shuffle_ps(m_v, m_v, PL_TO_SHUFFLE(s));
}

template <plSwizzle::Enum s>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetCombined(const plSimdVec4f& other) const
{
  return _mm_shuffle_ps(m_v, other.m_v, PL_TO_SHUFFLE(s));
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator-() const
{
  return _mm_sub_ps(_mm_setzero_ps(), m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator+(const plSimdVec4f& v) const
{
  return _mm_add_ps(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator-(const plSimdVec4f& v) const
{
  return _mm_sub_ps(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator*(const plSimdFloat& f) const
{
  return _mm_mul_ps(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::operator/(const plSimdFloat& f) const
{
  return _mm_div_ps(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompMul(const plSimdVec4f& v) const
{
  return _mm_mul_ps(m_v, v.m_v);
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompDiv<plMathAcc::FULL>(const plSimdVec4f& v) const
{
  return _mm_div_ps(m_v, v.m_v);
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompDiv<plMathAcc::BITS_23>(const plSimdVec4f& v) const
{
  __m128 x0 = _mm_rcp_ps(v.m_v);

  // One iteration of Newton-Raphson
  __m128 x1 = _mm_mul_ps(x0, _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(v.m_v, x0)));

  return _mm_mul_ps(m_v, x1);
}

template <>
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompDiv<plMathAcc::BITS_12>(const plSimdVec4f& v) const
{
  return _mm_mul_ps(m_v, _mm_rcp_ps(v.m_v));
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompMin(const plSimdVec4f& v) const
{
  return _mm_min_ps(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CompMax(const plSimdVec4f& v) const
{
  return _mm_max_ps(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Abs() const
{
  return _mm_andnot_ps(_mm_set1_ps(-0.0f), m_v);
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Round() const
{
#if PL_SSE_LEVEL >= PL_SSE_41
  return _mm_round_ps(m_v, _MM_FROUND_NINT);
#else
  PL_ASSERT_NOT_IMPLEMENTED;
#endif
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Floor() const
{
#if PL_SSE_LEVEL >= PL_SSE_41
  return _mm_round_ps(m_v, _MM_FROUND_FLOOR);
#else
  PL_ASSERT_NOT_IMPLEMENTED;
#endif
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Ceil() const
{
#if PL_SSE_LEVEL >= PL_SSE_41
  return _mm_round_ps(m_v, _MM_FROUND_CEIL);
#else
  PL_ASSERT_NOT_IMPLEMENTED;
#endif
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Trunc() const
{
#if PL_SSE_LEVEL >= PL_SSE_41
  return _mm_round_ps(m_v, _MM_FROUND_TRUNC);
#else
  PL_ASSERT_NOT_IMPLEMENTED;
#endif
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::FlipSign(const plSimdVec4b& vCmp) const
{
  return _mm_xor_ps(m_v, _mm_and_ps(vCmp.m_v, _mm_set1_ps(-0.0f)));
}

// static
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Select(const plSimdVec4b& vCmp, const plSimdVec4f& vTrue, const plSimdVec4f& vFalse)
{
#if PL_SSE_LEVEL >= PL_SSE_41
  return _mm_blendv_ps(vFalse.m_v, vTrue.m_v, vCmp.m_v);
#else
  return _mm_or_ps(_mm_andnot_ps(cmp.m_v, ifFalse.m_v), _mm_and_ps(cmp.m_v, ifTrue.m_v));
#endif
}

PL_ALWAYS_INLINE plSimdVec4f& plSimdVec4f::operator+=(const plSimdVec4f& v)
{
  m_v = _mm_add_ps(m_v, v.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4f& plSimdVec4f::operator-=(const plSimdVec4f& v)
{
  m_v = _mm_sub_ps(m_v, v.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4f& plSimdVec4f::operator*=(const plSimdFloat& f)
{
  m_v = _mm_mul_ps(m_v, f.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4f& plSimdVec4f::operator/=(const plSimdFloat& f)
{
  m_v = _mm_div_ps(m_v, f.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator==(const plSimdVec4f& v) const
{
  return _mm_cmpeq_ps(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator!=(const plSimdVec4f& v) const
{
  return _mm_cmpneq_ps(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator<=(const plSimdVec4f& v) const
{
  return _mm_cmple_ps(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator<(const plSimdVec4f& v) const
{
  return _mm_cmplt_ps(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator>=(const plSimdVec4f& v) const
{
  return _mm_cmpge_ps(m_v, v.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4f::operator>(const plSimdVec4f& v) const
{
  return _mm_cmpgt_ps(m_v, v.m_v);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalSum<2>() const
{
#if PL_SSE_LEVEL >= PL_SSE_31
  __m128 a = _mm_hadd_ps(m_v, m_v);
  return _mm_shuffle_ps(a, a, PL_TO_SHUFFLE(plSwizzle::XXXX));
#else
  return GetComponent<0>() + GetComponent<1>();
#endif
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalSum<3>() const
{
  return HorizontalSum<2>() + GetComponent<2>();
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalSum<4>() const
{
#if PL_SSE_LEVEL >= PL_SSE_31
  __m128 a = _mm_hadd_ps(m_v, m_v);
  return _mm_hadd_ps(a, a);
#else
  return (GetComponent<0>() + GetComponent<1>()) + (GetComponent<2>() + GetComponent<3>());
#endif
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMin<2>() const
{
  return _mm_min_ps(GetComponent<0>().m_v, GetComponent<1>().m_v);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMin<3>() const
{
  return _mm_min_ps(_mm_min_ps(GetComponent<0>().m_v, GetComponent<1>().m_v), GetComponent<2>().m_v);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMin<4>() const
{
  __m128 xyxyzwzw = _mm_min_ps(_mm_shuffle_ps(m_v, m_v, PL_TO_SHUFFLE(plSwizzle::ZWXY)), m_v);
  __m128 zwzwxyxy = _mm_shuffle_ps(xyxyzwzw, xyxyzwzw, PL_TO_SHUFFLE(plSwizzle::YXWZ));
  return _mm_min_ps(xyxyzwzw, zwzwxyxy);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMax<2>() const
{
  return _mm_max_ps(GetComponent<0>().m_v, GetComponent<1>().m_v);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMax<3>() const
{
  return _mm_max_ps(_mm_max_ps(GetComponent<0>().m_v, GetComponent<1>().m_v), GetComponent<2>().m_v);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMax<4>() const
{
  __m128 xyxyzwzw = _mm_max_ps(_mm_shuffle_ps(m_v, m_v, PL_TO_SHUFFLE(plSwizzle::ZWXY)), m_v);
  __m128 zwzwxyxy = _mm_shuffle_ps(xyxyzwzw, xyxyzwzw, PL_TO_SHUFFLE(plSwizzle::YXWZ));
  return _mm_max_ps(xyxyzwzw, zwzwxyxy);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::Dot<1>(const plSimdVec4f& v) const
{
#if PL_SSE_LEVEL >= PL_SSE_41
  return _mm_dp_ps(m_v, v.m_v, 0x1f);
#else
  return CompMul(v).HorizontalSum<1>();
#endif
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::Dot<2>(const plSimdVec4f& v) const
{
#if PL_SSE_LEVEL >= PL_SSE_41
  return _mm_dp_ps(m_v, v.m_v, 0x3f);
#else
  return CompMul(v).HorizontalSum<2>();
#endif
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::Dot<3>(const plSimdVec4f& v) const
{
#if PL_SSE_LEVEL >= PL_SSE_41
  return _mm_dp_ps(m_v, v.m_v, 0x7f);
#else
  return CompMul(v).HorizontalSum<3>();
#endif
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdVec4f::Dot<4>(const plSimdVec4f& v) const
{
#if PL_SSE_LEVEL >= PL_SSE_41
  return _mm_dp_ps(m_v, v.m_v, 0xff);
#else
  return CompMul(v).HorizontalSum<4>();
#endif
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CrossRH(const plSimdVec4f& v) const
{
  __m128 a = _mm_mul_ps(m_v, _mm_shuffle_ps(v.m_v, v.m_v, PL_TO_SHUFFLE(plSwizzle::YZXW)));
  __m128 b = _mm_mul_ps(v.m_v, _mm_shuffle_ps(m_v, m_v, PL_TO_SHUFFLE(plSwizzle::YZXW)));
  __m128 c = _mm_sub_ps(a, b);

  return _mm_shuffle_ps(c, c, PL_TO_SHUFFLE(plSwizzle::YZXW));
}

PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetOrthogonalVector() const
{
  // See http://blog.selfshadow.com/2011/10/17/perp-vectors/ - this is Stark's first variant, SIMDified.
  return CrossRH(_mm_and_ps(m_v, _mm_cmpeq_ps(m_v, HorizontalMin<3>().m_v)));
}

// static
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::MulAdd(const plSimdVec4f& a, const plSimdVec4f& b, const plSimdVec4f& c)
{
#if PL_SSE_LEVEL >= PL_SSE_AVX2
  return _mm_fmadd_ps(a.m_v, b.m_v, c.m_v);
#else
  return a.CompMul(b) + c;
#endif
}

// static
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::MulAdd(const plSimdVec4f& a, const plSimdFloat& b, const plSimdVec4f& c)
{
#if PL_SSE_LEVEL >= PL_SSE_AVX2
  return _mm_fmadd_ps(a.m_v, b.m_v, c.m_v);
#else
  return a * b + c;
#endif
}

// static
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::MulSub(const plSimdVec4f& a, const plSimdVec4f& b, const plSimdVec4f& c)
{
#if PL_SSE_LEVEL >= PL_SSE_AVX2
  return _mm_fmsub_ps(a.m_v, b.m_v, c.m_v);
#else
  return a.CompMul(b) - c;
#endif
}

// static
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::MulSub(const plSimdVec4f& a, const plSimdFloat& b, const plSimdVec4f& c)
{
#if PL_SSE_LEVEL >= PL_SSE_AVX2
  return _mm_fmsub_ps(a.m_v, b.m_v, c.m_v);
#else
  return a * b - c;
#endif
}

// static
PL_ALWAYS_INLINE plSimdVec4f plSimdVec4f::CopySign(const plSimdVec4f& vMagnitude, const plSimdVec4f& vSign)
{
  __m128 minusZero = _mm_set1_ps(-0.0f);
  return _mm_or_ps(_mm_andnot_ps(minusZero, vMagnitude.m_v), _mm_and_ps(minusZero, vSign.m_v));
}

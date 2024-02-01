#pragma once

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat()
{
  PL_CHECK_SIMD_ALIGNMENT(this);

#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = _mm_set1_ps(plMath::NaN<float>());
#endif
}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(float f)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_set1_ps(f);
}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(plInt32 i)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  __m128 v = _mm_cvtsi32_ss(_mm_setzero_ps(), i);
  m_v = _mm_shuffle_ps(v, v, PL_TO_SHUFFLE(plSwizzle::XXXX));
}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(plUInt32 i)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

#if PL_ENABLED(PL_PLATFORM_64BIT)
  __m128 v = _mm_cvtsi64_ss(_mm_setzero_ps(), i);
#else
  __m128 v = _mm_cvtsi32_ss(_mm_setzero_ps(), i);
#endif
  m_v = _mm_shuffle_ps(v, v, PL_TO_SHUFFLE(plSwizzle::XXXX));
}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(plAngle a)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_set1_ps(a.GetRadian());
}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(plInternal::QuadFloat v)
{
  m_v = v;
}

PL_ALWAYS_INLINE plSimdFloat::operator float() const
{
  float f;
  _mm_store_ss(&f, m_v);
  return f;
}

// static
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::MakeZero()
{
  return _mm_setzero_ps();
}

// static
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::MakeNaN()
{
  return _mm_set1_ps(plMath::NaN<float>());
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::operator+(const plSimdFloat& f) const
{
  return _mm_add_ps(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::operator-(const plSimdFloat& f) const
{
  return _mm_sub_ps(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::operator*(const plSimdFloat& f) const
{
  return _mm_mul_ps(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::operator/(const plSimdFloat& f) const
{
  return _mm_div_ps(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat& plSimdFloat::operator+=(const plSimdFloat& f)
{
  m_v = _mm_add_ps(m_v, f.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdFloat& plSimdFloat::operator-=(const plSimdFloat& f)
{
  m_v = _mm_sub_ps(m_v, f.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdFloat& plSimdFloat::operator*=(const plSimdFloat& f)
{
  m_v = _mm_mul_ps(m_v, f.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdFloat& plSimdFloat::operator/=(const plSimdFloat& f)
{
  m_v = _mm_div_ps(m_v, f.m_v);
  return *this;
}

PL_ALWAYS_INLINE bool plSimdFloat::IsEqual(const plSimdFloat& rhs, const plSimdFloat& fEpsilon) const
{
  plSimdFloat minusEps = rhs - fEpsilon;
  plSimdFloat plusEps = rhs + fEpsilon;
  return ((*this >= minusEps) && (*this <= plusEps));
}

PL_ALWAYS_INLINE bool plSimdFloat::operator==(const plSimdFloat& f) const
{
  return _mm_comieq_ss(m_v, f.m_v) == 1;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator!=(const plSimdFloat& f) const
{
  return _mm_comineq_ss(m_v, f.m_v) == 1;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator>=(const plSimdFloat& f) const
{
  return _mm_comige_ss(m_v, f.m_v) == 1;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator>(const plSimdFloat& f) const
{
  return _mm_comigt_ss(m_v, f.m_v) == 1;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator<=(const plSimdFloat& f) const
{
  return _mm_comile_ss(m_v, f.m_v) == 1;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator<(const plSimdFloat& f) const
{
  return _mm_comilt_ss(m_v, f.m_v) == 1;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator==(float f) const
{
  return (*this) == plSimdFloat(f);
}

PL_ALWAYS_INLINE bool plSimdFloat::operator!=(float f) const
{
  return (*this) != plSimdFloat(f);
}

PL_ALWAYS_INLINE bool plSimdFloat::operator>(float f) const
{
  return (*this) > plSimdFloat(f);
}

PL_ALWAYS_INLINE bool plSimdFloat::operator>=(float f) const
{
  return (*this) >= plSimdFloat(f);
}

PL_ALWAYS_INLINE bool plSimdFloat::operator<(float f) const
{
  return (*this) < plSimdFloat(f);
}

PL_ALWAYS_INLINE bool plSimdFloat::operator<=(float f) const
{
  return (*this) <= plSimdFloat(f);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetReciprocal<plMathAcc::FULL>() const
{
  return _mm_div_ps(_mm_set1_ps(1.0f), m_v);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetReciprocal<plMathAcc::BITS_23>() const
{
  __m128 x0 = _mm_rcp_ps(m_v);

  // One iteration of Newton-Raphson
  __m128 x1 = _mm_mul_ps(x0, _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(m_v, x0)));

  return x1;
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetReciprocal<plMathAcc::BITS_12>() const
{
  return _mm_rcp_ps(m_v);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetInvSqrt<plMathAcc::FULL>() const
{
  return _mm_div_ps(_mm_set1_ps(1.0f), _mm_sqrt_ps(m_v));
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetInvSqrt<plMathAcc::BITS_23>() const
{
  const __m128 x0 = _mm_rsqrt_ps(m_v);

  // One iteration of Newton-Raphson
  return _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(0.5f), x0), _mm_sub_ps(_mm_set1_ps(3.0f), _mm_mul_ps(_mm_mul_ps(m_v, x0), x0)));
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetInvSqrt<plMathAcc::BITS_12>() const
{
  return _mm_rsqrt_ps(m_v);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetSqrt<plMathAcc::FULL>() const
{
  return _mm_sqrt_ps(m_v);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetSqrt<plMathAcc::BITS_23>() const
{
  return (*this) * GetInvSqrt<plMathAcc::BITS_23>();
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetSqrt<plMathAcc::BITS_12>() const
{
  return (*this) * GetInvSqrt<plMathAcc::BITS_12>();
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::Max(const plSimdFloat& f) const
{
  return _mm_max_ps(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::Min(const plSimdFloat& f) const
{
  return _mm_min_ps(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::Abs() const
{
  return _mm_andnot_ps(_mm_set1_ps(-0.0f), m_v);
}

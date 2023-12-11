#pragma once

PLASMA_ALWAYS_INLINE plSimdVec4i::plSimdVec4i()
{
  PLASMA_CHECK_SIMD_ALIGNMENT(this);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  m_v = _mm_set1_epi32(0xCDCDCDCD);
#endif
}

PLASMA_ALWAYS_INLINE plSimdVec4i::plSimdVec4i(plInt32 iXyzw)
{
  PLASMA_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_set1_epi32(iXyzw);
}

PLASMA_ALWAYS_INLINE plSimdVec4i::plSimdVec4i(plInt32 x, plInt32 y, plInt32 z, plInt32 w)
{
  PLASMA_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_setr_epi32(x, y, z, w);
}

PLASMA_ALWAYS_INLINE plSimdVec4i::plSimdVec4i(plInternal::QuadInt v)
{
  m_v = v;
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::MakeZero()
{
  return _mm_setzero_si128();
}

PLASMA_ALWAYS_INLINE void plSimdVec4i::Set(plInt32 iXyzw)
{
  m_v = _mm_set1_epi32(iXyzw);
}

PLASMA_ALWAYS_INLINE void plSimdVec4i::Set(plInt32 x, plInt32 y, plInt32 z, plInt32 w)
{
  m_v = _mm_setr_epi32(x, y, z, w);
}

PLASMA_ALWAYS_INLINE void plSimdVec4i::SetZero()
{
  m_v = _mm_setzero_si128();
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Load<1>(const plInt32* pInts)
{
  m_v = _mm_loadu_si32(pInts);
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Load<2>(const plInt32* pInts)
{
  m_v = _mm_loadu_si64(pInts);
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Load<3>(const plInt32* pInts)
{
  m_v = _mm_setr_epi32(pInts[0], pInts[1], pInts[2], 0);
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Load<4>(const plInt32* pInts)
{
  m_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pInts));
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Store<1>(plInt32* pInts) const
{
  _mm_storeu_si32(pInts, m_v);
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Store<2>(plInt32* pInts) const
{
  _mm_storeu_si64(pInts, m_v);
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Store<3>(plInt32* pInts) const
{
  _mm_storeu_si64(pInts, m_v);
  _mm_storeu_si32(pInts + 2, _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(m_v), _mm_castsi128_ps(m_v))));
}

template <>
PLASMA_ALWAYS_INLINE void plSimdVec4i::Store<4>(plInt32* pInts) const
{
  _mm_storeu_si128(reinterpret_cast<__m128i*>(pInts), m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4i::ToFloat() const
{
  return _mm_cvtepi32_ps(m_v);
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::Truncate(const plSimdVec4f& f)
{
  return _mm_cvttps_epi32(f.m_v);
}

template <int N>
PLASMA_ALWAYS_INLINE plInt32 plSimdVec4i::GetComponent() const
{
#if PLASMA_SSE_LEVEL >= PLASMA_SSE_41
  return _mm_extract_epi32(m_v, N);
#else
  return m_v.m128i_i32[N];
#endif
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
  return _mm_shuffle_epi32(m_v, PLASMA_TO_SHUFFLE(s));
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator-() const
{
  return _mm_sub_epi32(_mm_setzero_si128(), m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator+(const plSimdVec4i& v) const
{
  return _mm_add_epi32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator-(const plSimdVec4i& v) const
{
  return _mm_sub_epi32(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::CompMul(const plSimdVec4i& v) const
{
#if PLASMA_SSE_LEVEL >= PLASMA_SSE_41
  return _mm_mullo_epi32(m_v, v.m_v);
#else
  PLASMA_ASSERT_NOT_IMPLEMENTED; // not sure whether this code works so better assert
  __m128i tmp1 = _mm_mul_epu32(m_v, v.m_v);
  __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(m_v, 4), _mm_srli_si128(v.m_v, 4));
  return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, PLASMA_SHUFFLE(0, 2, 0, 0)), _mm_shuffle_epi32(tmp2, PLASMA_SHUFFLE(0, 2, 0, 0)));
#endif
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::CompDiv(const plSimdVec4i& v) const
{
#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC)
  return _mm_div_epi32(m_v, v.m_v);
#else
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
#endif
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator|(const plSimdVec4i& v) const
{
  return _mm_or_si128(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator&(const plSimdVec4i& v) const
{
  return _mm_and_si128(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator^(const plSimdVec4i& v) const
{
  return _mm_xor_si128(m_v, v.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator~() const
{
  __m128i ones = _mm_cmpeq_epi8(_mm_setzero_si128(), _mm_setzero_si128());
  return _mm_xor_si128(ones, m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator<<(plUInt32 uiShift) const
{
  return _mm_slli_epi32(m_v, uiShift);
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::operator>>(plUInt32 uiShift) const
{
  return _mm_srai_epi32(m_v, uiShift);
}

PLASMA_FORCE_INLINE plSimdVec4i plSimdVec4i::operator<<(const plSimdVec4i& v) const
{
  int a[4];
  int b[4];
  Store<4>(a);
  v.Store<4>(b);

  for (plUInt32 i = 0; i < 4; ++i)
  {
    a[i] = a[i] << b[i];
  }

  plSimdVec4i r;
  r.Load<4>(a);
  return r;
}

PLASMA_FORCE_INLINE plSimdVec4i plSimdVec4i::operator>>(const plSimdVec4i& v) const
{
  int a[4];
  int b[4];
  Store<4>(a);
  v.Store<4>(b);

  for (plUInt32 i = 0; i < 4; ++i)
  {
    a[i] = a[i] >> b[i];
  }

  plSimdVec4i r;
  r.Load<4>(a);
  return r;
}

PLASMA_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator+=(const plSimdVec4i& v)
{
  m_v = _mm_add_epi32(m_v, v.m_v);
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator-=(const plSimdVec4i& v)
{
  m_v = _mm_sub_epi32(m_v, v.m_v);
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator|=(const plSimdVec4i& v)
{
  m_v = _mm_or_si128(m_v, v.m_v);
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator&=(const plSimdVec4i& v)
{
  m_v = _mm_and_si128(m_v, v.m_v);
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator^=(const plSimdVec4i& v)
{
  m_v = _mm_xor_si128(m_v, v.m_v);
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator<<=(plUInt32 uiShift)
{
  m_v = _mm_slli_epi32(m_v, uiShift);
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4i& plSimdVec4i::operator>>=(plUInt32 uiShift)
{
  m_v = _mm_srai_epi32(m_v, uiShift);
  return *this;
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::CompMin(const plSimdVec4i& v) const
{
#if PLASMA_SSE_LEVEL >= PLASMA_SSE_41
  return _mm_min_epi32(m_v, v.m_v);
#else
  __m128i mask = _mm_cmplt_epi32(m_v, v.m_v);
  return _mm_or_si128(_mm_and_si128(mask, m_v), _mm_andnot_si128(mask, v.m_v));
#endif
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::CompMax(const plSimdVec4i& v) const
{
#if PLASMA_SSE_LEVEL >= PLASMA_SSE_41
  return _mm_max_epi32(m_v, v.m_v);
#else
  __m128i mask = _mm_cmpgt_epi32(m_v, v.m_v);
  return _mm_or_si128(_mm_and_si128(mask, m_v), _mm_andnot_si128(mask, v.m_v));
#endif
}

PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::Abs() const
{
#if PLASMA_SSE_LEVEL >= PLASMA_SSE_31
  return _mm_abs_epi32(m_v);
#else
  __m128i negMask = _mm_cmplt_epi32(m_v, _mm_setzero_si128());
  __m128i neg = _mm_sub_epi32(_mm_setzero_si128(), m_v);
  return _mm_or_si128(_mm_and_si128(negMask, neg), _mm_andnot_si128(negMask, m_v));
#endif
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator==(const plSimdVec4i& v) const
{
  return _mm_castsi128_ps(_mm_cmpeq_epi32(m_v, v.m_v));
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator!=(const plSimdVec4i& v) const
{
  return !(*this == v);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator<=(const plSimdVec4i& v) const
{
  return !(*this > v);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator<(const plSimdVec4i& v) const
{
  return _mm_castsi128_ps(_mm_cmplt_epi32(m_v, v.m_v));
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator>=(const plSimdVec4i& v) const
{
  return !(*this < v);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4i::operator>(const plSimdVec4i& v) const
{
  return _mm_castsi128_ps(_mm_cmpgt_epi32(m_v, v.m_v));
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4i plSimdVec4i::Select(const plSimdVec4b& vCmp, const plSimdVec4i& vTrue, const plSimdVec4i& vFalse)
{
#if PLASMA_SSE_LEVEL >= PLASMA_SSE_41
  return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(vFalse.m_v), _mm_castsi128_ps(vTrue.m_v), vCmp.m_v));
#else
  return _mm_castps_si128(_mm_or_ps(_mm_andnot_ps(cmp.m_v, _mm_castsi128_ps(ifFalse.m_v)), _mm_and_ps(cmp.m_v, _mm_castsi128_ps(ifTrue.m_v))));
#endif
}

// not needed atm
#if 0
void plSimdVec4i::Transpose(plSimdVec4i& v0, plSimdVec4i& v1, plSimdVec4i& v2, plSimdVec4i& v3)
{
  __m128i T0 = _mm_unpacklo_epi32(v0.m_v, v1.m_v);
  __m128i T1 = _mm_unpacklo_epi32(v2.m_v, v3.m_v);
  __m128i T2 = _mm_unpackhi_epi32(v0.m_v, v1.m_v);
  __m128i T3 = _mm_unpackhi_epi32(v2.m_v, v3.m_v);

  v0.m_v = _mm_unpacklo_epi64(T0, T1);
  v1.m_v = _mm_unpackhi_epi64(T0, T1);
  v2.m_v = _mm_unpacklo_epi64(T2, T3);
  v3.m_v = _mm_unpackhi_epi64(T2, T3);
}
#endif
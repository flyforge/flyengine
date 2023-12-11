#pragma once

PLASMA_ALWAYS_INLINE plSimdVec4b::plSimdVec4b()
{
  PLASMA_CHECK_SIMD_ALIGNMENT(this);
}

PLASMA_ALWAYS_INLINE plSimdVec4b::plSimdVec4b(bool b)
{
  PLASMA_CHECK_SIMD_ALIGNMENT(this);

  plUInt32 mask = b ? 0xFFFFFFFF : 0;
  __m128 tmp = _mm_load_ss((float*)&mask);
  m_v = _mm_shuffle_ps(tmp, tmp, PLASMA_TO_SHUFFLE(plSwizzle::XXXX));
}

PLASMA_ALWAYS_INLINE plSimdVec4b::plSimdVec4b(bool x, bool y, bool z, bool w)
{
  PLASMA_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) plUInt32 mask[4] = {x ? 0xFFFFFFFF : 0, y ? 0xFFFFFFFF : 0, z ? 0xFFFFFFFF : 0, w ? 0xFFFFFFFF : 0};
  m_v = _mm_load_ps((float*)mask);
}

PLASMA_ALWAYS_INLINE plSimdVec4b::plSimdVec4b(plInternal::QuadBool v)
{
  m_v = v;
}

template <int N>
PLASMA_ALWAYS_INLINE bool plSimdVec4b::GetComponent() const
{
  return _mm_movemask_ps(_mm_shuffle_ps(m_v, m_v, PLASMA_SHUFFLE(N, N, N, N))) != 0;
}

PLASMA_ALWAYS_INLINE bool plSimdVec4b::x() const
{
  return GetComponent<0>();
}

PLASMA_ALWAYS_INLINE bool plSimdVec4b::y() const
{
  return GetComponent<1>();
}

PLASMA_ALWAYS_INLINE bool plSimdVec4b::z() const
{
  return GetComponent<2>();
}

PLASMA_ALWAYS_INLINE bool plSimdVec4b::w() const
{
  return GetComponent<3>();
}

template <plSwizzle::Enum s>
PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4b::Get() const
{
  return _mm_shuffle_ps(m_v, m_v, PLASMA_TO_SHUFFLE(s));
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4b::operator&&(const plSimdVec4b& rhs) const
{
  return _mm_and_ps(m_v, rhs.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4b::operator||(const plSimdVec4b& rhs) const
{
  return _mm_or_ps(m_v, rhs.m_v);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4b::operator!() const
{
  __m128 allTrue = _mm_cmpeq_ps(_mm_setzero_ps(), _mm_setzero_ps());
  return _mm_xor_ps(m_v, allTrue);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4b::operator==(const plSimdVec4b& rhs) const
{
  return !(*this != rhs);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4b::operator!=(const plSimdVec4b& rhs) const
{
  return _mm_xor_ps(m_v, rhs.m_v);
}

template <int N>
PLASMA_ALWAYS_INLINE bool plSimdVec4b::AllSet() const
{
  const int mask = PLASMA_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) == mask;
}

template <int N>
PLASMA_ALWAYS_INLINE bool plSimdVec4b::AnySet() const
{
  const int mask = PLASMA_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) != 0;
}

template <int N>
PLASMA_ALWAYS_INLINE bool plSimdVec4b::NoneSet() const
{
  const int mask = PLASMA_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) == 0;
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4b::Select(const plSimdVec4b& vCmp, const plSimdVec4b& vTrue, const plSimdVec4b& vFalse)
{
#if PLASMA_SSE_LEVEL >= PLASMA_SSE_41
  return _mm_blendv_ps(vFalse.m_v, vTrue.m_v, vCmp.m_v);
#else
  return _mm_or_ps(_mm_andnot_ps(cmp.m_v, ifFalse.m_v), _mm_and_ps(cmp.m_v, ifTrue.m_v));
#endif
}
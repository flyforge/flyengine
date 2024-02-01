#pragma once

PL_ALWAYS_INLINE plSimdVec4b::plSimdVec4b()
{
  PL_CHECK_SIMD_ALIGNMENT(this);
}

PL_ALWAYS_INLINE plSimdVec4b::plSimdVec4b(bool b)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_u32(b ? 0xFFFFFFFF : 0);
}

PL_ALWAYS_INLINE plSimdVec4b::plSimdVec4b(bool x, bool y, bool z, bool w)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) plUInt32 mask[4] = {x ? 0xFFFFFFFF : 0, y ? 0xFFFFFFFF : 0, z ? 0xFFFFFFFF : 0, w ? 0xFFFFFFFF : 0};
  m_v = vld1q_u32(mask);
}

PL_ALWAYS_INLINE plSimdVec4b::plSimdVec4b(plInternal::QuadBool v)
{
  m_v = v;
}

template <int N>
PL_ALWAYS_INLINE bool plSimdVec4b::GetComponent() const
{
  return vgetq_lane_u32(m_v, N) & 1;
}

PL_ALWAYS_INLINE bool plSimdVec4b::x() const
{
  return GetComponent<0>();
}

PL_ALWAYS_INLINE bool plSimdVec4b::y() const
{
  return GetComponent<1>();
}

PL_ALWAYS_INLINE bool plSimdVec4b::z() const
{
  return GetComponent<2>();
}

PL_ALWAYS_INLINE bool plSimdVec4b::w() const
{
  return GetComponent<3>();
}

template <plSwizzle::Enum s>
PL_ALWAYS_INLINE plSimdVec4b plSimdVec4b::Get() const
{
  return __builtin_shufflevector(m_v, m_v, PL_TO_SHUFFLE(s));
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4b::operator&&(const plSimdVec4b& rhs) const
{
  return vandq_u32(m_v, rhs.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4b::operator||(const plSimdVec4b& rhs) const
{
  return vorrq_u32(m_v, rhs.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4b::operator!() const
{
  return vmvnq_u32(m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4b::operator==(const plSimdVec4b& rhs) const
{
  return vceqq_u32(m_v, rhs.m_v);
}

PL_ALWAYS_INLINE plSimdVec4b plSimdVec4b::operator!=(const plSimdVec4b& rhs) const
{
  return veorq_u32(m_v, rhs.m_v);
}

template <int N>
PL_ALWAYS_INLINE bool plSimdVec4b::AllSet() const
{
  const int mask = PL_BIT(N) - 1;
  return (plInternal::NeonMoveMask(m_v) & mask) == mask;
}

template <int N>
PL_ALWAYS_INLINE bool plSimdVec4b::AnySet() const
{
  const int mask = PL_BIT(N) - 1;
  return (plInternal::NeonMoveMask(m_v) & mask) != 0;
}

template <int N>
PL_ALWAYS_INLINE bool plSimdVec4b::NoneSet() const
{
  const int mask = PL_BIT(N) - 1;
  return (plInternal::NeonMoveMask(m_v) & mask) == 0;
}

// static
PL_ALWAYS_INLINE plSimdVec4b plSimdVec4b::Select(const plSimdVec4b& vCmp, const plSimdVec4b& vTrue, const plSimdVec4b& vFalse)
{
  return vbslq_u32(vCmp.m_v, vTrue.m_v, vFalse.m_v);
}

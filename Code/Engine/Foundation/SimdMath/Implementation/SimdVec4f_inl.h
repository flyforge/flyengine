#pragma once

PLASMA_ALWAYS_INLINE plSimdVec4f::plSimdVec4f(plInternal::QuadFloat v)
{
  m_v = v;
}

template <int N, plMathAcc::Enum acc>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::GetLength() const
{
  const plSimdFloat squaredLen = GetLengthSquared<N>();
  return squaredLen.GetSqrt<acc>();
}

template <int N, plMathAcc::Enum acc>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::GetInvLength() const
{
  const plSimdFloat squaredLen = GetLengthSquared<N>();
  return squaredLen.GetInvSqrt<acc>();
}

template <int N>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::GetLengthSquared() const
{
  return Dot<N>(*this);
}

template <int N, plMathAcc::Enum acc>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::GetLengthAndNormalize()
{
  const plSimdFloat squaredLen = GetLengthSquared<N>();
  const plSimdFloat reciprocalLen = squaredLen.GetInvSqrt<acc>();
  *this = (*this) * reciprocalLen;
  return squaredLen * reciprocalLen;
}

template <int N, plMathAcc::Enum acc>
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::GetNormalized() const
{
  return (*this) * GetInvLength<N, acc>();
}

template <int N, plMathAcc::Enum acc>
PLASMA_ALWAYS_INLINE void plSimdVec4f::Normalize()
{
  *this = GetNormalized<N, acc>();
}

template <int N>
PLASMA_ALWAYS_INLINE bool plSimdVec4f::IsNormalized(const plSimdFloat& fEpsilon) const
{
  const plSimdFloat sqLength = GetLengthSquared<N>();
  return sqLength.IsEqual(1.0f, fEpsilon);
}

inline plSimdFloat plSimdVec4f::GetComponent(int i) const
{
  switch (i)
  {
    case 0:
      return GetComponent<0>();

    case 1:
      return GetComponent<1>();

    case 2:
      return GetComponent<2>();

    default:
      return GetComponent<3>();
  }
}

PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Fraction() const
{
  return *this - Trunc();
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdVec4f::Lerp(const plSimdVec4f& a, const plSimdVec4f& b, const plSimdVec4f& t)
{
  return a + t.CompMul(b - a);
}

PLASMA_ALWAYS_INLINE plSimdVec4b plSimdVec4f::IsEqual(const plSimdVec4f& rhs, const plSimdFloat& fEpsilon) const
{
  plSimdVec4f minusEps = rhs - plSimdVec4f(fEpsilon);
  plSimdVec4f plusEps = rhs + plSimdVec4f(fEpsilon);
  return (*this >= minusEps) && (*this <= plusEps);
}

template <>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalSum<1>() const
{
  return GetComponent<0>();
}

template <>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMin<1>() const
{
  return GetComponent<0>();
}

template <>
PLASMA_ALWAYS_INLINE plSimdFloat plSimdVec4f::HorizontalMax<1>() const
{
  return GetComponent<0>();
}

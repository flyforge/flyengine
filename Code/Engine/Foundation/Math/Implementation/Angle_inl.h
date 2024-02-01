#pragma once

template <typename Type>
constexpr PL_ALWAYS_INLINE Type plAngle::Pi()
{
  return static_cast<Type>(3.1415926535897932384626433832795);
}

template <typename Type>
constexpr PL_ALWAYS_INLINE Type plAngle::DegToRadMultiplier()
{
  return Pi<Type>() / (Type)180;
}

template <typename Type>
constexpr PL_ALWAYS_INLINE Type plAngle::RadToDegMultiplier()
{
  return ((Type)180) / Pi<Type>();
}

template <typename Type>
constexpr Type plAngle::DegToRad(Type f)
{
  return f * DegToRadMultiplier<Type>();
}

template <typename Type>
constexpr Type plAngle::RadToDeg(Type f)
{
  return f * RadToDegMultiplier<Type>();
}

constexpr inline plAngle plAngle::MakeFromDegree(float fDegree)
{
  return plAngle(DegToRad(fDegree));
}

constexpr PL_ALWAYS_INLINE plAngle plAngle::MakeFromRadian(float fRadian)
{
  return plAngle(fRadian);
}

constexpr inline float plAngle::GetDegree() const
{
  return RadToDeg(m_fRadian);
}

constexpr PL_ALWAYS_INLINE float plAngle::GetRadian() const
{
  return m_fRadian;
}

inline plAngle plAngle::GetNormalizedRange() const
{
  plAngle out(m_fRadian);
  out.NormalizeRange();
  return out;
}

inline bool plAngle::IsEqualSimple(plAngle rhs, plAngle epsilon) const
{
  const plAngle diff = AngleBetween(*this, rhs);

  return ((diff.m_fRadian >= -epsilon.m_fRadian) && (diff.m_fRadian <= epsilon.m_fRadian));
}

inline bool plAngle::IsEqualNormalized(plAngle rhs, plAngle epsilon) const
{
  // equality between normalized angles
  const plAngle aNorm = GetNormalizedRange();
  const plAngle bNorm = rhs.GetNormalizedRange();

  return aNorm.IsEqualSimple(bNorm, epsilon);
}

constexpr PL_ALWAYS_INLINE plAngle plAngle::operator-() const
{
  return plAngle(-m_fRadian);
}

PL_ALWAYS_INLINE void plAngle::operator+=(plAngle r)
{
  m_fRadian += r.m_fRadian;
}

PL_ALWAYS_INLINE void plAngle::operator-=(plAngle r)
{
  m_fRadian -= r.m_fRadian;
}

constexpr inline plAngle plAngle::operator+(plAngle r) const
{
  return plAngle(m_fRadian + r.m_fRadian);
}

constexpr inline plAngle plAngle::operator-(plAngle r) const
{
  return plAngle(m_fRadian - r.m_fRadian);
}

constexpr PL_ALWAYS_INLINE bool plAngle::operator==(const plAngle& r) const
{
  return m_fRadian == r.m_fRadian;
}

constexpr PL_ALWAYS_INLINE bool plAngle::operator!=(const plAngle& r) const
{
  return m_fRadian != r.m_fRadian;
}

constexpr PL_ALWAYS_INLINE bool plAngle::operator<(const plAngle& r) const
{
  return m_fRadian < r.m_fRadian;
}

constexpr PL_ALWAYS_INLINE bool plAngle::operator>(const plAngle& r) const
{
  return m_fRadian > r.m_fRadian;
}

constexpr PL_ALWAYS_INLINE bool plAngle::operator<=(const plAngle& r) const
{
  return m_fRadian <= r.m_fRadian;
}

constexpr PL_ALWAYS_INLINE bool plAngle::operator>=(const plAngle& r) const
{
  return m_fRadian >= r.m_fRadian;
}

constexpr inline plAngle operator*(plAngle a, float f)
{
  return plAngle::MakeFromRadian(a.GetRadian() * f);
}

constexpr inline plAngle operator*(float f, plAngle a)
{
  return plAngle::MakeFromRadian(a.GetRadian() * f);
}

constexpr inline plAngle operator/(plAngle a, float f)
{
  return plAngle::MakeFromRadian(a.GetRadian() / f);
}

constexpr inline float operator/(plAngle a, plAngle b)
{
  return a.GetRadian() / b.GetRadian();
}

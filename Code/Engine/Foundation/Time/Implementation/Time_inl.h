#pragma once

#include <Foundation/Basics.h>

constexpr PLASMA_ALWAYS_INLINE plTime::plTime(double fTime)
  : m_fTime(fTime)
{
}

constexpr PLASMA_ALWAYS_INLINE float plTime::AsFloatInSeconds() const
{
  return static_cast<float>(m_fTime);
}

constexpr PLASMA_ALWAYS_INLINE double plTime::GetNanoseconds() const
{
  return m_fTime * 1000000000.0;
}

constexpr PLASMA_ALWAYS_INLINE double plTime::GetMicroseconds() const
{
  return m_fTime * 1000000.0;
}

constexpr PLASMA_ALWAYS_INLINE double plTime::GetMilliseconds() const
{
  return m_fTime * 1000.0;
}

constexpr PLASMA_ALWAYS_INLINE double plTime::GetSeconds() const
{
  return m_fTime;
}

constexpr PLASMA_ALWAYS_INLINE double plTime::GetMinutes() const
{
  return m_fTime / 60.0;
}

constexpr PLASMA_ALWAYS_INLINE double plTime::GetHours() const
{
  return m_fTime / (60.0 * 60.0);
}

constexpr PLASMA_ALWAYS_INLINE void plTime::operator-=(const plTime& other)
{
  m_fTime -= other.m_fTime;
}

constexpr PLASMA_ALWAYS_INLINE void plTime::operator+=(const plTime& other)
{
  m_fTime += other.m_fTime;
}

constexpr PLASMA_ALWAYS_INLINE void plTime::operator*=(double fFactor)
{
  m_fTime *= fFactor;
}

constexpr PLASMA_ALWAYS_INLINE void plTime::operator/=(double fFactor)
{
  m_fTime /= fFactor;
}

constexpr PLASMA_ALWAYS_INLINE plTime plTime::operator-() const
{
  return plTime(-m_fTime);
}

constexpr PLASMA_ALWAYS_INLINE plTime plTime::operator-(const plTime& other) const
{
  return plTime(m_fTime - other.m_fTime);
}

constexpr PLASMA_ALWAYS_INLINE plTime plTime::operator+(const plTime& other) const
{
  return plTime(m_fTime + other.m_fTime);
}

constexpr PLASMA_ALWAYS_INLINE plTime operator*(plTime t, double f)
{
  return plTime::MakeFromSeconds(t.GetSeconds() * f);
}

constexpr PLASMA_ALWAYS_INLINE plTime operator*(double f, plTime t)
{
  return plTime::MakeFromSeconds(t.GetSeconds() * f);
}

constexpr PLASMA_ALWAYS_INLINE plTime operator*(plTime f, plTime t)
{
  return plTime::MakeFromSeconds(t.GetSeconds() * f.GetSeconds());
}

constexpr PLASMA_ALWAYS_INLINE plTime operator/(plTime t, double f)
{
  return plTime::MakeFromSeconds(t.GetSeconds() / f);
}

constexpr PLASMA_ALWAYS_INLINE plTime operator/(double f, plTime t)
{
  return plTime::MakeFromSeconds(f / t.GetSeconds());
}

constexpr PLASMA_ALWAYS_INLINE plTime operator/(plTime f, plTime t)
{
  return plTime::MakeFromSeconds(f.GetSeconds() / t.GetSeconds());
}

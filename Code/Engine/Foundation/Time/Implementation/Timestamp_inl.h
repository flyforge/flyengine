#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Math.h>

inline plTimestamp::plTimestamp() = default;

inline bool plTimestamp::IsValid() const
{
  return m_iTimestamp != PLASMA_INVALID_TIME_STAMP;
}

inline void plTimestamp::operator+=(const plTime& timeSpan)
{
  PLASMA_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  m_iTimestamp += (plInt64)timeSpan.GetMicroseconds();
}

inline void plTimestamp::operator-=(const plTime& timeSpan)
{
  PLASMA_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  m_iTimestamp -= (plInt64)timeSpan.GetMicroseconds();
}

inline const plTime plTimestamp::operator-(const plTimestamp& other) const
{
  PLASMA_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  PLASMA_ASSERT_DEBUG(other.IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return plTime::MakeFromMicroseconds((double)(m_iTimestamp - other.m_iTimestamp));
}

inline const plTimestamp plTimestamp::operator+(const plTime& timeSpan) const
{
  PLASMA_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return plTimestamp::MakeFromInt(m_iTimestamp + (plInt64)timeSpan.GetMicroseconds(), plSIUnitOfTime::Microsecond);
}

inline const plTimestamp plTimestamp::operator-(const plTime& timeSpan) const
{
  PLASMA_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return plTimestamp::MakeFromInt(m_iTimestamp - (plInt64)timeSpan.GetMicroseconds(), plSIUnitOfTime::Microsecond);
}

inline const plTimestamp operator+(const plTime& timeSpan, const plTimestamp& timestamp)
{
  PLASMA_ASSERT_DEBUG(timestamp.IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return plTimestamp::MakeFromInt(timestamp.GetInt64(plSIUnitOfTime::Microsecond) + (plInt64)timeSpan.GetMicroseconds(), plSIUnitOfTime::Microsecond);
}



inline plUInt32 plDateTime::GetYear() const
{
  return m_iYear;
}

inline void plDateTime::SetYear(plInt16 iYear)
{
  m_iYear = iYear;
}

inline plUInt8 plDateTime::GetMonth() const
{
  return m_uiMonth;
}

inline void plDateTime::SetMonth(plUInt8 uiMonth)
{
  PLASMA_ASSERT_DEBUG(uiMonth >= 1 && uiMonth <= 12, "Invalid month value");
  m_uiMonth = uiMonth;
}

inline plUInt8 plDateTime::GetDay() const
{
  return m_uiDay;
}

inline void plDateTime::SetDay(plUInt8 uiDay)
{
  PLASMA_ASSERT_DEBUG(uiDay >= 1 && uiDay <= 31, "Invalid day value");
  m_uiDay = uiDay;
}

inline plUInt8 plDateTime::GetDayOfWeek() const
{
  return m_uiDayOfWeek;
}

inline void plDateTime::SetDayOfWeek(plUInt8 uiDayOfWeek)
{
  PLASMA_ASSERT_DEBUG(uiDayOfWeek <= 6, "Invalid day of week value");
  m_uiDayOfWeek = uiDayOfWeek;
}

inline plUInt8 plDateTime::GetHour() const
{
  return m_uiHour;
}

inline void plDateTime::SetHour(plUInt8 uiHour)
{
  PLASMA_ASSERT_DEBUG(uiHour <= 23, "Invalid hour value");
  m_uiHour = uiHour;
}

inline plUInt8 plDateTime::GetMinute() const
{
  return m_uiMinute;
}

inline void plDateTime::SetMinute(plUInt8 uiMinute)
{
  PLASMA_ASSERT_DEBUG(uiMinute <= 59, "Invalid minute value");
  m_uiMinute = uiMinute;
}

inline plUInt8 plDateTime::GetSecond() const
{
  return m_uiSecond;
}

inline void plDateTime::SetSecond(plUInt8 uiSecond)
{
  PLASMA_ASSERT_DEBUG(uiSecond <= 59, "Invalid second value");
  m_uiSecond = uiSecond;
}

inline plUInt32 plDateTime::GetMicroseconds() const
{
  return m_uiMicroseconds;
}

inline void plDateTime::SetMicroseconds(plUInt32 uiMicroSeconds)
{
  PLASMA_ASSERT_DEBUG(uiMicroSeconds <= 999999u, "Invalid micro-second value");
  m_uiMicroseconds = uiMicroSeconds;
}

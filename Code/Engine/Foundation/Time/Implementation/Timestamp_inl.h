#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Math.h>

static const plInt64 PLASMA_INVALID_TIME_STAMP = 0x7FFFFFFFFFFFFFFFLL;

inline plTimestamp::plTimestamp()
{
  Invalidate();
}

inline plTimestamp::plTimestamp(plInt64 iTimeValue, plSIUnitOfTime::Enum unitOfTime)
{
  SetInt64(iTimeValue, unitOfTime);
}

inline void plTimestamp::Invalidate()
{
  m_iTimestamp = PLASMA_INVALID_TIME_STAMP;
}

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
  return plTime::Microseconds((double)(m_iTimestamp - other.m_iTimestamp));
}

inline const plTimestamp plTimestamp::operator+(const plTime& timeSpan) const
{
  PLASMA_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return plTimestamp(m_iTimestamp + (plInt64)timeSpan.GetMicroseconds(), plSIUnitOfTime::Microsecond);
}

inline const plTimestamp plTimestamp::operator-(const plTime& timeSpan) const
{
  PLASMA_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return plTimestamp(m_iTimestamp - (plInt64)timeSpan.GetMicroseconds(), plSIUnitOfTime::Microsecond);
}

inline const plTimestamp operator+(const plTime& timeSpan, const plTimestamp& timestamp)
{
  PLASMA_ASSERT_DEBUG(timestamp.IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return plTimestamp(timestamp.GetInt64(plSIUnitOfTime::Microsecond) + (plInt64)timeSpan.GetMicroseconds(), plSIUnitOfTime::Microsecond);
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
  m_uiMonth = plMath::Clamp<plUInt8>(uiMonth, 1, 12);
}

inline plUInt8 plDateTime::GetDay() const
{
  return m_uiDay;
}

inline void plDateTime::SetDay(plUInt8 uiDay)
{
  m_uiDay = plMath::Clamp<plUInt8>(uiDay, 1u, 31u);
}

inline plUInt8 plDateTime::GetDayOfWeek() const
{
  return m_uiDayOfWeek;
}

inline void plDateTime::SetDayOfWeek(plUInt8 uiDayOfWeek)
{
  m_uiDayOfWeek = plMath::Clamp<plUInt8>(uiDayOfWeek, 0u, 6u);
}

inline plUInt8 plDateTime::GetHour() const
{
  return m_uiHour;
}

inline void plDateTime::SetHour(plUInt8 uiHour)
{
  m_uiHour = plMath::Clamp<plUInt8>(uiHour, 0u, 23u);
}

inline plUInt8 plDateTime::GetMinute() const
{
  return m_uiMinute;
}

inline void plDateTime::SetMinute(plUInt8 uiMinute)
{
  m_uiMinute = plMath::Clamp<plUInt8>(uiMinute, 0u, 59u);
}

inline plUInt8 plDateTime::GetSecond() const
{
  return m_uiSecond;
}

inline void plDateTime::SetSecond(plUInt8 uiSecond)
{
  m_uiSecond = plMath::Clamp<plUInt8>(uiSecond, 0u, 59u);
}

inline plUInt32 plDateTime::GetMicroseconds() const
{
  return m_uiMicroseconds;
}

inline void plDateTime::SetMicroseconds(plUInt32 uiMicroSeconds)
{
  m_uiMicroseconds = plMath::Clamp<plUInt32>(uiMicroSeconds, 0u, 999999u);
}

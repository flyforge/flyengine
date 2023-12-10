#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/OSX/ScopedCFRef.h>

#include <CoreFoundation/CFCalendar.h>
#include <CoreFoundation/CoreFoundation.h>

const plTimestamp plTimestamp::CurrentTimestamp()
{
  timeval currentTime;
  gettimeofday(&currentTime, nullptr);

  return plTimestamp::MakeFromInt(currentTime.tv_sec * 1000000LL + currentTime.tv_usec, plSIUnitOfTime::Microsecond);
}

const plTimestamp plDateTime::GetTimestamp() const
{
  plScopedCFRef<CFTimeZoneRef> timplone(CFTimeZoneCreateWithTimeIntervalFromGMT(kCFAllocatorDefault, 0));
  plScopedCFRef<CFCalendarRef> calendar(CFCalendarCreateWithIdentifier(kCFAllocatorSystemDefault, kCFGregorianCalendar));
  CFCalendarSetTimeZone(calendar, timplone);

  int year = m_iYear, month = m_uiMonth, day = m_uiDay, hour = m_uiHour, minute = m_uiMinute, second = m_uiSecond;

  // Validate the year against the valid range of the calendar
  {
    auto yearMin = CFCalendarGetMinimumRangeOfUnit(calendar, kCFCalendarUnitYear), yearMax = CFCalendarGetMaximumRangeOfUnit(calendar, kCFCalendarUnitYear);

    if (year < yearMin.location || year > yearMax.length)
    {
      return plTimestamp::MakeInvalid();
    }
  }

  // Validate the month against the valid range of the calendar
  {
    auto monthMin = CFCalendarGetMinimumRangeOfUnit(calendar, kCFCalendarUnitMonth), monthMax = CFCalendarGetMaximumRangeOfUnit(calendar, kCFCalendarUnitMonth);

    if (month < monthMin.location || month > monthMax.length)
    {
      return plTimestamp::MakeInvalid();
    }
  }

  // Validate the day against the valid range of the calendar
  {
    auto dayMin = CFCalendarGetMinimumRangeOfUnit(calendar, kCFCalendarUnitDay), dayMax = CFCalendarGetMaximumRangeOfUnit(calendar, kCFCalendarUnitDay);

    if (day < dayMin.location || day > dayMax.length)
    {
      return plTimestamp::MakeInvalid();
    }
  }

  CFAbsoluteTime absTime;
  if (CFCalendarComposeAbsoluteTime(calendar, &absTime, "yMdHms", year, month, day, hour, minute, second) == FALSE)
  {
    return plTimestamp::MakeInvalid();
  }

  return plTimestamp::MakeFromInt(static_cast<plInt64>((absTime + kCFAbsoluteTimeIntervalSince1970) * 1000000.0), plSIUnitOfTime::Microsecond);
}

plResult plDateTime::SetFromTimestamp(plTimestamp timestamp)
{
  // Round the microseconds to the full second so that we can reconstruct the right date / time afterwards
  plInt64 us = timestamp.GetInt64(plSIUnitOfTime::Microsecond);
  plInt64 microseconds = us % (1000 * 1000);

  CFAbsoluteTime at = (static_cast<CFAbsoluteTime>((us - microseconds) / 1000000.0)) - kCFAbsoluteTimeIntervalSince1970;

  plScopedCFRef<CFTimeZoneRef> timplone(CFTimeZoneCreateWithTimeIntervalFromGMT(kCFAllocatorDefault, 0));
  plScopedCFRef<CFCalendarRef> calendar(CFCalendarCreateWithIdentifier(kCFAllocatorSystemDefault, kCFGregorianCalendar));
  CFCalendarSetTimeZone(calendar, timplone);

  int year, month, day, dayOfWeek, hour, minute, second;

  if (CFCalendarDecomposeAbsoluteTime(calendar, at, "yMdHmsE", &year, &month, &day, &hour, &minute, &second, &dayOfWeek) == FALSE)
  {
    return PLASMA_FAILURE;
  }

  m_iYear = (plInt16)year;
  m_uiMonth = (plUInt8)month;
  m_uiDay = (plUInt8)day;
  m_uiDayOfWeek = (plUInt8)(dayOfWeek - 1);
  m_uiHour = (plUInt8)hour;
  m_uiMinute = (plUInt8)minute;
  m_uiSecond = (plUInt8)second;
  m_uiMicroseconds = (plUInt32)microseconds;
  return PLASMA_SUCCESS;
}

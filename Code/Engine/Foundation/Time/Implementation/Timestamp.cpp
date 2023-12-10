#include <Foundation/FoundationPCH.h>

#include <Foundation/Time/Timestamp.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plTimestamp, plNoBase, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("time", m_iTimestamp),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plInt64 plTimestamp::GetInt64(plSIUnitOfTime::Enum unitOfTime) const
{
  PLASMA_ASSERT_DEV(IsValid(), "Can't retrieve timestamp of invalid values!");
  PLASMA_ASSERT_DEV(unitOfTime >= plSIUnitOfTime::Nanosecond && unitOfTime <= plSIUnitOfTime::Second, "Invalid plSIUnitOfTime value ({0})", unitOfTime);

  switch (unitOfTime)
  {
    case plSIUnitOfTime::Nanosecond:
      return m_iTimestamp * 1000LL;
    case plSIUnitOfTime::Microsecond:
      return m_iTimestamp;
    case plSIUnitOfTime::Millisecond:
      return m_iTimestamp / 1000LL;
    case plSIUnitOfTime::Second:
      return m_iTimestamp / 1000000LL;
  }
  return PLASMA_INVALID_TIME_STAMP;
}

plTimestamp plTimestamp::MakeFromInt(plInt64 iTimeValue, plSIUnitOfTime::Enum unitOfTime)
{
  PLASMA_ASSERT_DEV(unitOfTime >= plSIUnitOfTime::Nanosecond && unitOfTime <= plSIUnitOfTime::Second, "Invalid plSIUnitOfTime value ({0})", unitOfTime);

  plTimestamp ts;

  switch (unitOfTime)
  {
    case plSIUnitOfTime::Nanosecond:
      ts.m_iTimestamp = iTimeValue / 1000LL;
      break;
    case plSIUnitOfTime::Microsecond:
      ts.m_iTimestamp = iTimeValue;
      break;
    case plSIUnitOfTime::Millisecond:
      ts.m_iTimestamp = iTimeValue * 1000LL;
      break;
    case plSIUnitOfTime::Second:
      ts.m_iTimestamp = iTimeValue * 1000000LL;
      break;

      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return ts;
}

bool plTimestamp::Compare(const plTimestamp& rhs, CompareMode::Enum mode) const
{
  switch (mode)
  {
    case CompareMode::FileTimeEqual:
      // Resolution of seconds until all platforms are tuned to milliseconds.
      return (m_iTimestamp / 1000000LL) == (rhs.m_iTimestamp / 1000000LL);

    case CompareMode::Identical:
      return m_iTimestamp == rhs.m_iTimestamp;

    case CompareMode::Newer:
      // Resolution of seconds until all platforms are tuned to milliseconds.
      return (m_iTimestamp / 1000000LL) > (rhs.m_iTimestamp / 1000000LL);
  }

  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return false;
}

plDateTime::plDateTime() = default;
plDateTime::~plDateTime() = default;

plDateTime plDateTime::MakeFromTimestamp(plTimestamp timestamp)
{
  plDateTime res;
  res.SetFromTimestamp(timestamp).AssertSuccess("Invalid timestamp");
  return res;
}

bool plDateTime::IsValid() const
{
  if (m_uiMonth <= 0 || m_uiMonth > 12)
    return false;

  if (m_uiDay <= 0 || m_uiDay > 31)
    return false;

  if (m_uiDayOfWeek > 6)
    return false;

  if (m_uiHour > 23)
    return false;

  if (m_uiMinute > 59)
    return false;

  if (m_uiSecond > 59)
    return false;

  return true;
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plDateTime& arg)
{
  plStringUtils::snprintf(szTmp, uiLength, "%04u-%02u-%02u_%02u-%02u-%02u-%03u", arg.GetYear(), arg.GetMonth(), arg.GetDay(), arg.GetHour(),
    arg.GetMinute(), arg.GetSecond(), arg.GetMicroseconds() / 1000);

  return szTmp;
}

namespace
{
  // This implementation chooses a 3-character-long short name for each of the twelve months
  // for consistency reasons. Mind, that other, potentially more widely-spread stylist
  // alternatives may exist.
  const char* GetMonthShortName(const plDateTime& dateTime)
  {
    switch (dateTime.GetMonth())
    {
      case 1:
        return "Jan";
      case 2:
        return "Feb";
      case 3:
        return "Mar";
      case 4:
        return "Apr";
      case 5:
        return "May";
      case 6:
        return "Jun";
      case 7:
        return "Jul";
      case 8:
        return "Aug";
      case 9:
        return "Sep";
      case 10:
        return "Oct";
      case 11:
        return "Nov";
      case 12:
        return "Dec";
      default:
        PLASMA_ASSERT_DEV(false, "Unknown month.");
        return "Unknown Month";
    }
  }

  // This implementation chooses a 3-character-long short name for each of the seven days
  // of the week for consistency reasons. Mind, that other, potentially more widely-spread
  // stylistic alternatives may exist.
  const char* GetDayOfWeekShortName(const plDateTime& dateTime)
  {
    switch (dateTime.GetDayOfWeek())
    {
      case 0:
        return "Sun";
      case 1:
        return "Mon";
      case 2:
        return "Tue";
      case 3:
        return "Wed";
      case 4:
        return "Thu";
      case 5:
        return "Fri";
      case 6:
        return "Sat";
      default:
        PLASMA_ASSERT_DEV(false, "Unknown day of week.");
        return "Unknown Day of Week";
    }
  }
} // namespace

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgDateTime& arg)
{
  const plDateTime& dateTime = arg.m_Value;

  plUInt32 offset = 0;

  if ((arg.m_uiFormattingFlags & plArgDateTime::ShowDate) == plArgDateTime::ShowDate)
  {
    if ((arg.m_uiFormattingFlags & plArgDateTime::TextualDate) == plArgDateTime::TextualDate)
    {
      offset += plStringUtils::snprintf(
        szTmp + offset, uiLength - offset, "%04u %s %02u", dateTime.GetYear(), ::GetMonthShortName(dateTime), dateTime.GetDay());
    }
    else
    {
      offset +=
        plStringUtils::snprintf(szTmp + offset, uiLength - offset, "%04u-%02u-%02u", dateTime.GetYear(), dateTime.GetMonth(), dateTime.GetDay());
    }
  }

  if ((arg.m_uiFormattingFlags & plArgDateTime::ShowWeekday) == plArgDateTime::ShowWeekday)
  {
    // add a space
    if (offset != 0)
    {
      szTmp[offset] = ' ';
      ++offset;
      szTmp[offset] = '\0';
    }

    offset += plStringUtils::snprintf(szTmp + offset, uiLength - offset, "(%s)", ::GetDayOfWeekShortName(dateTime));
  }

  if ((arg.m_uiFormattingFlags & plArgDateTime::ShowTime) == plArgDateTime::ShowTime)
  {
    // add a space
    if (offset != 0)
    {
      szTmp[offset] = ' ';
      szTmp[offset + 1] = '-';
      szTmp[offset + 2] = ' ';
      szTmp[offset + 3] = '\0';
      offset += 3;
    }

    offset += plStringUtils::snprintf(szTmp + offset, uiLength - offset, "%02u:%02u", dateTime.GetHour(), dateTime.GetMinute());

    if ((arg.m_uiFormattingFlags & plArgDateTime::ShowSeconds) == plArgDateTime::ShowSeconds)
    {
      offset += plStringUtils::snprintf(szTmp + offset, uiLength - offset, ":%02u", dateTime.GetSecond());
    }

    if ((arg.m_uiFormattingFlags & plArgDateTime::ShowMilliseconds) == plArgDateTime::ShowMilliseconds)
    {
      offset += plStringUtils::snprintf(szTmp + offset, uiLength - offset, ".%03u", dateTime.GetMicroseconds() / 1000);
    }

    if ((arg.m_uiFormattingFlags & plArgDateTime::ShowTimeZone) == plArgDateTime::ShowTimeZone)
    {
      plStringUtils::snprintf(szTmp + offset, uiLength - offset, " (UTC)");
    }
  }

  return szTmp;
}

// Include inline file
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Time/Implementation/Win/Timestamp_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX)
#  include <Foundation/Time/Implementation/OSX/Timestamp_osx.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Time/Implementation/Android/Timestamp_android.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
#  include <Foundation/Time/Implementation/Posix/Timestamp_posix.h>
#else
#  error "Time functions are not implemented on current platform"
#endif

PLASMA_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Timestamp);

#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Time.h>

struct plSIUnitOfTime
{
  enum Enum
  {
    Nanosecond,  ///< SI-unit of time (10^-9 second)
    Microsecond, ///< SI-unit of time (10^-6 second)
    Millisecond, ///< SI-unit of time (10^-3 second)
    Second,      ///< SI-unit of time (base unit)
  };
};

/// \brief The timestamp class encapsulates a date in time as microseconds since Unix epoch.
///
/// The value is represented by an plInt64 and allows storing time stamps from roughly
/// -291030 BC to 293970 AC.
/// Use this class to efficiently store a timestamp that is valid across platforms.
class PLASMA_FOUNDATION_DLL plTimestamp
{
public:
  struct CompareMode
  {
    enum Enum
    {
      FileTimeEqual, ///< Uses a resolution that guarantees that a file's timestamp is considered equal on all platforms.
      Identical,     ///< Uses maximal stored resolution.
      Newer,         ///< Just compares values and returns true if the left-hand side is larger than the right hand side
    };
  };
  /// \brief  Returns the current timestamp. Returned value will always be valid.
  ///
  /// Depending on the platform the precision varies between seconds and nanoseconds.
  static const plTimestamp CurrentTimestamp(); // [tested]

  PLASMA_DECLARE_POD_TYPE();

  // *** Constructors ***
public:
  /// \brief Creates an invalidated timestamp.
  plTimestamp(); // [tested]

  /// \brief Creates an new timestamp with the given time in the given unit of time since Unix epoch.
  plTimestamp(plInt64 iTimeValue, plSIUnitOfTime::Enum unitOfTime); // [tested]

  // *** Public Functions ***
public:
  /// \brief Invalidates the timestamp.
  void Invalidate(); // [tested]

  /// \brief Returns whether the timestamp is valid.
  bool IsValid() const; // [tested]

  /// \brief Returns the number of 'unitOfTime' since Unix epoch.
  plInt64 GetInt64(plSIUnitOfTime::Enum unitOfTime) const; // [tested]

  /// \brief Sets the timestamp as 'iTimeValue' in 'unitOfTime' since Unix epoch.
  void SetInt64(plInt64 iTimeValue, plSIUnitOfTime::Enum unitOfTime); // [tested]

  /// \brief Returns whether this timestamp is considered equal to 'rhs' in the given mode.
  ///
  /// Use CompareMode::FileTime when working with file time stamps across platforms.
  /// It will use the lowest resolution supported by all platforms to make sure the
  /// timestamp of a file is considered equal regardless on which platform it was retrieved.
  bool Compare(const plTimestamp& rhs, CompareMode::Enum mode) const; // [tested]

  // *** Operators ***
public:
  /// \brief Adds the time value of "timeSpan" to this data value.
  void operator+=(const plTime& timeSpan); // [tested]

  /// \brief Subtracts the time value of "timeSpan" from this date value.
  void operator-=(const plTime& timeSpan); // [tested]

  /// \brief Returns the time span between this timestamp and "other".
  const plTime operator-(const plTimestamp& other) const; // [tested]

  /// \brief Returns a timestamp that is "timeSpan" further into the future from this timestamp.
  const plTimestamp operator+(const plTime& timeSpan) const; // [tested]

  /// \brief Returns a timestamp that is "timeSpan" further into the past from this timestamp.
  const plTimestamp operator-(const plTime& timeSpan) const; // [tested]


private:
  PLASMA_ALLOW_PRIVATE_PROPERTIES(plTimestamp);
  /// \brief The date is stored as microseconds since Unix epoch.
  plInt64 m_iTimestamp;
};

/// \brief Returns a timestamp that is "timeSpan" further into the future from "timestamp".
const plTimestamp operator+(plTime& ref_timeSpan, const plTimestamp& timestamp);

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_FOUNDATION_DLL, plTimestamp);

/// \brief The plDateTime class can be used to convert plTimestamp into a human readable form.
///
/// Note: As plTimestamp is microseconds since Unix epoch, the values in this class will always be
/// in UTC.
class PLASMA_FOUNDATION_DLL plDateTime
{
public:
  /// \brief Creates an empty date time instance with an invalid date.
  ///
  /// Day, Month and Year will be invalid and must be set.
  plDateTime(); // [tested]

  /// \brief Creates a date time instance from the given timestamp.
  plDateTime(plTimestamp timestamp); // [tested]

  /// \brief Converts this instance' values into a plTimestamp.
  ///
  /// The conversion is done via the OS and can fail for values that are outside the supported range.
  /// In this case, the returned value will be invalid. Anything after 1970 and before the
  /// not so distant future should be safe.
  const plTimestamp GetTimestamp() const; // [tested]

  /// \brief Sets this instance to the given timestamp.
  ///
  /// The conversion is done via the OS and will fail for invalid dates and values outside the supported range,
  /// in which case false will be returned.
  /// Anything after 1970 and before the not so distant future should be safe.
  bool SetTimestamp(plTimestamp timestamp); // [tested]

  // *** Accessors ***
public:
  /// \brief Returns the currently set year.
  plUInt32 GetYear() const; // [tested]

  /// \brief Sets the year to the given value.
  void SetYear(plInt16 iYear); // [tested]

  /// \brief Returns the currently set month.
  plUInt8 GetMonth() const; // [tested]

  /// \brief Sets the month to the given value, will be clamped to valid range [1, 12].
  void SetMonth(plUInt8 uiMonth); // [tested]

  /// \brief Returns the currently set day.
  plUInt8 GetDay() const; // [tested]

  /// \brief Sets the day to the given value, will be clamped to valid range [1, 31].
  void SetDay(plUInt8 uiDay); // [tested]

  /// \brief Returns the currently set day of week.
  plUInt8 GetDayOfWeek() const;

  /// \brief Sets the day of week to the given value, will be clamped to valid range [0, 6].
  void SetDayOfWeek(plUInt8 uiDayOfWeek);

  /// \brief Returns the currently set hour.
  plUInt8 GetHour() const; // [tested]

  /// \brief Sets the hour to the given value, will be clamped to valid range [0, 23].
  void SetHour(plUInt8 uiHour); // [tested]

  /// \brief Returns the currently set minute.
  plUInt8 GetMinute() const; // [tested]

  /// \brief Sets the minute to the given value, will be clamped to valid range [0, 59].
  void SetMinute(plUInt8 uiMinute); // [tested]

  /// \brief Returns the currently set second.
  plUInt8 GetSecond() const; // [tested]

  /// \brief Sets the second to the given value, will be clamped to valid range [0, 59].
  void SetSecond(plUInt8 uiSecond); // [tested]

  /// \brief Returns the currently set microseconds.
  plUInt32 GetMicroseconds() const; // [tested]

  /// \brief Sets the microseconds to the given value, will be clamped to valid range [0, 999999].
  void SetMicroseconds(plUInt32 uiMicroSeconds); // [tested]

private:
  /// \brief The fraction of a second in microseconds of this date [0, 999999].
  plUInt32 m_uiMicroseconds;
  /// \brief The year of this date [-32k, +32k].
  plInt16 m_iYear;
  /// \brief The month of this date [1, 12].
  plUInt8 m_uiMonth;
  /// \brief The day of this date [1, 31].
  plUInt8 m_uiDay;
  /// \brief The day of week of this date [0, 6].
  plUInt8 m_uiDayOfWeek;
  /// \brief The hour of this date [0, 23].
  plUInt8 m_uiHour;
  /// \brief The number of minutes of this date [0, 59].
  plUInt8 m_uiMinute;
  /// \brief The number of seconds of this date [0, 59].
  plUInt8 m_uiSecond;
};

PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plDateTime& arg);

struct plArgDateTime
{
  enum FormattingFlags
  {
    ShowDate = PLASMA_BIT(0),
    TextualDate = ShowDate | PLASMA_BIT(1),
    ShowWeekday = PLASMA_BIT(2),
    ShowTime = PLASMA_BIT(3),
    ShowSeconds = ShowTime | PLASMA_BIT(4),
    ShowMilliseconds = ShowSeconds | PLASMA_BIT(5),
    ShowTimeZone = PLASMA_BIT(6),

    Default = ShowDate | ShowSeconds,
    DefaultTextual = TextualDate | ShowSeconds,
  };

  /// \brief Initialized a formatting object for an plDateTime instance.
  /// \param dateTime The plDateTime instance to format.
  /// \param bUseNames Indicates whether to use names for days of week and months (true)
  ///        or a purely numerical representation (false).
  /// \param bShowTimeZoneIndicator Whether to indicate the timplone of the plDateTime object.
  inline explicit plArgDateTime(const plDateTime& dateTime, plUInt32 uiFormattingFlags = Default)
    : m_Value(dateTime)
    , m_uiFormattingFlags(uiFormattingFlags)
  {
  }

  plDateTime m_Value;
  plUInt32 m_uiFormattingFlags;
};

PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgDateTime& arg);

#include <Foundation/Time/Implementation/Timestamp_inl.h>

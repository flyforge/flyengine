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
class PL_FOUNDATION_DLL plTimestamp
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

  PL_DECLARE_POD_TYPE();

  // *** Constructors ***
public:
  /// \brief Creates an invalidated timestamp.
  plTimestamp(); // [tested]

  /// \brief Returns an invalid timestamp
  [[nodiscard]] static plTimestamp MakeInvalid() { return plTimestamp(); }

  /// \brief Returns a timestamp initialized from 'iTimeValue' in 'unitOfTime' since Unix epoch.
  [[nodiscard]] static plTimestamp MakeFromInt(plInt64 iTimeValue, plSIUnitOfTime::Enum unitOfTime);

  // *** Public Functions ***
public:

  /// \brief Returns whether the timestamp is valid.
  bool IsValid() const; // [tested]

  /// \brief Returns the number of 'unitOfTime' since Unix epoch.
  plInt64 GetInt64(plSIUnitOfTime::Enum unitOfTime) const; // [tested]

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
  static constexpr const plInt64 PL_INVALID_TIME_STAMP = 0x7FFFFFFFFFFFFFFFLL;

  PL_ALLOW_PRIVATE_PROPERTIES(plTimestamp);
  /// \brief The date is stored as microseconds since Unix epoch.
  plInt64 m_iTimestamp = PL_INVALID_TIME_STAMP;
};

/// \brief Returns a timestamp that is "timeSpan" further into the future from "timestamp".
const plTimestamp operator+(plTime& ref_timeSpan, const plTimestamp& timestamp);

PL_DECLARE_REFLECTABLE_TYPE(PL_FOUNDATION_DLL, plTimestamp);

/// \brief The plDateTime class can be used to convert plTimestamp into a human readable form.
///
/// Note: As plTimestamp is microseconds since Unix epoch, the values in this class will always be
/// in UTC.
class PL_FOUNDATION_DLL plDateTime
{
public:
  /// \brief Creates an empty date time instance with an invalid date.
  ///
  /// Day, Month and Year will be invalid and must be set.
  plDateTime(); // [tested]
  ~plDateTime();

  /// \brief Checks whether all values are within valid ranges.
  bool IsValid() const;

  /// \brief Returns a date time that is all zero.
  [[nodiscard]] static plDateTime MakeZero() { return plDateTime(); }

  /// \brief Sets this instance to the given timestamp.
  ///
  /// This calls SetFromTimestamp() internally and asserts that the conversion succeeded.
  /// Use SetFromTimestamp() directly, if you need to be able to react to invalid data.
  [[nodiscard]] static plDateTime MakeFromTimestamp(plTimestamp timestamp);

  /// \brief Converts this instance' values into a plTimestamp.
  ///
  /// The conversion is done via the OS and can fail for values that are outside the supported range.
  /// In this case, the returned value will be invalid. Anything after 1970 and before the
  /// not so distant future should be safe.
  [[nodiscard]] const plTimestamp GetTimestamp() const; // [tested]

  /// \brief Sets this instance to the given timestamp.
  ///
  /// The conversion is done via the OS and will fail for invalid dates and values outside the supported range,
  /// in which case PL_FAILURE will be returned.
  /// Anything after 1970 and before the not so distant future should be safe.
  plResult SetFromTimestamp(plTimestamp timestamp);

  // *** Accessors ***
public:
  /// \brief Returns the currently set year.
  plUInt32 GetYear() const; // [tested]

  /// \brief Sets the year to the given value.
  void SetYear(plInt16 iYear); // [tested]

  /// \brief Returns the currently set month.
  plUInt8 GetMonth() const; // [tested]

  /// \brief Sets the month to the given value. Asserts that the value is in the valid range [1, 12].
  void SetMonth(plUInt8 uiMonth); // [tested]

  /// \brief Returns the currently set day.
  plUInt8 GetDay() const; // [tested]

  /// \brief Sets the day to the given value. Asserts that the value is in the valid range [1, 31].
  void SetDay(plUInt8 uiDay); // [tested]

  /// \brief Returns the currently set day of week.
  plUInt8 GetDayOfWeek() const;

  /// \brief Sets the day of week to the given value. Asserts that the value is in the valid range [0, 6].
  void SetDayOfWeek(plUInt8 uiDayOfWeek);

  /// \brief Returns the currently set hour.
  plUInt8 GetHour() const; // [tested]

  /// \brief Sets the hour to the given value. Asserts that the value is in the valid range [0, 23].
  void SetHour(plUInt8 uiHour); // [tested]

  /// \brief Returns the currently set minute.
  plUInt8 GetMinute() const; // [tested]

  /// \brief Sets the minute to the given value. Asserts that the value is in the valid range [0, 59].
  void SetMinute(plUInt8 uiMinute); // [tested]

  /// \brief Returns the currently set second.
  plUInt8 GetSecond() const; // [tested]

  /// \brief Sets the second to the given value. Asserts that the value is in the valid range [0, 59].
  void SetSecond(plUInt8 uiSecond); // [tested]

  /// \brief Returns the currently set microseconds.
  plUInt32 GetMicroseconds() const; // [tested]

  /// \brief Sets the microseconds to the given value. Asserts that the value is in the valid range [0, 999999].
  void SetMicroseconds(plUInt32 uiMicroSeconds); // [tested]

private:
  /// \brief The fraction of a second in microseconds of this date [0, 999999].
  plUInt32 m_uiMicroseconds = 0;
  /// \brief The year of this date [-32k, +32k].
  plInt16 m_iYear = 0;
  /// \brief The month of this date [1, 12].
  plUInt8 m_uiMonth = 0;
  /// \brief The day of this date [1, 31].
  plUInt8 m_uiDay = 0;
  /// \brief The day of week of this date [0, 6].
  plUInt8 m_uiDayOfWeek = 0;
  /// \brief The hour of this date [0, 23].
  plUInt8 m_uiHour = 0;
  /// \brief The number of minutes of this date [0, 59].
  plUInt8 m_uiMinute = 0;
  /// \brief The number of seconds of this date [0, 59].
  plUInt8 m_uiSecond = 0;
};

PL_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plDateTime& arg);

struct plArgDateTime
{
  enum FormattingFlags
  {
    ShowDate = PL_BIT(0),
    TextualDate = ShowDate | PL_BIT(1),
    ShowWeekday = PL_BIT(2),
    ShowTime = PL_BIT(3),
    ShowSeconds = ShowTime | PL_BIT(4),
    ShowMilliseconds = ShowSeconds | PL_BIT(5),
    ShowTimeZone = PL_BIT(6),

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

PL_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgDateTime& arg);

#include <Foundation/Time/Implementation/Timestamp_inl.h>

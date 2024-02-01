#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/StaticSubSystem.h>

/// \brief The time class encapsulates a double value storing the time in seconds.
///
/// It offers convenient functions to get the time in other units.
/// plTime is a high-precision time using the OS specific high-precision timing functions
/// and may thus be used for profiling as well as simulation code.
struct PL_FOUNDATION_DLL plTime
{
public:
  /// \brief Gets the current time
  static plTime Now(); // [tested]

  /// \brief Creates an instance of plTime that was initialized from nanoseconds.
  [[nodiscard]] PL_ALWAYS_INLINE constexpr static plTime MakeFromNanoseconds(double fNanoseconds) { return plTime(fNanoseconds * 0.000000001); }
  [[nodiscard]] PL_ALWAYS_INLINE constexpr static plTime Nanoseconds(double fNanoseconds) { return plTime(fNanoseconds * 0.000000001); }

  /// \brief Creates an instance of plTime that was initialized from microseconds.
  [[nodiscard]] PL_ALWAYS_INLINE constexpr static plTime MakeFromMicroseconds(double fMicroseconds) { return plTime(fMicroseconds * 0.000001); }
  [[nodiscard]] PL_ALWAYS_INLINE constexpr static plTime Microseconds(double fMicroseconds) { return plTime(fMicroseconds * 0.000001); }

  /// \brief Creates an instance of plTime that was initialized from milliseconds.
  [[nodiscard]] PL_ALWAYS_INLINE constexpr static plTime MakeFromMilliseconds(double fMilliseconds) { return plTime(fMilliseconds * 0.001); }
  [[nodiscard]] PL_ALWAYS_INLINE constexpr static plTime Milliseconds(double fMilliseconds) { return plTime(fMilliseconds * 0.001); }

  /// \brief Creates an instance of plTime that was initialized from seconds.
  [[nodiscard]] PL_ALWAYS_INLINE constexpr static plTime MakeFromSeconds(double fSeconds) { return plTime(fSeconds); }
  [[nodiscard]] PL_ALWAYS_INLINE constexpr static plTime Seconds(double fSeconds) { return plTime(fSeconds); }

  /// \brief Creates an instance of plTime that was initialized from minutes.
  [[nodiscard]] PL_ALWAYS_INLINE constexpr static plTime MakeFromMinutes(double fMinutes) { return plTime(fMinutes * 60); }
  [[nodiscard]] PL_ALWAYS_INLINE constexpr static plTime Minutes(double fMinutes) { return plTime(fMinutes * 60); }

  /// \brief Creates an instance of plTime that was initialized from hours.
  [[nodiscard]] PL_ALWAYS_INLINE constexpr static plTime MakeFromHours(double fHours) { return plTime(fHours * 60 * 60); }
  [[nodiscard]] PL_ALWAYS_INLINE constexpr static plTime Hours(double fHours) { return plTime(fHours * 60 * 60); }

  /// \brief Creates an instance of plTime that was initialized with zero.
  [[nodiscard]] PL_ALWAYS_INLINE constexpr static plTime MakeZero() { return plTime(0.0); }

  PL_DECLARE_POD_TYPE();

  /// \brief The default constructor sets the time to zero.
  PL_ALWAYS_INLINE constexpr plTime() = default;

  /// \brief Returns true if the stored time is exactly zero. That typically means the value was not changed from the default.
  PL_ALWAYS_INLINE constexpr bool IsZero() const { return m_fTime == 0.0; }

  /// \brief Checks for a negative time value.
  PL_ALWAYS_INLINE constexpr bool IsNegative() const { return m_fTime < 0.0; }

  /// \brief Checks for a positive time value. This does not include zero.
  PL_ALWAYS_INLINE constexpr bool IsPositive() const { return m_fTime > 0.0; }

  /// \brief Returns true if the stored time is zero or negative.
  PL_ALWAYS_INLINE constexpr bool IsZeroOrNegative() const { return m_fTime <= 0.0; }

  /// \brief Returns true if the stored time is zero or positive.
  PL_ALWAYS_INLINE constexpr bool IsZeroOrPositive() const { return m_fTime >= 0.0; }

  /// \brief Returns the time as a float value (in seconds).
  ///
  /// Useful for simulation time steps etc.
  /// Please note that it is not recommended to use the float value for long running
  /// time calculations since the precision can deteriorate quickly. (Only use for delta times is recommended)
  constexpr float AsFloatInSeconds() const;

  /// \brief Returns the nanoseconds value
  constexpr double GetNanoseconds() const;

  /// \brief Returns the microseconds value
  constexpr double GetMicroseconds() const;

  /// \brief Returns the milliseconds value
  constexpr double GetMilliseconds() const;

  /// \brief Returns the seconds value.
  constexpr double GetSeconds() const;

  /// \brief Returns the minutes value.
  constexpr double GetMinutes() const;

  /// \brief Returns the hours value.
  constexpr double GetHours() const;

  /// \brief Subtracts the time value of "other" from this instances value.
  constexpr void operator-=(const plTime& other);

  /// \brief Adds the time value of "other" to this instances value.
  constexpr void operator+=(const plTime& other);

  /// \brief Multiplies the time by the given factor
  constexpr void operator*=(double fFactor);

  /// \brief Divides the time by the given factor
  constexpr void operator/=(double fFactor);

  /// \brief Returns the difference: "this instance - other"
  constexpr plTime operator-(const plTime& other) const;

  /// \brief Returns the sum: "this instance + other"
  constexpr plTime operator+(const plTime& other) const;

  constexpr plTime operator-() const;

  constexpr bool operator<(const plTime& rhs) const { return m_fTime < rhs.m_fTime; }
  constexpr bool operator<=(const plTime& rhs) const { return m_fTime <= rhs.m_fTime; }
  constexpr bool operator>(const plTime& rhs) const { return m_fTime > rhs.m_fTime; }
  constexpr bool operator>=(const plTime& rhs) const { return m_fTime >= rhs.m_fTime; }
  constexpr bool operator==(const plTime& rhs) const { return m_fTime == rhs.m_fTime; }
  constexpr bool operator!=(const plTime& rhs) const { return m_fTime != rhs.m_fTime; }

private:
  /// \brief For internal use only.
  constexpr explicit plTime(double fTime);

  /// \brief The time is stored in seconds
  double m_fTime = 0.0;

private:
  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Time);

  static void Initialize();
};

constexpr plTime operator*(plTime t, double f);
constexpr plTime operator*(double f, plTime t);
constexpr plTime operator*(plTime f, plTime t); // not physically correct, but useful (should result in seconds squared)

constexpr plTime operator/(plTime t, double f);
constexpr plTime operator/(double f, plTime t);
constexpr plTime operator/(plTime f, plTime t); // not physically correct, but useful (should result in a value without a unit)


#include <Foundation/Time/Implementation/Time_inl.h>

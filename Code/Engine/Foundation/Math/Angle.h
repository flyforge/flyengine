#pragma once

#include <Foundation/Math/Declarations.h>

/// \brief Float wrapper struct for a safe usage and conversions of angles.
///
/// Uses radian internally. Will <b>not</b> automatically keep its range between 0 degree - 360 degree (0 - 2PI) but you can call NormalizeRange to do
/// so.
class PLASMA_FOUNDATION_DLL plAngle
{
public:
  PLASMA_DECLARE_POD_TYPE();

  /// \brief Returns the constant to multiply with an angle in degree to convert it to radians.
  template <typename Type>
  constexpr static PLASMA_ALWAYS_INLINE Type DegToRadMultiplier(); // [tested]

  /// \brief Returns the constant to multiply with an angle in degree to convert it to radians.
  template <typename Type>
  constexpr static PLASMA_ALWAYS_INLINE Type RadToDegMultiplier(); // [tested]

  /// \brief Converts an angle in degree to radians.
  template <typename Type>
  constexpr static Type DegToRad(Type f); // [tested]

  /// \brief Converts an angle in radians to degree.
  template <typename Type>
  constexpr static Type RadToDeg(Type f); // [tested]

  /// \brief Returns a zero initialized angle. Same as a default constructed object.
  [[nodiscard]] constexpr static plAngle MakeZero() { return plAngle(); }

  /// \brief Creates an instance of plAngle that was initialized from degree. (Performs a conversion)
  [[nodiscard]] constexpr static plAngle MakeFromDegree(float fDegree); // [tested]

  /// \brief Creates an instance of plAngle that was initialized from radian. (No need for any conversion)
  [[nodiscard]] constexpr static plAngle MakeFromRadian(float fRadian); // [tested]

  /// \brief Standard constructor, initializing with 0.
  constexpr plAngle()
    : m_fRadian(0.0f)
  {
  } // [tested]

  /// \brief Returns the degree value. (Performs a conversion)
  constexpr float GetDegree() const; // [tested]

  /// \brief Returns the radian value. (No need for any conversion)
  constexpr float GetRadian() const; // [tested]

  /// \brief Sets the radian value. (No need for any conversion)
  PLASMA_ALWAYS_INLINE void SetRadian(float fRad) { m_fRadian = fRad; };

  /// \brief Brings the angle into the range of 0 degree - 360 degree
  /// \see GetNormalizedRange()
  void NormalizeRange(); // [tested]

  /// \brief Returns an equivalent angle with range between 0 degree - 360 degree
  /// \see NormalizeRange()
  plAngle GetNormalizedRange() const; // [tested]

  /// \brief Computes the smallest angle between the two given angles. The angle will always be a positive value.
  /// \note The two angles must be in the same range. E.g. they should be either normalized or at least the absolute angle between them should not be
  /// more than 180 degree.
  constexpr static plAngle AngleBetween(plAngle a, plAngle b); // [tested]

  /// \brief Equality check with epsilon. Simple check without normalization. 360 degree will equal 0 degree, but 720 will not.
  bool IsEqualSimple(plAngle rhs, plAngle epsilon) const; // [tested]

  /// \brief Equality check with epsilon that uses normalized angles. Will recognize 720 degree == 0 degree.
  bool IsEqualNormalized(plAngle rhs, plAngle epsilon) const; // [tested]

  // unary operators
  constexpr plAngle operator-() const; // [tested]

  // arithmetic operators
  constexpr plAngle operator+(plAngle r) const; // [tested]
  constexpr plAngle operator-(plAngle r) const; // [tested]

  // compound assignment operators
  void operator+=(plAngle r); // [tested]
  void operator-=(plAngle r); // [tested]

  // comparison
  constexpr bool operator==(const plAngle& r) const; // [tested]
  constexpr bool operator!=(const plAngle& r) const; // [tested]

  // At least the < operator is implement to make clamping etc. work
  constexpr bool operator<(const plAngle& r) const;
  constexpr bool operator>(const plAngle& r) const;
  constexpr bool operator<=(const plAngle& r) const;
  constexpr bool operator>=(const plAngle& r) const;

  // Note: relational operators on angles are not really possible - is 0 degree smaller or bigger than 359 degree?

private:
  /// \brief For internal use only.
  constexpr explicit plAngle(float fRadian)
    : m_fRadian(fRadian)
  {
  }

  /// The plRadian value
  float m_fRadian;

  /// Preventing an include circle by defining pi again (annoying, but unlikely to change ;)). Normally you should use plMath::Pi<Type>()
  template <typename Type>
  constexpr static Type Pi();
};

// Mathematical operators with float

/// \brief Returns f times angle a.
constexpr plAngle operator*(plAngle a, float f); // [tested]
/// \brief Returns f times angle a.
constexpr plAngle operator*(float f, plAngle a); // [tested]

/// \brief Returns the angle a divided by f.
constexpr plAngle operator/(plAngle a, float f); // [tested]
/// \brief Returns the fraction of angle a divided by angle b.
constexpr float operator/(plAngle a, plAngle b); // [tested]

#include <Foundation/Math/Implementation/Angle_inl.h>

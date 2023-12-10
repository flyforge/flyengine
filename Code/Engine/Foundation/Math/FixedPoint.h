#pragma once

#include <Foundation/Math/Declarations.h>

/// \brief Implements fixed point arithmetic for fractional values.
///
/// Advantages over float and double are mostly that the computations are entirely integer-based and therefore
/// have a predictable (i.e. deterministic) result, independent from floating point settings, SSE support and
/// differences among CPUs.
/// Additionally fixed point arithmetic should be quite fast, compare to traditional floating point arithmetic
/// (not comparing it to SSE though).
/// With the template argument 'DecimalBits' you can specify how many bits are used for the fractional part.
/// I.e. a simple integer has zero DecimalBits. For a precision of about 1/1000 you need at least 10 DecimalBits
/// (1 << 10) == 1024.
/// Conversion between integer and fixed point is very fast (a shift), in contrast to float/int conversion.
///
/// If you are using plFixedPoint to get guaranteed deterministic behavior, you should minimize the usage of
/// plFixedPoint <-> float conversions. You can set plFixedPoint variables from float constants, but you should
/// never put data into plFixedPoint variables that was computed using floating point arithmetic (even if the
/// computations are simple and look harmless). Instead do all those computations with plFixedPoint variables.
template <plUInt8 DecimalBits>
class plFixedPoint
{
public:
  /// \brief Default constructor does not do any initialization.
  PLASMA_ALWAYS_INLINE plFixedPoint() = default; // [tested]

  /// \brief Construct from an integer.
  /* implicit */ plFixedPoint(plInt32 iIntVal) { *this = iIntVal; } // [tested]

  /// \brief Construct from a float.
  /* implicit */ plFixedPoint(float fVal) { *this = fVal; } // [tested]

  /// \brief Construct from a double.
  /* implicit */ plFixedPoint(double fVal) { *this = fVal; } // [tested]

  /// \brief Assignment from an integer.
  const plFixedPoint<DecimalBits>& operator=(plInt32 iVal); // [tested]

  /// \brief Assignment from a float.
  const plFixedPoint<DecimalBits>& operator=(float fVal); // [tested]

  /// \brief Assignment from a double.
  const plFixedPoint<DecimalBits>& operator=(double fVal); // [tested]

  /// \brief Implicit conversion to int (the fractional part is dropped).
  plInt32 ToInt() const; // [tested]

  /// \brief Implicit conversion to float.
  float ToFloat() const; // [tested]

  /// \brief Implicit conversion to double.
  double ToDouble() const; // [tested]

  /// \brief 'Equality' comparison.
  bool operator==(const plFixedPoint<DecimalBits>& rhs) const { return m_iValue == rhs.m_iValue; } // [tested]

  /// \brief 'Inequality' comparison.
  bool operator!=(const plFixedPoint<DecimalBits>& rhs) const { return m_iValue != rhs.m_iValue; } // [tested]

  /// \brief 'Less than' comparison.
  bool operator<(const plFixedPoint<DecimalBits>& rhs) const { return m_iValue < rhs.m_iValue; } // [tested]

  /// \brief 'Greater than' comparison.
  bool operator>(const plFixedPoint<DecimalBits>& rhs) const { return m_iValue > rhs.m_iValue; } // [tested]

  /// \brief 'Less than or equal' comparison.
  bool operator<=(const plFixedPoint<DecimalBits>& rhs) const { return m_iValue <= rhs.m_iValue; } // [tested]

  /// \brief 'Greater than or equal' comparison.
  bool operator>=(const plFixedPoint<DecimalBits>& rhs) const { return m_iValue >= rhs.m_iValue; } // [tested]


  const plFixedPoint<DecimalBits> operator-() const { return plFixedPoint<DecimalBits>(-m_iValue, true); }

  /// \brief += operator
  void operator+=(const plFixedPoint<DecimalBits>& rhs) { m_iValue += rhs.m_iValue; } // [tested]

  /// \brief -= operator
  void operator-=(const plFixedPoint<DecimalBits>& rhs) { m_iValue -= rhs.m_iValue; } // [tested]

  /// \brief *= operator
  void operator*=(const plFixedPoint<DecimalBits>& rhs); // [tested]

  /// \brief /= operator
  void operator/=(const plFixedPoint<DecimalBits>& rhs); // [tested]

  /// \brief *= operator with integers (more efficient)
  void operator*=(plInt32 rhs) { m_iValue *= rhs; } // [tested]

  /// \brief /= operator with integers (more efficient)
  void operator/=(plInt32 rhs) { m_iValue /= rhs; } // [tested]

  /// \brief Returns the underlying integer value. Mostly useful for serialization (or tests).
  plInt32 GetRawValue() const { return m_iValue; }

  /// \brief Sets the underlying integer value. Mostly useful for serialization (or tests).
  void SetRawValue(plInt32 iVal) { m_iValue = iVal; }

private:
  plInt32 m_iValue;
};

template <plUInt8 DecimalBits>
float ToFloat(plFixedPoint<DecimalBits> f)
{
  return f.ToFloat();
}

// Additional operators:
// plFixedPoint operator+ (plFixedPoint, plFixedPoint); // [tested]
// plFixedPoint operator- (plFixedPoint, plFixedPoint); // [tested]
// plFixedPoint operator* (plFixedPoint, plFixedPoint); // [tested]
// plFixedPoint operator/ (plFixedPoint, plFixedPoint); // [tested]
// plFixedPoint operator* (int, plFixedPoint); // [tested]
// plFixedPoint operator* (plFixedPoint, int); // [tested]
// plFixedPoint operator/ (plFixedPoint, int); // [tested]

#include <Foundation/Math/Implementation/FixedPoint_inl.h>

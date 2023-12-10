#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/SimdMath/SimdTypes.h>

class PLASMA_FOUNDATION_DLL plSimdFloat
{
public:
  PLASMA_DECLARE_POD_TYPE();

  /// \brief Default constructor, leaves the data uninitialized.
  plSimdFloat(); // [tested]

  /// \brief Constructs from a given float.
  plSimdFloat(float f); // [tested]

  /// \brief Constructs from a given integer.
  plSimdFloat(plInt32 i); // [tested]

  /// \brief Constructs from a given integer.
  plSimdFloat(plUInt32 i); // [tested]

  /// \brief Constructs from given angle.
  plSimdFloat(plAngle a); // [tested]

  /// \brief Constructs from the internal implementation type.
  plSimdFloat(plInternal::QuadFloat v); // [tested]

  /// \brief Returns the stored number as a standard float.
  operator float() const; // [tested]

  /// \brief Creates an plSimdFloat that is initialized to zero.
  [[nodiscard]] static plSimdFloat MakeZero(); // [tested]

  /// \brief Creates an plSimdFloat that is initialized to Not-A-Number (NaN).
  [[nodiscard]] static plSimdFloat MakeNaN(); // [tested]

public:
  plSimdFloat operator+(const plSimdFloat& f) const; // [tested]
  plSimdFloat operator-(const plSimdFloat& f) const; // [tested]
  plSimdFloat operator*(const plSimdFloat& f) const; // [tested]
  plSimdFloat operator/(const plSimdFloat& f) const; // [tested]

  plSimdFloat& operator+=(const plSimdFloat& f); // [tested]
  plSimdFloat& operator-=(const plSimdFloat& f); // [tested]
  plSimdFloat& operator*=(const plSimdFloat& f); // [tested]
  plSimdFloat& operator/=(const plSimdFloat& f); // [tested]

  bool IsEqual(const plSimdFloat& rhs, const plSimdFloat& fEpsilon) const;

  bool operator==(const plSimdFloat& f) const; // [tested]
  bool operator!=(const plSimdFloat& f) const; // [tested]
  bool operator>(const plSimdFloat& f) const;  // [tested]
  bool operator>=(const plSimdFloat& f) const; // [tested]
  bool operator<(const plSimdFloat& f) const;  // [tested]
  bool operator<=(const plSimdFloat& f) const; // [tested]

  bool operator==(float f) const; // [tested]
  bool operator!=(float f) const; // [tested]
  bool operator>(float f) const;  // [tested]
  bool operator>=(float f) const; // [tested]
  bool operator<(float f) const;  // [tested]
  bool operator<=(float f) const; // [tested]

  template <plMathAcc::Enum acc = plMathAcc::FULL>
  plSimdFloat GetReciprocal() const; // [tested]

  template <plMathAcc::Enum acc = plMathAcc::FULL>
  plSimdFloat GetSqrt() const; // [tested]

  template <plMathAcc::Enum acc = plMathAcc::FULL>
  plSimdFloat GetInvSqrt() const; // [tested]

  [[nodiscard]] plSimdFloat Max(const plSimdFloat& f) const; // [tested]
  [[nodiscard]] plSimdFloat Min(const plSimdFloat& f) const; // [tested]
  [[nodiscard]] plSimdFloat Abs() const;                     // [tested]

public:
  plInternal::QuadFloat m_v;
};

#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEFloat_inl.h>
#elif PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUFloat_inl.h>
#elif PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONFloat_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif

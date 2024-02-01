#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

/// \brief A SIMD 4-component vector class of unsigned 32b integers
class PL_FOUNDATION_DLL plSimdVec4u
{
public:
  PL_DECLARE_POD_TYPE();

  plSimdVec4u(); // [tested]

  explicit plSimdVec4u(plUInt32 uiXyzw); // [tested]

  plSimdVec4u(plUInt32 x, plUInt32 y, plUInt32 z, plUInt32 w = 1); // [tested]

  plSimdVec4u(plInternal::QuadUInt v); // [tested]

  /// \brief Creates an plSimdVec4u that is initialized to zero.
  [[nodiscard]] static plSimdVec4u MakeZero(); // [tested]

  void Set(plUInt32 uiXyzw); // [tested]

  void Set(plUInt32 x, plUInt32 y, plUInt32 z, plUInt32 w); // [tested]

  void SetZero(); // [tested]

public:
  explicit plSimdVec4u(const plSimdVec4i& i); // [tested]

public:
  plSimdVec4f ToFloat() const; // [tested]

  [[nodiscard]] static plSimdVec4u Truncate(const plSimdVec4f& f); // [tested]

public:
  template <int N>
  plUInt32 GetComponent() const; // [tested]

  plUInt32 x() const; // [tested]
  plUInt32 y() const; // [tested]
  plUInt32 z() const; // [tested]
  plUInt32 w() const; // [tested]

  template <plSwizzle::Enum s>
  plSimdVec4u Get() const; // [tested]

public:
  [[nodiscard]] plSimdVec4u operator+(const plSimdVec4u& v) const; // [tested]
  [[nodiscard]] plSimdVec4u operator-(const plSimdVec4u& v) const; // [tested]

  [[nodiscard]] plSimdVec4u CompMul(const plSimdVec4u& v) const; // [tested]

  [[nodiscard]] plSimdVec4u operator|(const plSimdVec4u& v) const; // [tested]
  [[nodiscard]] plSimdVec4u operator&(const plSimdVec4u& v) const; // [tested]
  [[nodiscard]] plSimdVec4u operator^(const plSimdVec4u& v) const; // [tested]
  [[nodiscard]] plSimdVec4u operator~() const;                     // [tested]

  [[nodiscard]] plSimdVec4u operator<<(plUInt32 uiShift) const; // [tested]
  [[nodiscard]] plSimdVec4u operator>>(plUInt32 uiShift) const; // [tested]

  plSimdVec4u& operator+=(const plSimdVec4u& v); // [tested]
  plSimdVec4u& operator-=(const plSimdVec4u& v); // [tested]

  plSimdVec4u& operator|=(const plSimdVec4u& v); // [tested]
  plSimdVec4u& operator&=(const plSimdVec4u& v); // [tested]
  plSimdVec4u& operator^=(const plSimdVec4u& v); // [tested]

  plSimdVec4u& operator<<=(plUInt32 uiShift); // [tested]
  plSimdVec4u& operator>>=(plUInt32 uiShift); // [tested]

  [[nodiscard]] plSimdVec4u CompMin(const plSimdVec4u& v) const; // [tested]
  [[nodiscard]] plSimdVec4u CompMax(const plSimdVec4u& v) const; // [tested]

  plSimdVec4b operator==(const plSimdVec4u& v) const; // [tested]
  plSimdVec4b operator!=(const plSimdVec4u& v) const; // [tested]
  plSimdVec4b operator<=(const plSimdVec4u& v) const; // [tested]
  plSimdVec4b operator<(const plSimdVec4u& v) const;  // [tested]
  plSimdVec4b operator>=(const plSimdVec4u& v) const; // [tested]
  plSimdVec4b operator>(const plSimdVec4u& v) const;  // [tested]

public:
  plInternal::QuadUInt m_v;
};

#if PL_SIMD_IMPLEMENTATION == PL_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4u_inl.h>
#elif PL_SIMD_IMPLEMENTATION == PL_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4u_inl.h>
#elif PL_SIMD_IMPLEMENTATION == PL_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4u_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif

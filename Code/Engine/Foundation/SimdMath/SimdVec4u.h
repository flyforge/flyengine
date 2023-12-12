#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

/// \brief A SIMD 4-component vector class of unsigned 32b integers
class PLASMA_FOUNDATION_DLL plSimdVec4u
{
public:
  PLASMA_DECLARE_POD_TYPE();

  plSimdVec4u(); // [tested]

  explicit plSimdVec4u(plUInt32 uiXyzw); // [tested]

  plSimdVec4u(plUInt32 x, plUInt32 y, plUInt32 z, plUInt32 w = 1); // [tested]

  plSimdVec4u(plInternal::QuadUInt v); // [tested]

  void Set(plUInt32 uiXyzw); // [tested]

  void Set(plUInt32 x, plUInt32 y, plUInt32 z, plUInt32 w); // [tested]

  void SetZero(); // [tested]

public:
  explicit plSimdVec4u(const plSimdVec4i& i); // [tested]

public:
  plSimdVec4f ToFloat() const; // [tested]

  static plSimdVec4u Truncate(const plSimdVec4f& f); // [tested]

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
  plSimdVec4u operator+(const plSimdVec4u& v) const; // [tested]
  plSimdVec4u operator-(const plSimdVec4u& v) const; // [tested]

  plSimdVec4u CompMul(const plSimdVec4u& v) const; // [tested]

  plSimdVec4u operator|(const plSimdVec4u& v) const; // [tested]
  plSimdVec4u operator&(const plSimdVec4u& v) const; // [tested]
  plSimdVec4u operator^(const plSimdVec4u& v) const; // [tested]
  plSimdVec4u operator~() const;                     // [tested]

  plSimdVec4u operator<<(plUInt32 uiShift) const; // [tested]
  plSimdVec4u operator>>(plUInt32 uiShift) const; // [tested]

  plSimdVec4u& operator+=(const plSimdVec4u& v); // [tested]
  plSimdVec4u& operator-=(const plSimdVec4u& v); // [tested]

  plSimdVec4u& operator|=(const plSimdVec4u& v); // [tested]
  plSimdVec4u& operator&=(const plSimdVec4u& v); // [tested]
  plSimdVec4u& operator^=(const plSimdVec4u& v); // [tested]

  plSimdVec4u& operator<<=(plUInt32 uiShift); // [tested]
  plSimdVec4u& operator>>=(plUInt32 uiShift); // [tested]

  plSimdVec4u CompMin(const plSimdVec4u& v) const; // [tested]
  plSimdVec4u CompMax(const plSimdVec4u& v) const; // [tested]

  plSimdVec4b operator==(const plSimdVec4u& v) const; // [tested]
  plSimdVec4b operator!=(const plSimdVec4u& v) const; // [tested]
  plSimdVec4b operator<=(const plSimdVec4u& v) const; // [tested]
  plSimdVec4b operator<(const plSimdVec4u& v) const;  // [tested]
  plSimdVec4b operator>=(const plSimdVec4u& v) const; // [tested]
  plSimdVec4b operator>(const plSimdVec4u& v) const;  // [tested]

  static plSimdVec4u ZeroVector(); // [tested]

public:
  plInternal::QuadUInt m_v;
};

#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4u_inl.h>
#elif PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4u_inl.h>
#elif PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4u_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif

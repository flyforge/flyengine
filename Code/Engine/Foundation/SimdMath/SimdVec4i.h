#pragma once

#include <Foundation/SimdMath/SimdVec4f.h>

class plSimdVec4u;

/// \brief A SIMD 4-component vector class of signed 32b integers
class PLASMA_FOUNDATION_DLL plSimdVec4i
{
public:
  PLASMA_DECLARE_POD_TYPE();

  plSimdVec4i(); // [tested]

  explicit plSimdVec4i(plInt32 iXyzw); // [tested]

  plSimdVec4i(plInt32 x, plInt32 y, plInt32 z, plInt32 w = 1); // [tested]

  plSimdVec4i(plInternal::QuadInt v); // [tested]

  /// \brief Creates an plSimdVec4i that is initialized to zero.
  [[nodiscard]] static plSimdVec4i MakeZero(); // [tested]

  void Set(plInt32 iXyzw); // [tested]

  void Set(plInt32 x, plInt32 y, plInt32 z, plInt32 w); // [tested]

  void SetZero(); // [tested]

  template <int N>
  void Load(const plInt32* pInts); // [tested]

  template <int N>
  void Store(plInt32* pInts) const; // [tested]

public:
  explicit plSimdVec4i(const plSimdVec4u& u); // [tested]

public:
  plSimdVec4f ToFloat() const; // [tested]

  [[nodiscard]] static plSimdVec4i Truncate(const plSimdVec4f& f); // [tested]

public:
  template <int N>
  plInt32 GetComponent() const; // [tested]

  plInt32 x() const; // [tested]
  plInt32 y() const; // [tested]
  plInt32 z() const; // [tested]
  plInt32 w() const; // [tested]

  template <plSwizzle::Enum s>
  plSimdVec4i Get() const; // [tested]

public:
  [[nodiscard]] plSimdVec4i operator-() const;                     // [tested]
  [[nodiscard]] plSimdVec4i operator+(const plSimdVec4i& v) const; // [tested]
  [[nodiscard]] plSimdVec4i operator-(const plSimdVec4i& v) const; // [tested]

  [[nodiscard]] plSimdVec4i CompMul(const plSimdVec4i& v) const; // [tested]
  [[nodiscard]] plSimdVec4i CompDiv(const plSimdVec4i& v) const; // [tested]

  [[nodiscard]] plSimdVec4i operator|(const plSimdVec4i& v) const; // [tested]
  [[nodiscard]] plSimdVec4i operator&(const plSimdVec4i& v) const; // [tested]
  [[nodiscard]] plSimdVec4i operator^(const plSimdVec4i& v) const; // [tested]
  [[nodiscard]] plSimdVec4i operator~() const;                     // [tested]

  [[nodiscard]] plSimdVec4i operator<<(plUInt32 uiShift) const;     // [tested]
  [[nodiscard]] plSimdVec4i operator>>(plUInt32 uiShift) const;     // [tested]
  [[nodiscard]] plSimdVec4i operator<<(const plSimdVec4i& v) const; // [tested]
  [[nodiscard]] plSimdVec4i operator>>(const plSimdVec4i& v) const; // [tested]

  plSimdVec4i& operator+=(const plSimdVec4i& v); // [tested]
  plSimdVec4i& operator-=(const plSimdVec4i& v); // [tested]

  plSimdVec4i& operator|=(const plSimdVec4i& v); // [tested]
  plSimdVec4i& operator&=(const plSimdVec4i& v); // [tested]
  plSimdVec4i& operator^=(const plSimdVec4i& v); // [tested]

  plSimdVec4i& operator<<=(plUInt32 uiShift); // [tested]
  plSimdVec4i& operator>>=(plUInt32 uiShift); // [tested]

  [[nodiscard]] plSimdVec4i CompMin(const plSimdVec4i& v) const; // [tested]
  [[nodiscard]] plSimdVec4i CompMax(const plSimdVec4i& v) const; // [tested]
  [[nodiscard]] plSimdVec4i Abs() const;                         // [tested]

  [[nodiscard]] plSimdVec4b operator==(const plSimdVec4i& v) const; // [tested]
  [[nodiscard]] plSimdVec4b operator!=(const plSimdVec4i& v) const; // [tested]
  [[nodiscard]] plSimdVec4b operator<=(const plSimdVec4i& v) const; // [tested]
  [[nodiscard]] plSimdVec4b operator<(const plSimdVec4i& v) const;  // [tested]
  [[nodiscard]] plSimdVec4b operator>=(const plSimdVec4i& v) const; // [tested]
  [[nodiscard]] plSimdVec4b operator>(const plSimdVec4i& v) const;  // [tested]

  [[nodiscard]] static plSimdVec4i Select(const plSimdVec4b& vCmp, const plSimdVec4i& vTrue, const plSimdVec4i& vFalse); // [tested]

public:
  plInternal::QuadInt m_v;
};

#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4i_inl.h>
#elif PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4i_inl.h>
#elif PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4i_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif

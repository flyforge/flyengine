#pragma once

#include <Foundation/SimdMath/SimdSwizzle.h>
#include <Foundation/SimdMath/SimdTypes.h>

class PLASMA_FOUNDATION_DLL plSimdVec4b
{
public:
  PLASMA_DECLARE_POD_TYPE();

  plSimdVec4b();                               // [tested]
  plSimdVec4b(bool b);                         // [tested]
  plSimdVec4b(bool x, bool y, bool z, bool w); // [tested]
  plSimdVec4b(plInternal::QuadBool b);         // [tested]

public:
  template <int N>
  bool GetComponent() const; // [tested]

  bool x() const; // [tested]
  bool y() const; // [tested]
  bool z() const; // [tested]
  bool w() const; // [tested]

  template <plSwizzle::Enum s>
  plSimdVec4b Get() const; // [tested]

public:
  plSimdVec4b operator&&(const plSimdVec4b& rhs) const; // [tested]
  plSimdVec4b operator||(const plSimdVec4b& rhs) const; // [tested]
  plSimdVec4b operator!() const;                        // [tested]

  plSimdVec4b operator==(const plSimdVec4b& rhs) const; // [tested]
  plSimdVec4b operator!=(const plSimdVec4b& rhs) const; // [tested]

  template <int N = 4>
  bool AllSet() const; // [tested]

  template <int N = 4>
  bool AnySet() const; // [tested]

  template <int N = 4>
  bool NoneSet() const; // [tested]

  static plSimdVec4b Select(const plSimdVec4b& vCmp, const plSimdVec4b& vTrue, const plSimdVec4b& vFalse); // [tested]

public:
  plInternal::QuadBool m_v;
};

#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4b_inl.h>
#elif PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4b_inl.h>
#elif PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4b_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif

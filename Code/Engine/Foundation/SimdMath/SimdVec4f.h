#pragma once

#include <Foundation/SimdMath/SimdFloat.h>
#include <Foundation/SimdMath/SimdVec4b.h>

/// \brief A 4-component SIMD vector class
class PLASMA_FOUNDATION_DLL plSimdVec4f
{
public:
  PLASMA_DECLARE_POD_TYPE();

  plSimdVec4f(); // [tested]

  explicit plSimdVec4f(float fXyzw); // [tested]

  explicit plSimdVec4f(const plSimdFloat& fXyzw); // [tested]

  plSimdVec4f(float x, float y, float z, float w = 1.0f); // [tested]

  plSimdVec4f(plInternal::QuadFloat v); // [tested]

  void Set(float fXyzw); // [tested]

  void Set(float x, float y, float z, float w); // [tested]

  void SetX(const plSimdFloat& f); // [tested]
  void SetY(const plSimdFloat& f); // [tested]
  void SetZ(const plSimdFloat& f); // [tested]
  void SetW(const plSimdFloat& f); // [tested]

  void SetZero(); // [tested]

  template <int N>
  void Load(const float* pFloats); // [tested]

  template <int N>
  void Store(float* pFloats) const; // [tested]

public:
  template <plMathAcc::Enum acc = plMathAcc::FULL>
  plSimdVec4f GetReciprocal() const; // [tested]

  template <plMathAcc::Enum acc = plMathAcc::FULL>
  plSimdVec4f GetSqrt() const; // [tested]

  template <plMathAcc::Enum acc = plMathAcc::FULL>
  plSimdVec4f GetInvSqrt() const; // [tested]

  template <int N, plMathAcc::Enum acc = plMathAcc::FULL>
  plSimdFloat GetLength() const; // [tested]

  template <int N, plMathAcc::Enum acc = plMathAcc::FULL>
  plSimdFloat GetInvLength() const; // [tested]

  template <int N>
  plSimdFloat GetLengthSquared() const; // [tested]

  template <int N, plMathAcc::Enum acc = plMathAcc::FULL>
  plSimdFloat GetLengthAndNormalize(); // [tested]

  template <int N, plMathAcc::Enum acc = plMathAcc::FULL>
  plSimdVec4f GetNormalized() const; // [tested]

  template <int N, plMathAcc::Enum acc = plMathAcc::FULL>
  void Normalize(); // [tested]

  template <int N, plMathAcc::Enum acc = plMathAcc::FULL>
  void NormalizeIfNotZero(const plSimdFloat& fEpsilon = plMath::SmallEpsilon<float>()); // [tested]

  template <int N>
  bool IsZero() const; // [tested]

  template <int N>
  bool IsZero(const plSimdFloat& fEpsilon) const; // [tested]

  template <int N>
  bool IsNormalized(const plSimdFloat& fEpsilon = plMath::HugeEpsilon<float>()) const; // [tested]

  template <int N>
  bool IsNaN() const; // [tested]

  template <int N>
  bool IsValid() const; // [tested]

public:
  template <int N>
  plSimdFloat GetComponent() const; // [tested]

  plSimdFloat GetComponent(int i) const; // [tested]

  plSimdFloat x() const; // [tested]
  plSimdFloat y() const; // [tested]
  plSimdFloat z() const; // [tested]
  plSimdFloat w() const; // [tested]

  template <plSwizzle::Enum s>
  plSimdVec4f Get() const; // [tested]

  ///\brief x = this[s0], y = this[s1], z = other[s2], w = other[s3]
  template <plSwizzle::Enum s>
  plSimdVec4f GetCombined(const plSimdVec4f& other) const; // [tested]

public:
  plSimdVec4f operator-() const;                     // [tested]
  plSimdVec4f operator+(const plSimdVec4f& v) const; // [tested]
  plSimdVec4f operator-(const plSimdVec4f& v) const; // [tested]

  plSimdVec4f operator*(const plSimdFloat& f) const; // [tested]
  plSimdVec4f operator/(const plSimdFloat& f) const; // [tested]

  plSimdVec4f CompMul(const plSimdVec4f& v) const; // [tested]

  template <plMathAcc::Enum acc = plMathAcc::FULL>
  plSimdVec4f CompDiv(const plSimdVec4f& v) const; // [tested]

  plSimdVec4f CompMin(const plSimdVec4f& rhs) const; // [tested]
  plSimdVec4f CompMax(const plSimdVec4f& rhs) const; // [tested]

  plSimdVec4f Abs() const;      // [tested]
  plSimdVec4f Round() const;    // [tested]
  plSimdVec4f Floor() const;    // [tested]
  plSimdVec4f Ceil() const;     // [tested]
  plSimdVec4f Trunc() const;    // [tested]
  plSimdVec4f Fraction() const; // [tested]

  plSimdVec4f FlipSign(const plSimdVec4b& vCmp) const; // [tested]

  static plSimdVec4f Select(const plSimdVec4b& vCmp, const plSimdVec4f& vTrue, const plSimdVec4f& vFalse); // [tested]

  static plSimdVec4f Lerp(const plSimdVec4f& a, const plSimdVec4f& b, const plSimdVec4f& t);

  plSimdVec4f& operator+=(const plSimdVec4f& v); // [tested]
  plSimdVec4f& operator-=(const plSimdVec4f& v); // [tested]

  plSimdVec4f& operator*=(const plSimdFloat& f); // [tested]
  plSimdVec4f& operator/=(const plSimdFloat& f); // [tested]

  plSimdVec4b IsEqual(const plSimdVec4f& rhs, const plSimdFloat& fEpsilon) const; // [tested]

  plSimdVec4b operator==(const plSimdVec4f& v) const; // [tested]
  plSimdVec4b operator!=(const plSimdVec4f& v) const; // [tested]
  plSimdVec4b operator<=(const plSimdVec4f& v) const; // [tested]
  plSimdVec4b operator<(const plSimdVec4f& v) const;  // [tested]
  plSimdVec4b operator>=(const plSimdVec4f& v) const; // [tested]
  plSimdVec4b operator>(const plSimdVec4f& v) const;  // [tested]

  template <int N>
  plSimdFloat HorizontalSum() const; // [tested]

  template <int N>
  plSimdFloat HorizontalMin() const; // [tested]

  template <int N>
  plSimdFloat HorizontalMax() const; // [tested]

  template <int N>
  plSimdFloat Dot(const plSimdVec4f& v) const; // [tested]

  ///\brief 3D cross product, w is ignored.
  plSimdVec4f CrossRH(const plSimdVec4f& v) const; // [tested]

  ///\brief Generates an arbitrary vector such that Dot<3>(GetOrthogonalVector()) == 0
  plSimdVec4f GetOrthogonalVector() const; // [tested]

  static plSimdVec4f ZeroVector(); // [tested]

  static plSimdVec4f MulAdd(const plSimdVec4f& a, const plSimdVec4f& b, const plSimdVec4f& c); // [tested]
  static plSimdVec4f MulAdd(const plSimdVec4f& a, const plSimdFloat& b, const plSimdVec4f& c); // [tested]

  static plSimdVec4f MulSub(const plSimdVec4f& a, const plSimdVec4f& b, const plSimdVec4f& c); // [tested]
  static plSimdVec4f MulSub(const plSimdVec4f& a, const plSimdFloat& b, const plSimdVec4f& c); // [tested]

  static plSimdVec4f CopySign(const plSimdVec4f& vMagnitude, const plSimdVec4f& vSign); // [tested]

public:
  plInternal::QuadFloat m_v;
};

#include <Foundation/SimdMath/Implementation/SimdVec4f_inl.h>

#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEVec4f_inl.h>
#elif PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUVec4f_inl.h>
#elif PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONVec4f_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif

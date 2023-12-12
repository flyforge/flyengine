#pragma once

#include <Foundation/SimdMath/SimdVec4f.h>

/// \brief A 4x4 matrix class
class PLASMA_FOUNDATION_DLL plSimdMat4f
{
public:
  PLASMA_DECLARE_POD_TYPE();

  plSimdMat4f();

  plSimdMat4f(const float* const pData, plMatrixLayout::Enum layout); // [tested]

  plSimdMat4f(const plSimdVec4f& vCol0, const plSimdVec4f& vCol1, const plSimdVec4f& vCol2, const plSimdVec4f& vCol3); // [tested]

  /// \brief Sets each element manually: Naming is "column-n row-m"
  plSimdMat4f(float f1r1, float f2r1, float f3r1, float f4r1, float f1r2, float f2r2, float f3r2, float f4r2, float f1r3, float f2r3, float f3r3,
    float f4r3, float f1r4, float f2r4, float f3r4, float f4r4); // [tested]

  void SetFromArray(const float* const pData, plMatrixLayout::Enum layout); // [tested]

  void GetAsArray(float* out_pData, plMatrixLayout::Enum layout) const; // [tested]

  /// \brief Sets all elements to zero, except the diagonal, which is set to one.
  void SetIdentity(); // [tested]

  /// \brief Returns an Identity Matrix.
  static plSimdMat4f IdentityMatrix(); // [tested]

public:
  /// \brief Transposes this matrix.
  void Transpose(); // [tested]

  /// \brief Returns the transpose of this matrix.
  plSimdMat4f GetTranspose() const; // [tested]

  /// \brief Inverts this matrix. Return value indicates whether the matrix could be inverted.
  plResult Invert(const plSimdFloat& fEpsilon = plMath::SmallEpsilon<float>()); // [tested]

  /// \brief Returns the inverse of this matrix.
  plSimdMat4f GetInverse(const plSimdFloat& fEpsilon = plMath::SmallEpsilon<float>()) const; // [tested]

public:
  /// \brief Equality Check with epsilon
  bool IsEqual(const plSimdMat4f& rhs, const plSimdFloat& fEpsilon) const; // [tested]

  /// \brief Checks whether this is an identity matrix.
  bool IsIdentity(const plSimdFloat& fEpsilon = plMath::DefaultEpsilon<float>()) const; // [tested]

  /// \brief Checks whether all components are finite numbers.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

public:
  void SetRows(const plSimdVec4f& vRow0, const plSimdVec4f& vRow1, const plSimdVec4f& vRow2, const plSimdVec4f& vRow3); // [tested]
  void GetRows(plSimdVec4f& ref_vRow0, plSimdVec4f& ref_vRow1, plSimdVec4f& ref_vRow2, plSimdVec4f& ref_vRow3) const;   // [tested]

public:
  /// \brief Matrix-vector multiplication, assuming the 4th component of the vector is one (default behavior).
  plSimdVec4f TransformPosition(const plSimdVec4f& v) const; // [tested]

  /// \brief Matrix-vector multiplication, assuming the 4th component of the vector is zero. So, rotation/scaling only.
  plSimdVec4f TransformDirection(const plSimdVec4f& v) const; // [tested]

  plSimdMat4f operator*(const plSimdMat4f& rhs) const; // [tested]
  void operator*=(const plSimdMat4f& rhs);

  bool operator==(const plSimdMat4f& rhs) const; // [tested]
  bool operator!=(const plSimdMat4f& rhs) const; // [tested]

public:
  plSimdVec4f m_col0;
  plSimdVec4f m_col1;
  plSimdVec4f m_col2;
  plSimdVec4f m_col3;
};

#include <Foundation/SimdMath/Implementation/SimdMat4f_inl.h>

#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
#  include <Foundation/SimdMath/Implementation/SSE/SSEMat4f_inl.h>
#elif PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUMat4f_inl.h>
#elif PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONMat4f_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif

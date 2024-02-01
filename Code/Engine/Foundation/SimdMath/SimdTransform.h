#pragma once

#include <Foundation/SimdMath/SimdQuat.h>

class PL_FOUNDATION_DLL plSimdTransform
{
public:
  PL_DECLARE_POD_TYPE();

  /// \brief Default constructor: Does not do any initialization.
  plSimdTransform(); // [tested]

  /// \brief Sets position, rotation and scale.
  explicit plSimdTransform(const plSimdVec4f& vPosition, const plSimdQuat& qRotation = plSimdQuat::MakeIdentity(), const plSimdVec4f& vScale = plSimdVec4f(1.0f)); // [tested]

  /// \brief Sets rotation.
  explicit plSimdTransform(const plSimdQuat& qRotation); // [tested]

  /// \brief Creates a transform from the given position, rotation and scale.
  [[nodiscard]] static plSimdTransform Make(const plSimdVec4f& vPosition, const plSimdQuat& qRotation = plSimdQuat::MakeIdentity(), const plSimdVec4f& vScale = plSimdVec4f(1.0f)); // [tested]

  /// \brief Creates an identity transform.
  [[nodiscard]] static plSimdTransform MakeIdentity(); // [tested]

  /// \brief Creates a transform that is the local transformation needed to get from the parent's transform to the child's.
  [[nodiscard]] static plSimdTransform MakeLocalTransform(const plSimdTransform& globalTransformParent, const plSimdTransform& globalTransformChild); // [tested]

  /// \brief Creates a transform that is the global transform, that is reached by applying the child's local transform to the parent's global one.
  [[nodiscard]] static plSimdTransform MakeGlobalTransform(const plSimdTransform& globalTransformParent, const plSimdTransform& localTransformChild); // [tested]

  /// \brief Returns the scale component with maximum magnitude.
  plSimdFloat GetMaxScale() const; // [tested]

  /// \brief Returns whether this transform contains negative scaling aka mirroring.
  bool ContainsNegativeScale() const;

  /// \brief Returns whether this transform contains uniform scaling.
  bool ContainsUniformScale() const;

public:
  /// \brief Equality Check with epsilon
  bool IsEqual(const plSimdTransform& rhs, const plSimdFloat& fEpsilon) const; // [tested]

public:
  /// \brief Inverts this transform.
  void Invert(); // [tested]

  /// \brief Returns the inverse of this transform.
  plSimdTransform GetInverse() const; // [tested]

  /// \brief Returns the transformation as a matrix.
  plSimdMat4f GetAsMat4() const; // [tested]

public:
  [[nodiscard]] plSimdVec4f TransformPosition(const plSimdVec4f& v) const;  // [tested]
  [[nodiscard]] plSimdVec4f TransformDirection(const plSimdVec4f& v) const; // [tested]

  /// \brief Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
  void operator*=(const plSimdTransform& other); // [tested]

  /// \brief Multiplies \a q into the rotation component, thus rotating the entire transformation.
  void operator*=(const plSimdQuat& q); // [tested]

  void operator+=(const plSimdVec4f& v); // [tested]
  void operator-=(const plSimdVec4f& v); // [tested]

public:
  plSimdVec4f m_Position;
  plSimdQuat m_Rotation;
  plSimdVec4f m_Scale;
};

// *** free functions ***

/// \brief Transforms the vector v by the transform.
PL_ALWAYS_INLINE const plSimdVec4f operator*(const plSimdTransform& t, const plSimdVec4f& v); // [tested]

/// \brief Rotates the transform by the given quaternion. Multiplies q from the left with t.
PL_ALWAYS_INLINE const plSimdTransform operator*(const plSimdQuat& q, const plSimdTransform& t); // [tested]

/// \brief Rotates the transform by the given quaternion. Multiplies q from the right with t.
PL_ALWAYS_INLINE const plSimdTransform operator*(const plSimdTransform& t, const plSimdQuat& q); // [tested]

/// \brief Translates the plSimdTransform by the vector. This will move the object in global space.
PL_ALWAYS_INLINE const plSimdTransform operator+(const plSimdTransform& t, const plSimdVec4f& v); // [tested]

/// \brief Translates the plSimdTransform by the vector. This will move the object in global space.
PL_ALWAYS_INLINE const plSimdTransform operator-(const plSimdTransform& t, const plSimdVec4f& v); // [tested]

/// \brief Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
PL_ALWAYS_INLINE const plSimdTransform operator*(const plSimdTransform& lhs, const plSimdTransform& rhs); // [tested]

PL_ALWAYS_INLINE bool operator==(const plSimdTransform& t1, const plSimdTransform& t2); // [tested]
PL_ALWAYS_INLINE bool operator!=(const plSimdTransform& t1, const plSimdTransform& t2); // [tested]


#include <Foundation/SimdMath/Implementation/SimdTransform_inl.h>

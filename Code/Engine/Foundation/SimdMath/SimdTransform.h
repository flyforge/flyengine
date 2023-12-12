#pragma once

#include <Foundation/SimdMath/SimdQuat.h>

class PLASMA_FOUNDATION_DLL plSimdTransform
{
public:
  PLASMA_DECLARE_POD_TYPE();

  /// \brief Default constructor: Does not do any initialization.
  plSimdTransform(); // [tested]

  /// \brief Sets position, rotation and scale.
  explicit plSimdTransform(const plSimdVec4f& vPosition, const plSimdQuat& qRotation = plSimdQuat::IdentityQuaternion(),
    const plSimdVec4f& vScale = plSimdVec4f(1.0f)); // [tested]

  /// \brief Sets rotation.
  explicit plSimdTransform(const plSimdQuat& qRotation); // [tested]

  /// \brief Sets the position to be zero and the rotation to identity.
  void SetIdentity(); // [tested]

  /// \brief Returns an Identity Transform.
  static plSimdTransform IdentityTransform(); // [tested]

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

public:
  /// \brief Sets this transform to be the local transformation needed to get from the parent's transform to the child's.
  void SetLocalTransform(const plSimdTransform& globalTransformParent, const plSimdTransform& globalTransformChild); // [tested]

  /// \brief Sets this transform to the global transform, that is reached by applying the child's local transform to the parent's global
  /// one.
  void SetGlobalTransform(const plSimdTransform& globalTransformParent, const plSimdTransform& localTransformChild); // [tested]

  /// \brief Returns the transformation as a matrix.
  plSimdMat4f GetAsMat4() const; // [tested]

public:
  plSimdVec4f TransformPosition(const plSimdVec4f& v) const;  // [tested]
  plSimdVec4f TransformDirection(const plSimdVec4f& v) const; // [tested]

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
PLASMA_ALWAYS_INLINE const plSimdVec4f operator*(const plSimdTransform& t, const plSimdVec4f& v); // [tested]

/// \brief Rotates the transform by the given quaternion. Multiplies q from the left with t.
PLASMA_ALWAYS_INLINE const plSimdTransform operator*(const plSimdQuat& q, const plSimdTransform& t); // [tested]

/// \brief Rotates the transform by the given quaternion. Multiplies q from the right with t.
PLASMA_ALWAYS_INLINE const plSimdTransform operator*(const plSimdTransform& t, const plSimdQuat& q); // [tested]

/// \brief Translates the plSimdTransform by the vector. This will move the object in global space.
PLASMA_ALWAYS_INLINE const plSimdTransform operator+(const plSimdTransform& t, const plSimdVec4f& v); // [tested]

/// \brief Translates the plSimdTransform by the vector. This will move the object in global space.
PLASMA_ALWAYS_INLINE const plSimdTransform operator-(const plSimdTransform& t, const plSimdVec4f& v); // [tested]

/// \brief Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
PLASMA_ALWAYS_INLINE const plSimdTransform operator*(const plSimdTransform& lhs, const plSimdTransform& rhs); // [tested]

PLASMA_ALWAYS_INLINE bool operator==(const plSimdTransform& t1, const plSimdTransform& t2); // [tested]
PLASMA_ALWAYS_INLINE bool operator!=(const plSimdTransform& t1, const plSimdTransform& t2); // [tested]


#include <Foundation/SimdMath/Implementation/SimdTransform_inl.h>

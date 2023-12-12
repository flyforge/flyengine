#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec4.h>

/// \brief A combination of a bounding box and a bounding sphere with the same center.
///
/// This class uses less memory than storying a bounding box and sphere separate.

template <typename Type>
class plBoundingBoxSphereTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  PLASMA_DECLARE_POD_TYPE();

  using ComponentType = Type;

public:
  /// \brief Default constructor does not initialize anything.
  plBoundingBoxSphereTemplate(); // [tested]

  /// \brief Constructs the bounds from the center position, the box half extends and the sphere radius.
  plBoundingBoxSphereTemplate(const plVec3Template<Type>& vCenter, const plVec3Template<Type>& vBoxHalfExtents, Type fSphereRadius); // [tested]

  /// \brief Constructs the bounds from the given box and sphere.
  plBoundingBoxSphereTemplate(const plBoundingBoxTemplate<Type>& box, const plBoundingSphereTemplate<Type>& sphere); // [tested]

  /// \brief Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  plBoundingBoxSphereTemplate(const plBoundingBoxTemplate<Type>& box); // [tested]

  /// \brief Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  plBoundingBoxSphereTemplate(const plBoundingSphereTemplate<Type>& sphere); // [tested]

#if PLASMA_ENABLED(PLASMA_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    PLASMA_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                               "all code-paths properly initialize this object.");
  }
#endif

  /// \brief Resets the bounds to an invalid state.
  void SetInvalid(); // [tested]

  /// \brief Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Calculates the bounds from given set of points.
  void SetFromPoints(const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plVec3Template<Type>)); // [tested]

  /// \brief Returns the bounding box.
  const plBoundingBoxTemplate<Type> GetBox() const; // [tested]

  /// \brief Returns the bounding sphere.
  const plBoundingSphereTemplate<Type> GetSphere() const; // [tested]

  /// \brief Expands the bounds such that the given bounds are inside it.
  void ExpandToInclude(const plBoundingBoxSphereTemplate& rhs); // [tested]

  /// \brief Transforms the bounds in its local space.
  void Transform(const plMat4Template<Type>& mTransform); // [tested]

public:
  plVec3Template<Type> m_vCenter;
  Type m_fSphereRadius;
  plVec3Template<Type> m_vBoxHalfExtends;
};

/// \brief Checks whether this bounds and the other are identical.
template <typename Type>
bool operator==(const plBoundingBoxSphereTemplate<Type>& lhs, const plBoundingBoxSphereTemplate<Type>& rhs); // [tested]

/// \brief Checks whether this bounds and the other are not identical.
template <typename Type>
bool operator!=(const plBoundingBoxSphereTemplate<Type>& lhs, const plBoundingBoxSphereTemplate<Type>& rhs); // [tested]


#include <Foundation/Math/Implementation/BoundingBoxSphere_inl.h>

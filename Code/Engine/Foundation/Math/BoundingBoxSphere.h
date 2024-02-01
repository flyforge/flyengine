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
  PL_DECLARE_POD_TYPE();

  using ComponentType = Type;

public:
  /// \brief Default constructor does not initialize anything.
  plBoundingBoxSphereTemplate(); // [tested]

  plBoundingBoxSphereTemplate(const plBoundingBoxSphereTemplate& rhs);

  void operator=(const plBoundingBoxSphereTemplate& rhs);

  /// \brief Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  plBoundingBoxSphereTemplate(const plBoundingBoxTemplate<Type>& box); // [tested]

  /// \brief Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  plBoundingBoxSphereTemplate(const plBoundingSphereTemplate<Type>& sphere); // [tested]

  /// \brief Creates an object with all zero values. These are valid bounds around the origin with no volume.
  [[nodiscard]] static plBoundingBoxSphereTemplate<Type> MakeZero();

  /// \brief Creates an 'invalid' object, ie one with negative extents/radius. Invalid objects can be made valid through ExpandToInclude().
  [[nodiscard]] static plBoundingBoxSphereTemplate<Type> MakeInvalid();

  /// \brief Creates an object from the given center point and extents.
  [[nodiscard]] static plBoundingBoxSphereTemplate<Type> MakeFromCenterExtents(const plVec3Template<Type>& vCenter, const plVec3Template<Type>& vBoxHalfExtents, Type fSphereRadius);

  /// \brief Creates an object that contains all the provided points.
  [[nodiscard]] static plBoundingBoxSphereTemplate<Type> MakeFromPoints(const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plVec3Template<Type>));

  /// \brief Creates an object from another bounding box.
  [[nodiscard]] static plBoundingBoxSphereTemplate<Type> MakeFromBox(const plBoundingBoxTemplate<Type>& box);

  /// \brief Creates an object from another bounding sphere.
  [[nodiscard]] static plBoundingBoxSphereTemplate<Type> MakeFromSphere(const plBoundingSphereTemplate<Type>& sphere);

  /// \brief Creates an object from another bounding box and a sphere.
  [[nodiscard]] static plBoundingBoxSphereTemplate<Type> MakeFromBoxAndSphere(const plBoundingBoxTemplate<Type>& box, const plBoundingSphereTemplate<Type>& sphere);


#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    PL_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                               "all code-paths properly initialize this object.");
  }
#endif

  /// \brief Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

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

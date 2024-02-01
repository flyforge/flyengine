#pragma once

#include <Foundation/Math/Vec3.h>

/// \brief An implementation of a bounding sphere.
///
/// This class allows to construct and manipulate bounding spheres.
/// It also provides a large set of functions to do overlap queries, ray casts and other useful operations.
template <typename Type>
class plBoundingSphereTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  PL_DECLARE_POD_TYPE();

  using ComponentType = Type;

public:
  /// \brief Default constructor does not initialize any data.
  plBoundingSphereTemplate();


  /// \brief Creates a sphere at the origin with radius zero.
  [[nodiscard]] static plBoundingSphereTemplate<Type> MakeZero();

  /// \brief Creates an 'invalid' sphere, with its center at the given position and a negative radius.
  ///
  /// Such a sphere can be made 'valid' through ExpandToInclude(), but be aware that the originally provided center position
  /// will always be part of the sphere.
  [[nodiscard]] static plBoundingSphereTemplate<Type> MakeInvalid(const plVec3Template<Type>& vCenter = plVec3Template<Type>::MakeZero());

  /// \brief Creates a sphere with the provided center and radius.
  [[nodiscard]] static plBoundingSphereTemplate<Type> MakeFromCenterAndRadius(const plVec3Template<Type>& vCenter, Type fRadius);

  /// \brief Creates a bounding sphere around the provided points.
  ///
  /// The center of the sphere will be at the 'center of mass' of all the points, and the radius will be the distance to the
  /// farthest point from there.
  [[nodiscard]] static plBoundingSphereTemplate<Type> MakeFromPoints(const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plVec3Template<Type>));

#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    PL_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                               "all code-paths properly initialize this object.");
  }
#endif

  /// \brief Checks whether the sphere is all zero.
  bool IsZero(Type fEpsilon = plMath::DefaultEpsilon<Type>()) const; // [tested]

  /// \brief Returns whether the sphere has valid values.
  bool IsValid() const; // [tested]

  /// \brief Returns whether any value is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Increases the sphere's radius to include this point. Does NOT change its position, thus the resulting sphere might be not a very tight
  /// fit.
  void ExpandToInclude(const plVec3Template<Type>& vPoint); // [tested]

  /// \brief Increases the sphere's radius to include all given points. Does NOT change its position, thus the resulting sphere might be not a very
  /// tight fit. More efficient than calling this for every point individually.
  void ExpandToInclude(const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plVec3Template<Type>)); // [tested]

  /// \brief Increases this sphere's radius, such that it encloses the other sphere. Does not change the center position of this sphere.
  void ExpandToInclude(const plBoundingSphereTemplate& rhs); // [tested]

  /// \brief Increases this sphere's radius, such that it encloses the box. Does not change the center position of this sphere.
  void ExpandToInclude(const plBoundingBoxTemplate<Type>& rhs); // [tested]

  /// \brief Increases the size of the sphere by the given amount.
  void Grow(Type fDiff); // [tested]

  /// \brief Tests whether two spheres are identical.
  bool IsIdentical(const plBoundingSphereTemplate& rhs) const; // [tested]

  /// \brief Tests whether two spheres are equal within some threshold.
  bool IsEqual(const plBoundingSphereTemplate& rhs, Type fEpsilon = plMath::DefaultEpsilon<Type>()) const; // [tested]

  /// \brief Moves the sphere by the given vector.
  void Translate(const plVec3Template<Type>& vTranslation); // [tested]

  /// \brief Scales the sphere's size, does not change its center position.
  void ScaleFromCenter(Type fScale); // [tested]

  /// \brief Scales the sphere in world unites, meaning its center position will change as well.
  void ScaleFromOrigin(const plVec3Template<Type>& vScale); // [tested]

  /// \brief Transforms the sphere with the given matrix from the world origin. I.e. scalings and rotations will influence its position.
  void TransformFromOrigin(const plMat4Template<Type>& mTransform); // [tested]

  /// \brief Transforms the sphere with the given matrix from its own center. I.e. rotations have no effect, scalings will only affect the radius, and
  /// only translations will affect its position.
  void TransformFromCenter(const plMat4Template<Type>& mTransform); // [tested]

  /// \brief Computes the distance of the point to the sphere's surface. Returns negative values for points inside the sphere.
  Type GetDistanceTo(const plVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Returns the distance between the two spheres. Zero for spheres that are exactly touching each other, negative values for overlapping
  /// spheres.
  Type GetDistanceTo(const plBoundingSphereTemplate& rhs) const; // [tested]

  /// \brief Returns the minimum distance between the box and the sphere. Zero if both are exactly touching, negative values if they overlap.
  Type GetDistanceTo(const plBoundingBoxTemplate<Type>& rhs) const; // [tested]

  /// \brief Returns the minimum distance of any of the points to the sphere.
  Type GetDistanceTo(const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plVec3Template<Type>)) const; // [tested]

  /// \brief Returns true if the given point is inside the sphere.
  bool Contains(const plVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Returns whether all the given points are inside this sphere.
  bool Contains(const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plVec3Template<Type>)) const; // [tested]

  /// \brief Returns whether the other sphere is completely inside this sphere.
  bool Contains(const plBoundingSphereTemplate& rhs) const; // [tested]

  /// \brief Returns whether the given box is completely inside this sphere.
  bool Contains(const plBoundingBoxTemplate<Type>& rhs) const; // [tested]

  /// \brief Checks whether any of the given points is inside the sphere
  bool Overlaps(const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plVec3Template<Type>)) const; // [tested]

  /// \brief Checks whether the two objects overlap.
  bool Overlaps(const plBoundingSphereTemplate& rhs) const; // [tested]

  /// \brief Checks whether the two objects overlap.
  bool Overlaps(const plBoundingBoxTemplate<Type>& rhs) const; // [tested]

  /// \brief Returns a bounding box that encloses this sphere.
  const plBoundingBoxTemplate<Type> GetBoundingBox() const; // [tested]

  /// \brief Clamps the given position to the volume of the sphere. The resulting point will always be inside the sphere, but have the closest
  /// distance to the original point.
  [[nodiscard]] const plVec3Template<Type> GetClampedPoint(const plVec3Template<Type>& vPoint); // [tested]

  /// \brief Computes the intersection of a ray with this sphere. Returns true if there was an intersection. May optionally return the intersection
  /// time and position. The ray's direction must be normalized. The function will also return true, if the ray already starts inside the sphere, but
  /// it will still compute the intersection with the surface of the sphere.
  [[nodiscard]] bool GetRayIntersection(const plVec3Template<Type>& vRayStartPos, const plVec3Template<Type>& vRayDir, Type* out_pIntersectionDistance = nullptr,
    plVec3Template<Type>* out_pIntersection = nullptr) const; // [tested]

  /// \brief Returns true if the line segment intersects the sphere.
  [[nodiscard]] bool GetLineSegmentIntersection(const plVec3Template<Type>& vLineStartPos, const plVec3Template<Type>& vLineEndPos,
    Type* out_pHitFraction = nullptr, plVec3Template<Type>* out_pIntersection = nullptr) const; // [tested]


public:
  plVec3Template<Type> m_vCenter;
  Type m_fRadius;
};

/// \brief Checks whether this sphere and the other are identical.
template <typename Type>
bool operator==(const plBoundingSphereTemplate<Type>& lhs, const plBoundingSphereTemplate<Type>& rhs); // [tested]

/// \brief Checks whether this sphere and the other are not identical.
template <typename Type>
bool operator!=(const plBoundingSphereTemplate<Type>& lhs, const plBoundingSphereTemplate<Type>& rhs); // [tested]


#include <Foundation/Math/Implementation/BoundingSphere_inl.h>

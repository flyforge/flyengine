#pragma once

#include <Foundation/SimdMath/SimdTransform.h>

class PL_FOUNDATION_DLL plSimdBSphere
{
public:
  PL_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize any data.
  plSimdBSphere();

  /// \brief Creates a sphere with the given radius around the given center.
  plSimdBSphere(const plSimdVec4f& vCenter, const plSimdFloat& fRadius); // [tested]

  /// \brief Creates a sphere at the origin with radius zero.
  [[nodiscard]] static plSimdBSphere MakeZero();

  /// \brief Creates an 'invalid' sphere, with its center at the given position and a negative radius.
  ///
  /// Such a sphere can be made 'valid' through ExpandToInclude(), but be aware that the originally provided center position
  /// will always be part of the sphere.
  [[nodiscard]] static plSimdBSphere MakeInvalid(const plSimdVec4f& vCenter = plSimdVec4f::MakeZero()); // [tested]

  /// \brief Creates a sphere with the provided center and radius.
  [[nodiscard]] static plSimdBSphere MakeFromCenterAndRadius(const plSimdVec4f& vCenter, const plSimdFloat& fRadius); // [tested]

  /// \brief Creates a bounding sphere around the provided points.
  ///
  /// The center of the sphere will be at the 'center of mass' of all the points, and the radius will be the distance to the
  /// farthest point from there.
  [[nodiscard]] static plSimdBSphere MakeFromPoints(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plSimdVec4f));


public:
  /// \brief Sets the bounding sphere to invalid values.
  [[deprecated("Use MakeInvalid() instead.")]] void SetInvalid(); // [tested]

  /// \brief Returns whether the sphere has valid values.
  bool IsValid() const; // [tested]

  /// \brief Returns whether any value is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Returns the center
  plSimdVec4f GetCenter() const; // [tested]

  /// \brief Returns the radius
  plSimdFloat GetRadius() const; // [tested]

  /// \brief Initializes the sphere to be the bounding sphere of all the given points.
  [[deprecated("Use MakeFromPoints() instead.")]] void SetFromPoints(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plSimdVec4f));

  /// \brief Increases the sphere's radius to include this point.
  void ExpandToInclude(const plSimdVec4f& vPoint); // [tested]

  /// \brief Increases the sphere's radius to include all given points. Does NOT change its position, thus the resulting sphere might be not
  /// a very tight fit. More efficient than calling this for every point individually.
  void ExpandToInclude(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plSimdVec4f)); // [tested]

  /// \brief Increases this sphere's radius, such that it encloses the other sphere.
  void ExpandToInclude(const plSimdBSphere& rhs); // [tested]

public:
  /// \brief Transforms the sphere in its local space.
  void Transform(const plSimdTransform& t); // [tested]

  /// \brief Transforms the sphere in its local space.
  void Transform(const plSimdMat4f& mMat); // [tested]

public:
  /// \brief Computes the distance of the point to the sphere's surface. Returns negative values for points inside the sphere.
  plSimdFloat GetDistanceTo(const plSimdVec4f& vPoint) const; // [tested]

  /// \brief Returns the distance between the two spheres. Zero for spheres that are exactly touching each other, negative values for
  /// overlapping spheres.
  plSimdFloat GetDistanceTo(const plSimdBSphere& rhs) const; // [tested]

  /// \brief Returns true if the given point is inside the sphere.
  bool Contains(const plSimdVec4f& vPoint) const; // [tested]

  /// \brief Returns whether the other sphere is completely inside this sphere.
  bool Contains(const plSimdBSphere& rhs) const; // [tested]

  /// \brief Checks whether the two objects overlap.
  bool Overlaps(const plSimdBSphere& rhs) const; // [tested]

  /// \brief Clamps the given position to the volume of the sphere. The resulting point will always be inside the sphere, but have the
  /// closest distance to the original point.
  [[nodiscard]] plSimdVec4f GetClampedPoint(const plSimdVec4f& vPoint); // [tested]

  [[nodiscard]] bool operator==(const plSimdBSphere& rhs) const; // [tested]
  [[nodiscard]] bool operator!=(const plSimdBSphere& rhs) const; // [tested]

public:
  plSimdVec4f m_CenterAndRadius;
};

#include <Foundation/SimdMath/Implementation/SimdBSphere_inl.h>

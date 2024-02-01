#pragma once

#include <Foundation/SimdMath/SimdBBox.h>

class PL_FOUNDATION_DLL plSimdBBoxSphere
{
public:
  PL_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize anything.
  plSimdBBoxSphere(); // [tested]

  /// \brief Constructs the bounds from the center position, the box half extends and the sphere radius.
  [[deprecated("Use MakeFromCenterExtents() instead.")]] plSimdBBoxSphere(const plSimdVec4f& vCenter, const plSimdVec4f& vBoxHalfExtents, const plSimdFloat& fSphereRadius); // [tested]

  /// \brief Constructs the bounds from the given box and sphere.
  [[deprecated("Use MakeFromBoxAndSphere() instead.")]] plSimdBBoxSphere(const plSimdBBox& box, const plSimdBSphere& sphere); // [tested]

  /// \brief Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  plSimdBBoxSphere(const plSimdBBox& box); // [tested]

  /// \brief Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  plSimdBBoxSphere(const plSimdBSphere& sphere); // [tested]

  /// \brief Creates an object with all zero values. These are valid bounds around the origin with no volume.
  [[nodiscard]] static plSimdBBoxSphere MakeZero();

  /// \brief Creates an 'invalid' object, ie one with negative extents/radius. Invalid objects can be made valid through ExpandToInclude().
  [[nodiscard]] static plSimdBBoxSphere MakeInvalid(); // [tested]

  /// \brief Creates an object from the given center point and extents.
  [[nodiscard]] static plSimdBBoxSphere MakeFromCenterExtents(const plSimdVec4f& vCenter, const plSimdVec4f& vBoxHalfExtents, const plSimdFloat& fSphereRadius);

  /// \brief Creates an object that contains all the provided points.
  [[nodiscard]] static plSimdBBoxSphere MakeFromPoints(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plSimdVec4f));

  /// \brief Creates an object from another bounding box.
  [[nodiscard]] static plSimdBBoxSphere MakeFromBox(const plSimdBBox& box);

  /// \brief Creates an object from another bounding sphere.
  [[nodiscard]] static plSimdBBoxSphere MakeFromSphere(const plSimdBSphere& sphere);

  /// \brief Creates an object from another bounding box and a sphere.
  [[nodiscard]] static plSimdBBoxSphere MakeFromBoxAndSphere(const plSimdBBox& box, const plSimdBSphere& sphere);


public:
  /// \brief Resets the bounds to an invalid state.
  [[deprecated("Use MakeInvalid() instead.")]] void SetInvalid(); // [tested]

  /// \brief Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Calculates the bounds from given set of points.
  [[deprecated("Use MakeFromPoints() instead.")]] void SetFromPoints(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plSimdVec4f)); // [tested]

  /// \brief Returns the bounding box.
  plSimdBBox GetBox() const; // [tested]

  /// \brief Returns the bounding sphere.
  plSimdBSphere GetSphere() const; // [tested]

  /// \brief Expands the bounds such that the given bounds are inside it.
  void ExpandToInclude(const plSimdBBoxSphere& rhs); // [tested]

  /// \brief Transforms the bounds in its local space.
  void Transform(const plSimdTransform& t); // [tested]

  /// \brief Transforms the bounds in its local space.
  void Transform(const plSimdMat4f& mMat); // [tested]

  [[nodiscard]] bool operator==(const plSimdBBoxSphere& rhs) const; // [tested]
  [[nodiscard]] bool operator!=(const plSimdBBoxSphere& rhs) const; // [tested]

public:
  plSimdVec4f m_CenterAndRadius;
  plSimdVec4f m_BoxHalfExtents;
};

#include <Foundation/SimdMath/Implementation/SimdBBoxSphere_inl.h>

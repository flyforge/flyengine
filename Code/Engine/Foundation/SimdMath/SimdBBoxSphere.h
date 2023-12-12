#pragma once

#include <Foundation/SimdMath/SimdBBox.h>

class PLASMA_FOUNDATION_DLL plSimdBBoxSphere
{
public:
  PLASMA_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize anything.
  plSimdBBoxSphere(); // [tested]

  /// \brief Constructs the bounds from the center position, the box half extends and the sphere radius.
  plSimdBBoxSphere(const plSimdVec4f& vCenter, const plSimdVec4f& vBoxHalfExtents, const plSimdFloat& fSphereRadius); // [tested]

  /// \brief Constructs the bounds from the given box and sphere.
  plSimdBBoxSphere(const plSimdBBox& box, const plSimdBSphere& sphere); // [tested]

  /// \brief Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  plSimdBBoxSphere(const plSimdBBox& box); // [tested]

  /// \brief Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  plSimdBBoxSphere(const plSimdBSphere& sphere); // [tested]

public:
  /// \brief Resets the bounds to an invalid state.
  void SetInvalid(); // [tested]

  /// \brief Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Calculates the bounds from given set of points.
  void SetFromPoints(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plSimdVec4f)); // [tested]

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


  bool operator==(const plSimdBBoxSphere& rhs) const; // [tested]
  bool operator!=(const plSimdBBoxSphere& rhs) const; // [tested]

public:
  plSimdVec4f m_CenterAndRadius;
  plSimdVec4f m_BoxHalfExtents;
};

#include <Foundation/SimdMath/Implementation/SimdBBoxSphere_inl.h>

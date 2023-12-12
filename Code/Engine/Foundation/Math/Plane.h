#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

/// \brief Describes on which side of a plane a point or an object is located.
struct plPositionOnPlane
{
  enum Enum
  {
    Back,     ///< Something is completely on the back side of a plane
    Front,    ///< Something is completely in front of a plane
    OnPlane,  ///< Something is lying completely on a plane (all points)
    Spanning, ///< Something is spanning a plane, i.e. some points are on the front and some on the back
  };
};

/// \brief A class that represents a mathematical plane.
template <typename Type>
struct plPlaneTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  PLASMA_DECLARE_POD_TYPE();

  using ComponentType = Type;

  // *** Data ***
public:
  plVec3Template<Type> m_vNormal;
  Type m_fNegDistance;


  // *** Constructors ***
public:
  /// \brief Default constructor. Does not initialize the plane.
  plPlaneTemplate(); // [tested]

  /// \brief Creates the plane-equation from a normal and a point on the plane.
  plPlaneTemplate(const plVec3Template<Type>& vNormal, const plVec3Template<Type>& vPointOnPlane); // [tested]

  /// \brief Creates the plane-equation from three points on the plane.
  plPlaneTemplate(const plVec3Template<Type>& v1, const plVec3Template<Type>& v2, const plVec3Template<Type>& v3); // [tested]

  /// \brief Creates the plane-equation from three points on the plane, given as an array.
  plPlaneTemplate(const plVec3Template<Type>* const pVertices); // [tested]

  /// \brief Creates the plane-equation from a set of unreliable points lying on the same plane. Some points might be equal or too close to each other
  /// for the typical algorithm.
  plPlaneTemplate(const plVec3Template<Type>* const pVertices, plUInt32 uiMaxVertices); // [tested]

#if PLASMA_ENABLED(PLASMA_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    PLASMA_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                               "all code-paths properly initialize this object.");
  }
#endif

  /// \brief Returns an plVec4 with the plane normal in x,y,z and the negative distance in w.
  plVec4Template<Type> GetAsVec4() const;

  /// \brief Creates the plane-equation from a normal and a point on the plane.
  void SetFromNormalAndPoint(const plVec3Template<Type>& vNormal, const plVec3Template<Type>& vPointOnPlane); // [tested]

  /// \brief Creates the plane-equation from three points on the plane.
  plResult SetFromPoints(const plVec3Template<Type>& v1, const plVec3Template<Type>& v2, const plVec3Template<Type>& v3); // [tested]

  /// \brief Creates the plane-equation from three points on the plane, given as an array.
  plResult SetFromPoints(const plVec3Template<Type>* const pVertices); // [tested]

  /// \brief Creates the plane-equation from a set of unreliable points lying on the same plane. Some points might be equal or too close to each other
  /// for the typical algorithm. Returns false, if no reliable set of points could be found. Does try to create a plane anyway.
  plResult SetFromPoints(const plVec3Template<Type>* const pVertices, plUInt32 uiMaxVertices); // [tested]

  /// \brief Creates a plane from two direction vectors that span the plane, and one point on it.
  plResult SetFromDirections(
    const plVec3Template<Type>& vTangent1, const plVec3Template<Type>& vTangent2, const plVec3Template<Type>& vPointOnPlane); // [tested]

  /// \brief Sets the plane to an invalid state (all zero).
  void SetInvalid(); // [tested]

  // *** Distance and Position ***
public:
  /// \brief Returns the distance of the point to the plane.
  Type GetDistanceTo(const plVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Returns the minimum distance that any of the given points had to the plane.
  ///
  /// 'Minimum' means the (non-absolute) distance of a point to the plane. So a point behind the plane will always have a 'lower distance'
  /// than a point in front of the plane, even if that is closer to the plane's surface.
  Type GetMinimumDistanceTo(const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plVec3Template<Type>)) const; // [tested]

  /// \brief Returns the minimum distance between given box and a plane
  Type GetMinimumDistanceTo(const plBoundingBoxTemplate<Type>& box) const; // [tested]

  /// \brief Returns the maximum distance between given box and a plane
  Type GetMaximumDistanceTo(const plBoundingBoxTemplate<Type>& box) const; // [tested]

  /// \brief Returns the minimum and maximum distance that any of the given points had to the plane.
  ///
  /// 'Minimum' (and 'maximum') means the (non-absolute) distance of a point to the plane. So a point behind the plane will always have a 'lower
  /// distance' than a point in front of the plane, even if that is closer to the plane's surface.
  void GetMinMaxDistanceTo(Type& out_fMin, Type& out_fMax, const plVec3Template<Type>* pPoints, plUInt32 uiNumPoints,
    plUInt32 uiStride = sizeof(plVec3Template<Type>)) const; // [tested]

  /// \brief Returns on which side of the plane the point lies.
  plPositionOnPlane::Enum GetPointPosition(const plVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Returns on which side of the plane the point lies.
  plPositionOnPlane::Enum GetPointPosition(const plVec3Template<Type>& vPoint, Type fPlaneHalfWidth) const; // [tested]

  /// \brief Returns on which side of the plane the set of points lies. Might be on both sides.
  plPositionOnPlane::Enum GetObjectPosition(const plVec3Template<Type>* const pPoints, plUInt32 uiVertices) const; // [tested]

  /// \brief Returns on which side of the plane the set of points lies. Might be on both sides.
  plPositionOnPlane::Enum GetObjectPosition(const plVec3Template<Type>* const pPoints, plUInt32 uiVertices, Type fPlaneHalfWidth) const; // [tested]

  /// \brief Returns on which side of the plane the sphere is located.
  plPositionOnPlane::Enum GetObjectPosition(const plBoundingSphereTemplate<Type>& sphere) const; // [tested]

  /// \brief Returns on which side of the plane the box is located.
  plPositionOnPlane::Enum GetObjectPosition(const plBoundingBoxTemplate<Type>& box) const; // [tested]

  /// \brief Projects a point onto a plane (along the planes normal).
  [[nodiscard]] const plVec3Template<Type> ProjectOntoPlane(const plVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Returns the mirrored point. E.g. on the other side of the plane, at the same distance.
  [[nodiscard]] const plVec3Template<Type> Mirror(const plVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Take the given direction vector and returns a modified one that is coplanar to the plane.
  const plVec3Template<Type> GetCoplanarDirection(const plVec3Template<Type>& vDirection) const; // [tested]

  // *** Comparisons ***
public:
  /// \brief Checks whether this plane and the other are identical.
  bool IsIdentical(const plPlaneTemplate<Type>& rhs) const; // [tested]

  /// \brief Checks whether this plane and the other are equal within some threshold.
  bool IsEqual(const plPlaneTemplate<Type>& rhs, Type fEpsilon = plMath::DefaultEpsilon<Type>()) const; // [tested]

  /// \brief Checks whether the plane has valid values (not NaN, normalized normal).
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Checks whether any component is Infinity.
  bool IsFinite() const; // [tested]

  // *** Modifications ***
public:
  /// \brief Transforms the plane with the given matrix.
  void Transform(const plMat3Template<Type>& m); // [tested]

  /// \brief Transforms the plane with the given matrix.
  void Transform(const plMat4Template<Type>& m); // [tested]

  /// \brief Negates Normal/Distance to switch which side of the plane is front and back.
  void Flip(); // [tested]

  /// \brief Negates Normal/Distance to switch which side of the plane is front and back. Returns true, if the plane had to be flipped.
  bool FlipIfNecessary(const plVec3Template<Type>& vPoint, bool bPlaneShouldFacePoint = true); // [tested]

  // *** Intersection Tests ***
public:
  /// \brief Returns true, if the ray hit the plane. The intersection time describes at which multiple of the ray direction the ray hit the plane.
  ///
  /// An intersection will be reported regardless of whether the ray starts 'behind' or 'in front of' the plane, as long as it points at it.
  /// \a vRayDir does not need to be normalized.\n
  /// out_vIntersection = vRayStartPos + out_fIntersection * vRayDir
  ///
  /// Intersections with \a out_fIntersection less than zero will be discarded and not reported as intersections.
  /// If such intersections are desired, use GetRayIntersectionBiDirectional instead.
  [[nodiscard]] bool GetRayIntersection(const plVec3Template<Type>& vRayStartPos, const plVec3Template<Type>& vRayDir, Type* out_pIntersectionDinstance = nullptr,
    plVec3Template<Type>* out_pIntersection = nullptr) const; // [tested]

  /// \brief Returns true, if the ray intersects the plane. Intersection time and point are stored in the out-parameters. Allows for intersections at
  /// negative times (shooting into the opposite direction).
  [[nodiscard]] bool GetRayIntersectionBiDirectional(const plVec3Template<Type>& vRayStartPos, const plVec3Template<Type>& vRayDir,
    Type* out_pIntersectionDistance = nullptr, plVec3Template<Type>* out_pIntersection = nullptr) const; // [tested]

  /// \brief Returns true, if there is any intersection with the plane between the line's start and end position. Returns the fraction along the line
  /// and the actual intersection point.
  [[nodiscard]] bool GetLineSegmentIntersection(const plVec3Template<Type>& vLineStartPos, const plVec3Template<Type>& vLineEndPos,
    Type* out_pHitFraction = nullptr, plVec3Template<Type>* out_pIntersection = nullptr) const; // [tested]

  /// \brief Computes the one point where all three planes intersect. Returns PLASMA_FAILURE if no such point exists.
  static plResult GetPlanesIntersectionPoint(const plPlaneTemplate<Type>& p0, const plPlaneTemplate<Type>& p1, const plPlaneTemplate<Type>& p2, plVec3Template<Type>& out_vResult); // [tested]

  // *** Helper Functions ***
public:
  /// \brief Returns three points from an unreliable set of points, that reliably form a plane. Returns false, if there are none.
  static plResult FindSupportPoints(const plVec3Template<Type>* const pVertices, plInt32 iMaxVertices, plInt32& out_i1, plInt32& out_i2, plInt32& out_i3); // [tested]
};

/// \brief Checks whether this plane and the other are identical.
template <typename Type>
bool operator==(const plPlaneTemplate<Type>& lhs, const plPlaneTemplate<Type>& rhs); // [tested]

/// \brief Checks whether this plane and the other are not identical.
template <typename Type>
bool operator!=(const plPlaneTemplate<Type>& lhs, const plPlaneTemplate<Type>& rhs); // [tested]

#include <Foundation/Math/Implementation/Plane_inl.h>

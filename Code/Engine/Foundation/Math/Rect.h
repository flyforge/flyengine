#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Vec2.h>

/// \brief A simple rectangle class templated on the type for x, y and width, height.
///
template <typename Type>
class plRectTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  PLASMA_DECLARE_POD_TYPE();

  // *** Data ***
public:
  Type x;
  Type y;

  Type width;
  Type height;

  // *** Constructors ***
public:
  /// \brief Default constructor does not initialize the data.
  plRectTemplate();

  /// \brief Constructor to set all values.
  plRectTemplate(Type x, Type y, Type width, Type height);

  /// \brief Initializes x and y with zero, width and height with the given values.
  plRectTemplate(Type width, Type height);

  /// The smaller value along x.
  Type Left() const { return x; }

  /// The larger value along x.
  Type Right() const { return x + width; }

  /// The smaller value along y.
  Type Top() const { return y; }

  /// The larger value along y.
  Type Bottom() const { return y + height; }

  /// The smaller value along x. Same as Left().
  Type GetX1() const { return x; }

  /// The larger value along x. Same as Right().
  Type GetX2() const { return x + width; }

  /// The smaller value along y. Same as Top().
  Type GetY1() const { return y; }

  /// The larger value along y. Same as Bottom().
  Type GetY2() const { return y + height; }

  // *** Common Functions ***
public:
  bool operator==(const plRectTemplate<Type>& rhs) const;

  bool operator!=(const plRectTemplate<Type>& rhs) const;

  /// \brief Sets the rect to invalid values.
  ///
  /// IsValid() will return false afterwards.
  /// It is possible to make an invalid rect valid using ExpandToInclude().
  void SetInvalid();

  /// \brief Checks whether the position and size contain valid values.
  bool IsValid() const;

  /// \brief Returns true if the area of the rectangle is non zero
  bool HasNonZeroArea() const;

  /// \brief Returns true if the rectangle contains the provided point
  bool Contains(const plVec2Template<Type>& vPoint) const;

  /// \brief Returns true if the rectangle overlaps the provided rectangle.
  /// Also returns true if the rectangles are contained within each other completely(no intersecting edges).
  bool Overlaps(const plRectTemplate<Type>& other) const;

  /// \brief Extends this rectangle so that the provided rectangle is completely contained within it.
  void ExpandToInclude(const plRectTemplate<Type>& other);

  /// \brief Clips this rect so that it is fully inside the provided rectangle.
  void Clip(const plRectTemplate<Type>& clipRect);

  /// \brief The given point is clamped to the area of the rect, i.e. it will be either inside the rect or on its edge and it will have the closest
  /// possible distance to the original point.
  const plVec2Template<Type> GetClampedPoint(const plVec2Template<Type>& vPoint) const;

  void SetIntersection(const plRectTemplate<Type>& r0, const plRectTemplate<Type>& r1);

  void SetUnion(const plRectTemplate<Type>& r0, const plRectTemplate<Type>& r1);

  /// \brief Moves the rectangle
  void Translate(Type tX, Type tY);

  /// \brief Scales width and height, and moves the position as well.
  void Scale(Type sX, Type sY);
};

#include <Foundation/Math/Implementation/Rect_inl.h>

using plRectU32 = plRectTemplate<plUInt32>;
using plRectU16 = plRectTemplate<plUInt16>;
using plRectI32 = plRectTemplate<plInt32>;
using plRectI16 = plRectTemplate<plInt16>;
using plRectFloat = plRectTemplate<float>;
using plRectDouble = plRectTemplate<double>;

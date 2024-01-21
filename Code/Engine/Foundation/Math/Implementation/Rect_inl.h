#pragma once

template <typename Type>
PLASMA_ALWAYS_INLINE plRectTemplate<Type>::plRectTemplate() = default;

template <typename Type>
PLASMA_ALWAYS_INLINE plRectTemplate<Type>::plRectTemplate(Type x, Type y, Type width, Type height)
  : x(x)
  , y(y)
  , width(width)
  , height(height)
{
}

template <typename Type>
PLASMA_ALWAYS_INLINE plRectTemplate<Type>::plRectTemplate(Type width, Type height)
  : x(0)
  , y(0)
  , width(width)
  , height(height)
{
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool plRectTemplate<Type>::operator==(const plRectTemplate<Type>& rhs) const
{
  return x == rhs.x && y == rhs.y && width == rhs.width && height == rhs.height;
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool plRectTemplate<Type>::operator!=(const plRectTemplate<Type>& rhs) const
{
  return !(*this == rhs);
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool plRectTemplate<Type>::HasNonZeroArea() const
{
  return (width > 0) && (height > 0);
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool plRectTemplate<Type>::Contains(const plVec2Template<Type>& vPoint) const
{
  if (vPoint.x >= x && vPoint.x <= Right())
  {
    if (vPoint.y >= y && vPoint.y <= Bottom())
      return true;
  }

  return false;
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool plRectTemplate<Type>::Overlaps(const plRectTemplate<Type>& other) const
{
  if (x < other.Right() && Right() > other.x && y < other.Bottom() && Bottom() > other.y)
    return true;

  return false;
}

template <typename Type>
void plRectTemplate<Type>::ExpandToInclude(const plRectTemplate<Type>& other)
{
  Type thisRight = Right();
  Type thisBottom = Bottom();

  if (other.x < x)
    x = other.x;

  if (other.y < y)
    y = other.y;

  if (other.Right() > thisRight)
    width = other.Right() - x;
  else
    width = thisRight - x;

  if (other.Bottom() > thisBottom)
    height = other.Bottom() - y;
  else
    height = thisBottom - y;
}

template <typename Type>
void plRectTemplate<Type>::ExpandToInclude(const plVec2Template<Type>& other)
{
  Type thisRight = Right();
  Type thisBottom = Bottom();

  if (other.x < x)
    x = other.x;

  if (other.y < y)
    y = other.y;

  if (other.x > thisRight)
    width = other.x - x;
  else
    width = thisRight - x;

  if (other.y > thisBottom)
    height = other.y - y;
  else
    height = thisBottom - y;
}

template <typename Type>
void plRectTemplate<Type>::Grow(Type xy)
{
  x -= xy;
  y -= xy;
  width += xy * 2;
  height += xy * 2;
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plRectTemplate<Type>::Clip(const plRectTemplate<Type>& clipRect)
{
  Type newLeft = plMath::Max<Type>(x, clipRect.x);
  Type newTop = plMath::Max<Type>(y, clipRect.y);

  Type newRight = plMath::Min<Type>(Right(), clipRect.Right());
  Type newBottom = plMath::Min<Type>(Bottom(), clipRect.Bottom());

  x = newLeft;
  y = newTop;
  width = newRight - newLeft;
  height = newBottom - newTop;
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plRectTemplate<Type>::SetInvalid()
{
  /// \test This is new

  const Type fLargeValue = plMath::MaxValue<Type>() / 2;
  x = fLargeValue;
  y = fLargeValue;
  width = -fLargeValue;
  height = -fLargeValue;
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool plRectTemplate<Type>::IsValid() const
{
  /// \test This is new

  return width >= 0 && height >= 0;
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plVec2Template<Type> plRectTemplate<Type>::GetClampedPoint(const plVec2Template<Type>& vPoint) const
{
  /// \test This is new

  return plVec2Template<Type>(plMath::Clamp(vPoint.x, Left(), Right()), plMath::Clamp(vPoint.y, Top(), Bottom()));
}

template <typename Type>
void plRectTemplate<Type>::SetIntersection(const plRectTemplate<Type>& r0, const plRectTemplate<Type>& r1)
{
  /// \test This is new

  Type x1 = plMath::Max(r0.GetX1(), r1.GetX1());
  Type y1 = plMath::Max(r0.GetY1(), r1.GetY1());
  Type x2 = plMath::Min(r0.GetX2(), r1.GetX2());
  Type y2 = plMath::Min(r0.GetY2(), r1.GetY2());

  x = x1;
  y = y1;
  width = x2 - x1;
  height = y2 - y1;
}

template <typename Type>
void plRectTemplate<Type>::SetUnion(const plRectTemplate<Type>& r0, const plRectTemplate<Type>& r1)
{
  /// \test This is new

  Type x1 = plMath::Min(r0.GetX1(), r1.GetX1());
  Type y1 = plMath::Min(r0.GetY1(), r1.GetY1());
  Type x2 = plMath::Max(r0.GetX2(), r1.GetX2());
  Type y2 = plMath::Max(r0.GetY2(), r1.GetY2());

  x = x1;
  y = y1;
  width = x2 - x1;
  height = y2 - y1;
}

template <typename Type>
void plRectTemplate<Type>::Translate(Type tX, Type tY)
{
  /// \test This is new

  x += tX;
  y += tY;
}

template <typename Type>
void plRectTemplate<Type>::Scale(Type sX, Type sY)
{
  /// \test This is new

  x *= sX;
  y *= sY;
  width *= sX;
  height *= sY;
}

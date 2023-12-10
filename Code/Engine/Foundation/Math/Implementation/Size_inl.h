#pragma once

template <typename Type>
PLASMA_ALWAYS_INLINE plSizeTemplate<Type>::plSizeTemplate() = default;

template <typename Type>
PLASMA_ALWAYS_INLINE plSizeTemplate<Type>::plSizeTemplate(Type width, Type height)
  : width(width)
  , height(height)
{
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool plSizeTemplate<Type>::HasNonZeroArea() const
{
  return (width > 0) && (height > 0);
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool operator==(const plSizeTemplate<Type>& v1, const plSizeTemplate<Type>& v2)
{
  return v1.height == v2.height && v1.width == v2.width;
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool operator!=(const plSizeTemplate<Type>& v1, const plSizeTemplate<Type>& v2)
{
  return v1.height != v2.height || v1.width != v2.width;
}

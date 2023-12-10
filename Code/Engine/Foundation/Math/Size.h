#pragma once

#include <Foundation/Basics.h>

/// \brief A simple size class templated on the type for width and height.
///
template <typename Type>
class plSizeTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  PLASMA_DECLARE_POD_TYPE();

  // *** Data ***
public:
  Type width;
  Type height;

  // *** Constructors ***
public:
  /// \brief Default constructor does not initialize the data.
  plSizeTemplate();

  /// \brief Constructor to set all values.
  plSizeTemplate(Type width, Type height);

  // *** Common Functions ***
public:
  /// \brief Returns true if the area described by the size is non zero
  bool HasNonZeroArea() const;
};

template <typename Type>
bool operator==(const plSizeTemplate<Type>& v1, const plSizeTemplate<Type>& v2);

template <typename Type>
bool operator!=(const plSizeTemplate<Type>& v1, const plSizeTemplate<Type>& v2);

#include <Foundation/Math/Implementation/Size_inl.h>

using plSizeU32 = plSizeTemplate<plUInt32>;
using plSizeFloat = plSizeTemplate<float>;
using plSizeDouble = plSizeTemplate<double>;

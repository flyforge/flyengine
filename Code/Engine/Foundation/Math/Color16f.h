#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Float16.h>

/// \brief A 16bit per channel float color storage format.
///
/// For any calculations or conversions use plColor.
/// \see plColor
class PLASMA_FOUNDATION_DLL plColorLinear16f
{
public:
  // Means that colors can be copied using memcpy instead of copy construction.
  PLASMA_DECLARE_POD_TYPE();

  // *** Data ***
public:
  plFloat16 r;
  plFloat16 g;
  plFloat16 b;
  plFloat16 a;

  // *** Constructors ***
public:
  /// \brief default-constructed color is uninitialized (for speed)
  plColorLinear16f(); // [tested]

  /// \brief Initializes the color with r, g, b, a
  plColorLinear16f(plFloat16 r, plFloat16 g, plFloat16 b, plFloat16 a); // [tested]

  /// \brief Initializes the color with plColor
  plColorLinear16f(const plColor& color); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  // *** Functions ***
public:
  /// \brief Conversion to plColor.
  plColor ToLinearFloat() const; // [tested]

  /// \brief Conversion to const plFloat16*.
  const plFloat16* GetData() const { return &r; }

  /// \brief Conversion to plFloat16* - use with care!
  plFloat16* GetData() { return &r; }
};

#include <Foundation/Math/Implementation/Color16f_inl.h>

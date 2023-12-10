#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Math.h>

/// \brief A 8bit per channel color storage format with undefined encoding. It is up to the user to reinterpret as a gamma or linear space
/// color.
///
/// \see plColorLinearUB
/// \see plColorGammaUB
class PLASMA_FOUNDATION_DLL plColorBaseUB
{
public:
  PLASMA_DECLARE_POD_TYPE();

  plUInt8 r;
  plUInt8 g;
  plUInt8 b;
  plUInt8 a;

  /// \brief Default-constructed color is uninitialized (for speed)
  plColorBaseUB() = default;

  /// \brief Initializes the color with r, g, b, a
  plColorBaseUB(plUInt8 r, plUInt8 g, plUInt8 b, plUInt8 a = 255);

  /// \brief Conversion to const plUInt8*.
  const plUInt8* GetData() const { return &r; }

  /// \brief Conversion to plUInt8*
  plUInt8* GetData() { return &r; }
};

PLASMA_CHECK_AT_COMPILETIME(sizeof(plColorBaseUB) == 4);

/// \brief A 8bit per channel unsigned normalized (values interpreted as 0-1) color storage format that represents colors in linear space.
///
/// For any calculations or conversions use plColor.
/// \see plColor
class PLASMA_FOUNDATION_DLL plColorLinearUB : public plColorBaseUB
{
public:
  PLASMA_DECLARE_POD_TYPE();

  /// \brief Default-constructed color is uninitialized (for speed)
  plColorLinearUB() = default; // [tested]

  /// \brief Initializes the color with r, g, b, a
  plColorLinearUB(plUInt8 r, plUInt8 g, plUInt8 b, plUInt8 a = 255); // [tested]

  /// \brief Initializes the color with plColor.
  /// Assumes that the given color is normalized.
  /// \see plColor::IsNormalized
  plColorLinearUB(const plColor& color); // [tested]

  /// \brief Initializes the color with plColor.
  void operator=(const plColor& color); // [tested]

  /// \brief Converts this color to plColor.
  plColor ToLinearFloat() const; // [tested]
};

PLASMA_CHECK_AT_COMPILETIME(sizeof(plColorLinearUB) == 4);

/// \brief A 8bit per channel unsigned normalized (values interpreted as 0-1) color storage format that represents colors in gamma space.
///
/// For any calculations or conversions use plColor.
/// \see plColor
class PLASMA_FOUNDATION_DLL plColorGammaUB : public plColorBaseUB
{
public:
  PLASMA_DECLARE_POD_TYPE();

  /// \brief Default-constructed color is uninitialized (for speed)
  plColorGammaUB() = default;

  /// \brief Copies the color values. RGB are assumed to be in Gamma space.
  plColorGammaUB(plUInt8 uiGammaRed, plUInt8 uiGammaGreen, plUInt8 uiGammaBlue, plUInt8 uiLinearAlpha = 255); // [tested]

  /// \brief Initializes the color with plColor. Converts the linear space color to gamma space.
  /// Assumes that the given color is normalized.
  /// \see plColor::IsNormalized
  plColorGammaUB(const plColor& color); // [tested]

  /// \brief Initializes the color with plColor. Converts the linear space color to gamma space.
  void operator=(const plColor& color); // [tested]

  /// \brief Converts this color to plColor.
  plColor ToLinearFloat() const;
};

PLASMA_CHECK_AT_COMPILETIME(sizeof(plColorGammaUB) == 4);


#include <Foundation/Math/Implementation/Color8UNorm_inl.h>

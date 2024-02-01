#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec4.h>

/// \brief plColor represents an RGBA color in linear color space. Values are stored as float, allowing HDR values and full precision color
/// modifications.
///
/// plColor is the central class to handle colors throughout the engine. With floating point precision it can handle any value, including HDR colors.
/// Since it is stored in linear space, doing color transformations (e.g. adding colors or multiplying them) work as expected.
///
/// When you need to pass colors to the GPU you have multiple options.
///   * If you can spare the bandwidth, you should prefer to use floating point formats, e.g. the same as plColor on the CPU.
///   * If you need higher precision and HDR values, you can use plColorLinear16f as a storage format with only half the memory footprint.
///   * If you need to preserve memory and LDR values are sufficient, you should use plColorGammaUB. This format uses 8 Bit per pixel
///     but stores colors in Gamma space, resulting in higher precision in the range that the human eye can distinguish better.
///     However, when you store a color in Gamma space, you need to make sure to convert it back to linear space before doing ANY computations
///     with it. E.g. your shader needs to convert the color.
///   * You can also use 8 Bit per pixel with a linear color space by using plColorLinearUB, however this may give very noticeable precision loss.
///
/// When working with color in your code, be aware to always use the correct class to handle color conversions properly.
/// E.g. when you hardcode a color in source code, you might go to a Paint program, pick a nice color and then type that value into the
/// source code. Note that ALL colors that you see on screen are implicitly in sRGB / Gamma space. That means you should do the following cast:\n
///
/// \code
///   plColor linear = plColorGammaUB(100, 149, 237);
/// \endcode
///
/// This will automatically convert the color from Gamma to linear space. From there on all mathematical operations are possible.
///
/// The inverse has to be done when you want to present the value of a color in a UI:
///
/// \code
///   plColorGammaUB gamma = plColor(0.39f, 0.58f, 0.93f);
/// \endcode
///
/// Now the integer values in \a gamma can be used to e.g. populate a color picker and the color displayed on screen will show up the same, as
/// in a gamma correct 3D rendering.
///
///
///
/// The predefined colors can be seen at http://www.w3schools.com/colors/colors_names.asp
class PL_FOUNDATION_DLL plColor
{
public:
  PL_DECLARE_POD_TYPE();

  // *** Predefined Colors ***
public:
  static const plColor AliceBlue;            ///< #F0F8FF
  static const plColor AntiqueWhite;         ///< #FAEBD7
  static const plColor Aqua;                 ///< #00FFFF
  static const plColor Aquamarine;           ///< #7FFFD4
  static const plColor Azure;                ///< #F0FFFF
  static const plColor Beige;                ///< #F5F5DC
  static const plColor Bisque;               ///< #FFE4C4
  static const plColor Black;                ///< #000000
  static const plColor BlanchedAlmond;       ///< #FFEBCD
  static const plColor Blue;                 ///< #0000FF
  static const plColor BlueViolet;           ///< #8A2BE2
  static const plColor Brown;                ///< #A52A2A
  static const plColor BurlyWood;            ///< #DEB887
  static const plColor CadetBlue;            ///< #5F9EA0
  static const plColor Chartreuse;           ///< #7FFF00
  static const plColor Chocolate;            ///< #D2691E
  static const plColor Coral;                ///< #FF7F50
  static const plColor CornflowerBlue;       ///< #6495ED  The original!
  static const plColor Cornsilk;             ///< #FFF8DC
  static const plColor Crimson;              ///< #DC143C
  static const plColor Cyan;                 ///< #00FFFF
  static const plColor DarkBlue;             ///< #00008B
  static const plColor DarkCyan;             ///< #008B8B
  static const plColor DarkGoldenRod;        ///< #B8860B
  static const plColor DarkGray;             ///< #A9A9A9
  static const plColor DarkGrey;             ///< #A9A9A9
  static const plColor DarkGreen;            ///< #006400
  static const plColor DarkKhaki;            ///< #BDB76B
  static const plColor DarkMagenta;          ///< #8B008B
  static const plColor DarkOliveGreen;       ///< #556B2F
  static const plColor DarkOrange;           ///< #FF8C00
  static const plColor DarkOrchid;           ///< #9932CC
  static const plColor DarkRed;              ///< #8B0000
  static const plColor DarkSalmon;           ///< #E9967A
  static const plColor DarkSeaGreen;         ///< #8FBC8F
  static const plColor DarkSlateBlue;        ///< #483D8B
  static const plColor DarkSlateGray;        ///< #2F4F4F
  static const plColor DarkSlateGrey;        ///< #2F4F4F
  static const plColor DarkTurquoise;        ///< #00CED1
  static const plColor DarkViolet;           ///< #9400D3
  static const plColor DeepPink;             ///< #FF1493
  static const plColor DeepSkyBlue;          ///< #00BFFF
  static const plColor DimGray;              ///< #696969
  static const plColor DimGrey;              ///< #696969
  static const plColor DodgerBlue;           ///< #1E90FF
  static const plColor FireBrick;            ///< #B22222
  static const plColor FloralWhite;          ///< #FFFAF0
  static const plColor ForestGreen;          ///< #228B22
  static const plColor Fuchsia;              ///< #FF00FF
  static const plColor Gainsboro;            ///< #DCDCDC
  static const plColor GhostWhite;           ///< #F8F8FF
  static const plColor Gold;                 ///< #FFD700
  static const plColor GoldenRod;            ///< #DAA520
  static const plColor Gray;                 ///< #808080
  static const plColor Grey;                 ///< #808080
  static const plColor Green;                ///< #008000
  static const plColor GreenYellow;          ///< #ADFF2F
  static const plColor HoneyDew;             ///< #F0FFF0
  static const plColor HotPink;              ///< #FF69B4
  static const plColor IndianRed;            ///< #CD5C5C
  static const plColor Indigo;               ///< #4B0082
  static const plColor Ivory;                ///< #FFFFF0
  static const plColor Khaki;                ///< #F0E68C
  static const plColor Lavender;             ///< #E6E6FA
  static const plColor LavenderBlush;        ///< #FFF0F5
  static const plColor LawnGreen;            ///< #7CFC00
  static const plColor LemonChiffon;         ///< #FFFACD
  static const plColor LightBlue;            ///< #ADD8E6
  static const plColor LightCoral;           ///< #F08080
  static const plColor LightCyan;            ///< #E0FFFF
  static const plColor LightGoldenRodYellow; ///< #FAFAD2
  static const plColor LightGray;            ///< #D3D3D3
  static const plColor LightGrey;            ///< #D3D3D3
  static const plColor LightGreen;           ///< #90EE90
  static const plColor LightPink;            ///< #FFB6C1
  static const plColor LightSalmon;          ///< #FFA07A
  static const plColor LightSeaGreen;        ///< #20B2AA
  static const plColor LightSkyBlue;         ///< #87CEFA
  static const plColor LightSlateGray;       ///< #778899
  static const plColor LightSlateGrey;       ///< #778899
  static const plColor LightSteelBlue;       ///< #B0C4DE
  static const plColor LightYellow;          ///< #FFFFE0
  static const plColor Lime;                 ///< #00FF00
  static const plColor LimeGreen;            ///< #32CD32
  static const plColor Linen;                ///< #FAF0E6
  static const plColor Magenta;              ///< #FF00FF
  static const plColor Maroon;               ///< #800000
  static const plColor MediumAquaMarine;     ///< #66CDAA
  static const plColor MediumBlue;           ///< #0000CD
  static const plColor MediumOrchid;         ///< #BA55D3
  static const plColor MediumPurple;         ///< #9370DB
  static const plColor MediumSeaGreen;       ///< #3CB371
  static const plColor MediumSlateBlue;      ///< #7B68EE
  static const plColor MediumSpringGreen;    ///< #00FA9A
  static const plColor MediumTurquoise;      ///< #48D1CC
  static const plColor MediumVioletRed;      ///< #C71585
  static const plColor MidnightBlue;         ///< #191970
  static const plColor MintCream;            ///< #F5FFFA
  static const plColor MistyRose;            ///< #FFE4E1
  static const plColor Moccasin;             ///< #FFE4B5
  static const plColor NavajoWhite;          ///< #FFDEAD
  static const plColor Navy;                 ///< #000080
  static const plColor OldLace;              ///< #FDF5E6
  static const plColor Olive;                ///< #808000
  static const plColor OliveDrab;            ///< #6B8E23
  static const plColor Orange;               ///< #FFA500
  static const plColor OrangeRed;            ///< #FF4500
  static const plColor Orchid;               ///< #DA70D6
  static const plColor PaleGoldenRod;        ///< #EEE8AA
  static const plColor PaleGreen;            ///< #98FB98
  static const plColor PaleTurquoise;        ///< #AFEEEE
  static const plColor PaleVioletRed;        ///< #DB7093
  static const plColor PapayaWhip;           ///< #FFEFD5
  static const plColor PeachPuff;            ///< #FFDAB9
  static const plColor Peru;                 ///< #CD853F
  static const plColor Pink;                 ///< #FFC0CB
  static const plColor Plum;                 ///< #DDA0DD
  static const plColor PowderBlue;           ///< #B0E0E6
  static const plColor Purple;               ///< #800080
  static const plColor RebeccaPurple;        ///< #663399
  static const plColor Red;                  ///< #FF0000
  static const plColor RosyBrown;            ///< #BC8F8F
  static const plColor RoyalBlue;            ///< #4169E1
  static const plColor SaddleBrown;          ///< #8B4513
  static const plColor Salmon;               ///< #FA8072
  static const plColor SandyBrown;           ///< #F4A460
  static const plColor SeaGreen;             ///< #2E8B57
  static const plColor SeaShell;             ///< #FFF5EE
  static const plColor Sienna;               ///< #A0522D
  static const plColor Silver;               ///< #C0C0C0
  static const plColor SkyBlue;              ///< #87CEEB
  static const plColor SlateBlue;            ///< #6A5ACD
  static const plColor SlateGray;            ///< #708090
  static const plColor SlateGrey;            ///< #708090
  static const plColor Snow;                 ///< #FFFAFA
  static const plColor SpringGreen;          ///< #00FF7F
  static const plColor SteelBlue;            ///< #4682B4
  static const plColor Tan;                  ///< #D2B48C
  static const plColor Teal;                 ///< #008080
  static const plColor Thistle;              ///< #D8BFD8
  static const plColor Tomato;               ///< #FF6347
  static const plColor Turquoise;            ///< #40E0D0
  static const plColor Violet;               ///< #EE82EE
  static const plColor Wheat;                ///< #F5DEB3
  static const plColor White;                ///< #FFFFFF
  static const plColor WhiteSmoke;           ///< #F5F5F5
  static const plColor Yellow;               ///< #FFFF00
  static const plColor YellowGreen;          ///< #9ACD32

  // *** Data ***
public:
  float r;
  float g;
  float b;
  float a;

  // *** Static Functions ***
public:
  /// \brief Returns a color with all four RGBA components set to Not-A-Number (NaN).
  [[nodiscard]] static plColor MakeNaN();

  /// \brief Returns a color with all four RGBA components set to zero. This is different to plColor::Black, which has alpha still set to 1.0.
  [[nodiscard]] static plColor MakeZero();

  /// \brief Returns a color with the given r, g, b, a values. The values must be given in a linear color space.
  [[nodiscard]] static plColor MakeRGBA(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha = 1.0f);

  // *** Constructors ***
public:
  /// \brief default-constructed color is uninitialized (for speed)
  plColor(); // [tested]

  /// \brief Initializes the color with r, g, b, a. The color values must be given in a linear color space.
  ///
  /// To initialize the color from a Gamma color space, e.g. when using a color value that was determined with a color picker,
  /// use the constructor that takes a plColorGammaUB object for initialization.
  constexpr plColor(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha = 1.0f); // [tested]

  /// \brief Initializes this color from a plColorLinearUB object.
  ///
  /// Prefer to either use linear colors with floating point precision, or to use plColorGammaUB for 8 bit per pixel colors in gamma space.
  plColor(const plColorLinearUB& cc); // [tested]

  /// \brief Initializes this color from a plColorGammaUB object.
  ///
  /// This should be the preferred method when hard-coding colors in source code.
  plColor(const plColorGammaUB& cc); // [tested]

#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    PL_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                               "all code-paths properly initialize this object.");
  }
#endif

  /// \brief Sets the RGB components, ignores alpha.
  void SetRGB(float fLinearRed, float fLinearGreen, float fLinearBlue); // [tested]

  /// \brief Sets all four RGBA components.
  void SetRGBA(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha = 1.0f); // [tested]

  /// \brief Returns a color created from the kelvin temperature. https://wikipedia.org/wiki/Color_temperature
  /// Originally inspired from https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html
  /// But with heavy modification to better fit the mapping shown out in https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
  /// Physically accurate clipping points are 6580K for Red and 6560K for G and B. but approximated to 6570k for all to give a better mapping.
  static plColor MakeFromKelvin(plUInt32 uiKelvin);
  // *** Conversion Operators/Functions ***
public:
  /// \brief Sets this color from a HSV (hue, saturation, value) format.
  ///
  /// \a hue is in range [0; 360], \a sat and \a val are in range [0; 1]
  [[nodiscard]] static plColor MakeHSV(float fHue, float fSat, float fVal); // [tested]

  /// \brief Converts the color part to HSV format.
  ///
  /// \a hue is in range [0; 360], \a sat and \a val are in range [0; 1]
  void GetHSV(float& out_fHue, float& out_fSat, float& out_fValue) const; // [tested]

  /// \brief Conversion to const float*
  const float* GetData() const { return &r; }

  /// \brief Conversion to float*
  float* GetData() { return &r; }

  /// \brief Returns the 4 color values packed in an plVec4
  const plVec4 GetAsVec4() const;

  /// \brief Helper function to convert a float color value from gamma space to linear color space.
  static float GammaToLinear(float fGamma); // [tested]
  /// \brief Helper function to convert a float color value from linear space to gamma color space.
  static float LinearToGamma(float fGamma); // [tested]

  /// \brief Helper function to convert a float RGB color value from gamma space to linear color space.
  static plVec3 GammaToLinear(const plVec3& vGamma); // [tested]
  /// \brief Helper function to convert a float RGB color value from linear space to gamma color space.
  static plVec3 LinearToGamma(const plVec3& vGamma); // [tested]

  // *** Color specific functions ***
public:
  /// \brief Returns if the color is in the Range [0; 1] on all 4 channels.
  bool IsNormalized() const; // [tested]

  /// \brief Calculates the average of the RGB channels.
  float CalcAverageRGB() const;

  /// \brief Computes saturation.
  float GetSaturation() const; // [tested]

  /// \brief Computes the perceived luminance. Assumes linear color space (http://en.wikipedia.org/wiki/Luminance_%28relative%29).
  float GetLuminance() const; /// [tested]

  /// \brief Performs a simple (1.0 - color) inversion on all four channels.
  ///
  /// Using this function on non-normalized colors will lead to negative results.
  /// \see plColor IsNormalized
  plColor GetInvertedColor() const; // [tested]

  /// \brief Calculates the complementary color for this color (hue shifted by 180 degrees). The complementary color will have the same alpha.
  plColor GetComplementaryColor() const; // [tested]

  /// \brief Multiplies the given factor into red, green and blue, but not alpha.
  void ScaleRGB(float fFactor);

  /// \brief Multiplies the given factor into red, green, blue and also alpha.
  void ScaleRGBA(float fFactor);

  /// \brief Returns 1 for an LDR color (all ´RGB components < 1). Otherwise the value of the largest component. Ignores alpha.
  float ComputeHdrMultiplier() const;

  /// \brief Returns the base-2 logarithm of ComputeHdrMultiplier().
  /// 0 for LDR colors, +1, +2, etc. for HDR colors.
  float ComputeHdrExposureValue() const;

  /// \brief Raises 2 to the power \a ev and multiplies RGB with that factor.
  void ApplyHdrExposureValue(float fEv);

  /// \brief If this is an HDR color, the largest component value is used to normalize RGB to LDR range. Alpha is unaffected.
  void NormalizeToLdrRange();

  /// \brief Returns a darker color by converting the color to HSV, dividing the *value* by fFactor and converting it back.
  plColor GetDarker(float fFactor = 2.0f) const;

  // *** Numeric properties ***
public:
  /// \brief Returns true, if any of \a r, \a g, \a b or \a a is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Checks that all components are finite numbers.
  bool IsValid() const; // [tested]

  // *** Operators ***
public:
  /// \brief Converts the color from plColorLinearUB to linear float values.
  void operator=(const plColorLinearUB& cc); // [tested]

  /// \brief Converts the color from plColorGammaUB to linear float values. Gamma is correctly converted to linear space.
  void operator=(const plColorGammaUB& cc); // [tested]

  /// \brief Adds \a rhs component-wise to this color.
  void operator+=(const plColor& rhs); // [tested]

  /// \brief Subtracts \a rhs component-wise from this vector.
  void operator-=(const plColor& rhs); // [tested]

  /// \brief Multiplies \a rhs component-wise with this color.
  void operator*=(const plColor& rhs); // [tested]

  /// \brief Multiplies all components of this color with f.
  void operator*=(float f); // [tested]

  /// \brief Divides all components of this color by f.
  void operator/=(float f); // [tested]

  /// \brief Transforms the RGB components by the matrix. Alpha has no influence on the computation and will stay unmodified. The fourth row of the
  /// matrix is ignored.
  ///
  /// This operation can be used to do basic color correction.
  void operator*=(const plMat4& rhs); // [tested]

  /// \brief Equality Check (bitwise). Only compares RGB, ignores Alpha.
  bool IsIdenticalRGB(const plColor& rhs) const; // [tested]

  /// \brief Equality Check (bitwise). Compares all four components.
  bool IsIdenticalRGBA(const plColor& rhs) const; // [tested]

  /// \brief Equality Check with epsilon. Only compares RGB, ignores Alpha.
  bool IsEqualRGB(const plColor& rhs, float fEpsilon) const; // [tested]

  /// \brief Equality Check with epsilon. Compares all four components.
  bool IsEqualRGBA(const plColor& rhs, float fEpsilon) const; // [tested]

  /// \brief Returns the current color but with changes the alpha value to the given value.
  plColor WithAlpha(float fAlpha) const;

  /// \brief Packs the 4 color values as uint8 into a single uint32 with A in the least significant bits and R in the most significant ones.
  [[nodiscard]] plUInt32 ToRGBA8() const;

  /// \brief Packs the 4 color values as uint8 into a single uint32 with R in the least significant bits and A in the most significant ones.
  [[nodiscard]] plUInt32 ToABGR8() const;
};

// *** Operators ***

/// \brief Component-wise addition.
const plColor operator+(const plColor& c1, const plColor& c2); // [tested]

/// \brief Component-wise subtraction.
const plColor operator-(const plColor& c1, const plColor& c2); // [tested]

/// \brief Component-wise multiplication.
const plColor operator*(const plColor& c1, const plColor& c2); // [tested]

/// \brief Returns a scaled color.
const plColor operator*(float f, const plColor& c); // [tested]

/// \brief Returns a scaled color. Will scale all components.
const plColor operator*(const plColor& c, float f); // [tested]

/// \brief Returns a scaled color. Will scale all components.
const plColor operator/(const plColor& c, float f); // [tested]

/// \brief Transforms the RGB components by the matrix. Alpha has no influence on the computation and will stay unmodified. The fourth row of the
/// matrix is ignored.
///
/// This operation can be used to do basic color correction.
const plColor operator*(const plMat4& lhs, const plColor& rhs); // [tested]

/// \brief Returns true, if both colors are identical in all components.
bool operator==(const plColor& c1, const plColor& c2); // [tested]

/// \brief Returns true, if both colors are not identical in all components.
bool operator!=(const plColor& c1, const plColor& c2); // [tested]

/// \brief Strict weak ordering. Useful for sorting colors into a map.
bool operator<(const plColor& c1, const plColor& c2); // [tested]

PL_CHECK_AT_COMPILETIME(sizeof(plColor) == 16);

#include <Foundation/Math/Implementation/Color_inl.h>

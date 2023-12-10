#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Constants.h>
#include <Foundation/Math/Declarations.h>


/// \brief This namespace provides common math-functionality as functions.
///
/// It is a namespace, instead of a static class, because that allows it to be extended
/// at other locations, which is especially useful when adding custom types.
namespace plMath
{
  /// \brief Returns whether the given value is NaN under this type.
  template <typename Type>
  constexpr static bool IsNaN(Type value)
  {
    return false;
  }

  /// \brief Returns whether the given value represents a finite value (i.e. not +/- Infinity and not NaN)
  template <typename Type>
  constexpr static bool IsFinite(Type value)
  {
    return true;
  }

  /// ***** Trigonometric Functions *****

  /// \brief Takes an angle, returns its sine
  [[nodiscard]] float Sin(plAngle a); // [tested]

  /// \brief Takes an angle, returns its cosine
  [[nodiscard]] float Cos(plAngle a); // [tested]

  /// \brief Takes an angle, returns its tangent
  [[nodiscard]] float Tan(plAngle a); // [tested]

  /// \brief Returns the arcus sinus of f
  [[nodiscard]] plAngle ASin(float f); // [tested]

  /// \brief Returns the arcus cosinus of f
  [[nodiscard]] plAngle ACos(float f); // [tested]

  /// \brief Returns the arcus tangent of f
  [[nodiscard]] plAngle ATan(float f); // [tested]

  /// \brief Returns the atan2 of x and y
  [[nodiscard]] plAngle ATan2(float y, float x); // [tested]

  /// \brief Returns e^f
  [[nodiscard]] float Exp(float f); // [tested]

  /// \brief Returns the logarithmus naturalis of f
  [[nodiscard]] float Ln(float f); // [tested]

  /// \brief Returns log (f), to the base 2
  [[nodiscard]] float Log2(float f); // [tested]

  /// \brief Returns the integral logarithm to the base 2, that comes closest to the given integer.
  [[nodiscard]] plUInt32 Log2i(plUInt32 uiVal); // [tested]

  /// \brief Returns log (f), to the base 10
  [[nodiscard]] float Log10(float f); // [tested]

  /// \brief Returns log (f), to the base fBase
  [[nodiscard]] float Log(float fBase, float f); // [tested]

  /// \brief Returns 2^f
  [[nodiscard]] float Pow2(float f); // [tested]

  /// \brief Returns base^exp
  [[nodiscard]] float Pow(float fBase, float fExp); // [tested]

  /// \brief Returns 2^f
  [[nodiscard]] constexpr plInt32 Pow2(plInt32 i); // [tested]

  /// \brief Returns base^exp
  [[nodiscard]] plInt32 Pow(plInt32 iBase, plInt32 iExp); // [tested]

  /// \brief Returns f * f
  template <typename T>
  [[nodiscard]] constexpr T Square(T f); // [tested]

  /// \brief Returns the square root of f
  [[nodiscard]] float Sqrt(float f); // [tested]

  /// \brief Returns the square root of f
  [[nodiscard]] double Sqrt(double f); // [tested]

  /// \brief Returns the n-th root of f.
  [[nodiscard]] float Root(float f, float fNthRoot); // [tested]

  /// \brief Returns the sign of f (i.e: -1, 1 or 0)
  template <typename T>
  [[nodiscard]] constexpr T Sign(T f); // [tested]

  /// \brief Returns the absolute value of f
  template <typename T>
  [[nodiscard]] constexpr T Abs(T f); // [tested]

  /// \brief Returns the smaller value, f1 or f2
  template <typename T>
  [[nodiscard]] constexpr T Min(T f1, T f2); // [tested]

  /// \brief Returns the smaller value, f1 or f2 or ...
  template <typename T, typename... ARGS>
  [[nodiscard]] constexpr T Min(T f1, T f2, ARGS... f); // [tested]

  /// \brief Returns the greater value, f1 or f2
  template <typename T>
  [[nodiscard]] constexpr T Max(T f1, T f2); // [tested]

  /// \brief Returns the smaller value, f1 or f2 or ...
  template <typename T, typename... ARGS>
  [[nodiscard]] constexpr T Min(T f1, T f2, ARGS... f); // [tested]

  /// \brief Clamps "value" to the range [min; max]. Returns "value", if it is inside the range already
  template <typename T>
  [[nodiscard]] constexpr T Clamp(T value, T min_val, T max_val); // [tested]

  /// \brief Clamps "value" to the range [0; 1]. Returns "value", if it is inside the range already
  template <typename T>
  [[nodiscard]] constexpr T Saturate(T value); // [tested]

  /// \brief Returns the next smaller integer, closest to f. Also the SMALLER value, if f is negative.
  [[nodiscard]] float Floor(float f); // [tested]

  /// \brief Returns the next higher integer, closest to f. Also the HIGHER value, if f is negative.
  [[nodiscard]] float Ceil(float f); // [tested]

  /// \brief Returns a multiple of fMultiple that is smaller than f.
  [[nodiscard]] float RoundDown(float f, float fMultiple); // [tested]

  /// \brief Returns a multiple of fMultiple that is smaller than f.
  [[nodiscard]] double RoundDown(double f, double fMultiple); // [tested]

  /// \brief Returns a multiple of fMultiple that is larger than f.
  [[nodiscard]] float RoundUp(float f, float fMultiple); // [tested]

  /// \brief Returns a multiple of fMultiple that is larger than f.
  [[nodiscard]] double RoundUp(double f, double fMultiple); // [tested]

  /// \brief Returns the integer-part of f (removes the fraction).
  template <typename Type>
  [[nodiscard]] Type Trunc(Type f); // [tested]

  /// \brief Casts the float to an integer, removes the fractional part
  ///
  /// \sa Trunc, Round, Floor, Ceil
  [[nodiscard]] constexpr plInt32 FloatToInt(float value);

  // There is a compiler bug in VS 2019 targeting 32-bit that causes an internal compiler error when casting double to long long.
  // FloatToInt(double) is not available on these version of the MSVC compiler.
#if PLASMA_DISABLED(PLASMA_PLATFORM_ARCH_X86) || (_MSC_VER <= 1916)
  /// \brief Casts the float to an integer, removes the fractional part
  ///
  /// \sa Trunc, Round, Floor, Ceil
  [[nodiscard]] constexpr plInt64 FloatToInt(double value);
#endif

  /// \brief Rounds f to the next integer.
  ///
  /// If f is positive 0.5 is rounded UP (i.e. to 1), if f is negative, -0.5 is rounded DOWN (i.e. to -1).
  [[nodiscard]] float Round(float f); // [tested]

  /// \brief Rounds f to the next integer.
  ///
  /// If f is positive 0.5 is rounded UP (i.e. to 1), if f is negative, -0.5 is rounded DOWN (i.e. to -1).
  [[nodiscard]] double Round(double f); // [tested]

  /// \brief Rounds f to the closest value of multiple.
  [[nodiscard]] float RoundToMultiple(float f, float fMultiple);

  /// \brief Rounds f to the closest value of multiple.
  [[nodiscard]] double RoundToMultiple(double f, double fMultiple);

  /// \brief Returns the fraction-part of f.
  template <typename Type>
  [[nodiscard]] Type Fraction(Type f); // [tested]

  /// \brief Returns "value mod div" for floats. This also works with negative numbers, both for value and for div.
  [[nodiscard]] float Mod(float value, float fDiv); // [tested]

  /// \brief Returns "value mod div" for doubles. This also works with negative numbers, both for value and for div.
  [[nodiscard]] double Mod(double f, double fDiv); // [tested]

  /// \brief Returns 1 / f
  template <typename Type>
  [[nodiscard]] constexpr Type Invert(Type f); // [tested]

  /// \brief Returns a multiple of the given multiple that is larger than or equal to value.
  [[nodiscard]] constexpr plInt32 RoundUp(plInt32 value, plUInt16 uiMultiple); // [tested]

  /// \brief Returns a multiple of the given multiple that is smaller than or equal to value.
  [[nodiscard]] constexpr plInt32 RoundDown(plInt32 value, plUInt16 uiMultiple); // [tested]

  /// \brief Returns a multiple of the given multiple that is greater than or equal to value.
  [[nodiscard]] constexpr plUInt32 RoundUp(plUInt32 value, plUInt16 uiMultiple); // [tested]

  /// \brief Returns a multiple of the given multiple that is smaller than or equal to value.
  [[nodiscard]] constexpr plUInt32 RoundDown(plUInt32 value, plUInt16 uiMultiple); // [tested]

  /// \brief Returns true, if i is an odd number
  [[nodiscard]] constexpr bool IsOdd(plInt32 i); // [tested]

  /// \brief Returns true, if i is an even number
  [[nodiscard]] constexpr bool IsEven(plInt32 i); // [tested]

  /// \brief Returns the index of the least significant bit set
  ///
  /// Asserts that bitmask is not 0.
  [[nodiscard]] plUInt32 FirstBitLow(plUInt32 uiBitmask); // [tested]

  /// \brief Returns the index of the least significant bit set
  ///
  /// Asserts that bitmask is not 0.
  [[nodiscard]] plUInt32 FirstBitLow(plUInt64 uiBitmask); // [tested]

  /// \brief Returns the index of the most significant bit set
  ///
  /// Asserts that bitmask is not 0.
  [[nodiscard]] plUInt32 FirstBitHigh(plUInt32 uiBitmask); // [tested]

  /// \brief Returns the index of the most significant bit set
  ///
  /// Asserts that bitmask is not 0.
  [[nodiscard]] plUInt32 FirstBitHigh(plUInt64 uiBitmask); // [tested]

  /// Returns the number of zeros at the end (least significant part) of a bitmask.
  ///
  /// E.g.
  /// 0b0111 -> 0
  /// 0b0110 -> 1
  /// 0b0100 -> 2
  /// Returns 32 when the input is 0
  [[nodiscard]] plUInt32 CountTrailingZeros(plUInt32 uiBitmask); // [tested]

  /// \brief 64 bit overload for CountTrailingZeros()
  [[nodiscard]] plUInt32 CountTrailingZeros(plUInt64 uiBitmask); // [tested]

  /// Returns the number of zeros at the start (most significant part) of a bitmask.
  ///
  /// E.g.
  /// 0b0111 -> 29
  /// 0b0011 -> 30
  /// 0b0001 -> 31
  /// 0b0000 -> 32
  /// Returns 32 when the input is 0
  [[nodiscard]] plUInt32 CountLeadingZeros(plUInt32 uiBitmask); // [tested]

  /// \brief Returns the number of bits set
  [[nodiscard]] plUInt32 CountBits(plUInt32 value);

  /// \brief Returns the number of bits set
  [[nodiscard]] plUInt32 CountBits(plUInt64 value);

  /// \brief Creates a bitmask in which the low N bits are set. For example for N=5, this would be '0000 ... 0001 1111'
  ///
  /// For N >= 32 all bits will be set.
  template <typename Type>
  [[nodiscard]] Type Bitmask_LowN(plUInt32 uiNumBitsToSet);

  /// \brief Creates a bitmask in which the high N bits are set. For example for N=5, this would be '1111 1000 ... 0000'
  ///
  /// For N >= 32 all bits will be set.
  template <typename Type>
  [[nodiscard]] Type Bitmask_HighN(plUInt32 uiNumBitsToSet);

  /// \brief Swaps the values in the two variables f1 and f2
  template <typename T>
  void Swap(T& ref_f1, T& ref_f2); // [tested]

  /// \brief Returns the linear interpolation of f1 and f2. factor is a value between 0 and 1.
  template <typename T>
  [[nodiscard]] T Lerp(T f1, T f2, float fFactor); // [tested]

  /// \brief Returns the linear interpolation of f1 and f2. factor is a value between 0 and 1.
  template <typename T>
  [[nodiscard]] T Lerp(T f1, T f2, double fFactor); // [tested]

  /// \brief Returns 0, if value < edge, and 1, if value >= edge.
  template <typename T>
  [[nodiscard]] constexpr T Step(T value, T edge); // [tested]

  /// \brief Returns 0, if value is <= edge1, 1 if value >= edge2 and the hermite interpolation in between
  template <typename Type>
  [[nodiscard]] Type SmoothStep(Type value, Type edge1, Type edge2); // [tested]

  /// \brief Returns true, if there exists some x with base^x == value
  [[nodiscard]] PLASMA_FOUNDATION_DLL bool IsPowerOf(plInt32 value, plInt32 iBase); // [tested]

  /// \brief Returns true, if there exists some x with 2^x == value
  [[nodiscard]] constexpr bool IsPowerOf2(plInt32 value); // [tested]

  /// \brief Returns true, if there exists some x with 2^x == value
  [[nodiscard]] constexpr bool IsPowerOf2(plUInt32 value); // [tested]

  /// \brief Returns the next power-of-two that is <= value
  [[nodiscard]] PLASMA_FOUNDATION_DLL plUInt32 PowerOfTwo_Floor(plUInt32 value); // [tested]

  /// \brief Returns the next power-of-two that is >= value
  [[nodiscard]] PLASMA_FOUNDATION_DLL plUInt32 PowerOfTwo_Ceil(plUInt32 value); // [tested]

  /// \brief Returns the greatest common divisor.
  [[nodiscard]] PLASMA_FOUNDATION_DLL plUInt32 GreatestCommonDivisor(plUInt32 a, plUInt32 b); // [tested]

  /// \brief Checks, whether fValue is in the range [fDesired - fMaxImprecision; fDesired + fMaxImprecision].
  template <typename Type>
  [[nodiscard]] constexpr bool IsEqual(Type lhs, Type rhs, Type fEpsilon);

  /// \brief Checks whether the value of the first parameter lies between the value of the second and third.
  template <typename T>
  [[nodiscard]] constexpr bool IsInRange(T value, T minVal, T maxVal); // [tested]

  /// \brief Checks whether the given number is close to zero.
  template <typename Type>
  [[nodiscard]] bool IsZero(Type f, Type fEpsilon); // [tested]

  /// \brief Converts a color value from float [0;1] range to unsigned byte [0;255] range, with proper rounding
  [[nodiscard]] plUInt8 ColorFloatToByte(float value); // [tested]

  /// \brief Converts a color value from float [0;1] range to unsigned short [0;65535] range, with proper rounding
  [[nodiscard]] plUInt16 ColorFloatToShort(float value); // [tested]

  /// \brief Converts a color value from float [-1;1] range to signed byte [-127;127] range, with proper rounding
  [[nodiscard]] plInt8 ColorFloatToSignedByte(float value); // [tested]

  /// \brief Converts a color value from float [-1;1] range to signed short [-32767;32767] range, with proper rounding
  [[nodiscard]] plInt16 ColorFloatToSignedShort(float value); // [tested]

  /// \brief Converts a color value from unsigned byte [0;255] range to float [0;1] range, with proper rounding
  [[nodiscard]] constexpr float ColorByteToFloat(plUInt8 value); // [tested]

  /// \brief Converts a color value from unsigned short [0;65535] range to float [0;1] range, with proper rounding
  [[nodiscard]] constexpr float ColorShortToFloat(plUInt16 value); // [tested]

  /// \brief Converts a color value from signed byte [-128;127] range to float [-1;1] range, with proper rounding
  [[nodiscard]] constexpr float ColorSignedByteToFloat(plInt8 value); // [tested]

  /// \brief Converts a color value from signed short [-32768;32767] range to float [0;1] range, with proper rounding
  [[nodiscard]] constexpr float ColorSignedShortToFloat(plInt16 value); // [tested]

  /// \brief Evaluates the cubic spline defined by four control points at time \a t and returns the interpolated result.
  /// Can be used with T as float, vec2, vec3 or vec4
  template <typename T, typename T2>
  [[nodiscard]] T EvaluateBezierCurve(T2 t, const T& startPoint, const T& controlPoint1, const T& controlPoint2, const T& endPoint);

  /// \brief out_Result = \a a * \a b. If an overflow happens, PLASMA_FAILURE is returned.
  PLASMA_FOUNDATION_DLL plResult TryMultiply32(plUInt32& out_uiResult, plUInt32 a, plUInt32 b, plUInt32 c = 1, plUInt32 d = 1); // [tested]

  /// \brief returns \a a * \a b. If an overflow happens, the program is terminated.
  [[nodiscard]] PLASMA_FOUNDATION_DLL plUInt32 SafeMultiply32(plUInt32 a, plUInt32 b, plUInt32 c = 1, plUInt32 d = 1);

  /// \brief out_Result = \a a * \a b. If an overflow happens, PLASMA_FAILURE is returned.
  PLASMA_FOUNDATION_DLL plResult TryMultiply64(plUInt64& out_uiResult, plUInt64 a, plUInt64 b, plUInt64 c = 1, plUInt64 d = 1); // [tested]

  /// \brief returns \a a * \a b. If an overflow happens, the program is terminated.
  [[nodiscard]] PLASMA_FOUNDATION_DLL plUInt64 SafeMultiply64(plUInt64 a, plUInt64 b, plUInt64 c = 1, plUInt64 d = 1);

  /// \brief Checks whether the given 64bit value actually fits into size_t, If it doesn't PLASMA_FAILURE is returned.
  plResult TryConvertToSizeT(size_t& out_uiResult, plUInt64 uiValue); // [tested]

  /// \brief Checks whether the given 64bit value actually fits into size_t, If it doesn't the program is terminated.
  [[nodiscard]] PLASMA_FOUNDATION_DLL size_t SafeConvertToSizeT(plUInt64 uiValue);

  /// \brief If 'value' is not-a-number (NaN) 'fallback' is returned, otherwise 'value' is passed through unmodified.
  [[nodiscard]] PLASMA_FOUNDATION_DLL float ReplaceNaN(float fValue, float fFallback); // [tested]

  /// \brief If 'value' is not-a-number (NaN) 'fallback' is returned, otherwise 'value' is passed through unmodified.
  [[nodiscard]] PLASMA_FOUNDATION_DLL double ReplaceNaN(double fValue, double fFallback); // [tested]

} // namespace plMath

#include <Foundation/Math/Implementation/MathDouble_inl.h>
#include <Foundation/Math/Implementation/MathFixedPoint_inl.h>
#include <Foundation/Math/Implementation/MathFloat_inl.h>
#include <Foundation/Math/Implementation/MathInt32_inl.h>
#include <Foundation/Math/Implementation/Math_inl.h>

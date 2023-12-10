#pragma once

#include <float.h>

namespace plMath
{
  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr float Pi()
  {
    return static_cast<float>(3.1415926535897932384626433832795f);
  }

  template <>
  constexpr double Pi()
  {
    return static_cast<double>(3.1415926535897932384626433832795);
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr float e()
  {
    return static_cast<float>(2.71828182845904);
  }

  template <>
  constexpr double e()
  {
    return static_cast<double>(2.71828182845904);
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename TYPE>
  constexpr bool SupportsNaN()
  {
    return false;
  }

  template <>
  constexpr bool SupportsNaN<float>()
  {
    return true;
  }

  template <>
  constexpr bool SupportsNaN<double>()
  {
    return true;
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename TYPE>
  constexpr TYPE NaN()
  {
    return static_cast<TYPE>(0);
  }

  template <>
  constexpr float NaN()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // NaN = 0111 1111 1000 0000 0000 0000 0000 0001

    plIntFloatUnion i2f(0x7f800042u);
    return i2f.f;
  }

  template <>
  constexpr double NaN()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // NaN = 0111 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001

    plInt64DoubleUnion i2f(0x7FF0000000000042ull);
    return i2f.f;
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename TYPE>
  constexpr bool SupportsInfinity()
  {
    return false;
  }

  template <>
  constexpr bool SupportsInfinity<float>()
  {
    return true;
  }

  template <>
  constexpr bool SupportsInfinity<double>()
  {
    return true;
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename TYPE>
  constexpr TYPE Infinity()
  {
    return static_cast<TYPE>(0);
  }

  template <>
  constexpr float Infinity()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // INF = 0111 1111 1000 0000 0000 0000 0000 0000

    // bitwise representation of float infinity (positive)
    plIntFloatUnion i2f(0x7f800000u);
    return i2f.f;
  }

  template <>
  constexpr double Infinity()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // INF = 0111 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000

    // bitwise representation of double infinity (positive)
    plInt64DoubleUnion i2f(0x7FF0000000000000ull);

    return i2f.f;
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr plUInt8 MaxValue()
  {
    return 0xFF;
  }

  template <>
  constexpr plUInt16 MaxValue()
  {
    return 0xFFFF;
  }

  template <>
  constexpr plUInt32 MaxValue()
  {
    return 0xFFFFFFFFu;
  }

  template <>
  constexpr plUInt64 MaxValue()
  {
    return 0xFFFFFFFFFFFFFFFFull;
  }

  template <>
  constexpr plInt8 MaxValue()
  {
    return 0x7F;
  }

  template <>
  constexpr plInt16 MaxValue()
  {
    return 0x7FFF;
  }

  template <>
  constexpr plInt32 MaxValue()
  {
    return 0x7FFFFFFF;
  }

  template <>
  constexpr plInt64 MaxValue()
  {
    return 0x7FFFFFFFFFFFFFFFll;
  }

  template <>
  constexpr float MaxValue()
  {
    return 3.402823465e+38F;
  }

  template <>
  constexpr double MaxValue()
  {
    return 1.7976931348623158e+307;
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr plUInt8 MinValue()
  {
    return 0;
  }

  template <>
  constexpr plUInt16 MinValue()
  {
    return 0;
  }

  template <>
  constexpr plUInt32 MinValue()
  {
    return 0;
  }

  template <>
  constexpr plUInt64 MinValue()
  {
    return 0;
  }

  template <>
  constexpr plInt8 MinValue()
  {
    return -MaxValue<plInt8>() - 1;
  }

  template <>
  constexpr plInt16 MinValue()
  {
    return -MaxValue<plInt16>() - 1;
  }

  template <>
  constexpr plInt32 MinValue()
  {
    return -MaxValue<plInt32>() - 1;
  }

  template <>
  constexpr plInt64 MinValue()
  {
    return -MaxValue<plInt64>() - 1;
  }

  template <>
  constexpr float MinValue()
  {
    return -3.402823465e+38F;
  }

  template <>
  constexpr double MinValue()
  {
    return -1.7976931348623158e+307;
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr float HighValue()
  {
    return 1.8446726e+019f;
  }

  template <>
  constexpr double HighValue()
  {
    return 1.8446726e+150;
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr float FloatEpsilon()
  {
    return FLT_EPSILON;
  }

  template <>
  constexpr double FloatEpsilon()
  {
    return DBL_EPSILON;
  }


  template <typename TYPE>
  constexpr TYPE SmallEpsilon()
  {
    return (TYPE)0.000001;
  }

  template <typename TYPE>
  constexpr TYPE DefaultEpsilon()
  {
    return (TYPE)0.00001;
  }

  template <typename TYPE>
  constexpr TYPE LargeEpsilon()
  {
    return (TYPE)0.0001;
  }

  template <typename TYPE>
  constexpr TYPE HugeEpsilon()
  {
    return (TYPE)0.001;
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr plUInt32 NumBits<plUInt8>()
  {
    return 8;
  }

  template <>
  constexpr plUInt32 NumBits<plUInt16>()
  {
    return 16;
  }

  template <>
  constexpr plUInt32 NumBits<plUInt32>()
  {
    return 32;
  }

  template <>
  constexpr plUInt32 NumBits<plUInt64>()
  {
    return 64;
  }

  template <>
  constexpr plUInt32 NumBits<plInt8>()
  {
    return 8;
  }

  template <>
  constexpr plUInt32 NumBits<plInt16>()
  {
    return 16;
  }

  template <>
  constexpr plUInt32 NumBits<plInt32>()
  {
    return 32;
  }

  template <>
  constexpr plUInt32 NumBits<plInt64>()
  {
    return 64;
  }

  template <>
  constexpr plUInt32 NumBits<float>()
  {
    return 32;
  }

  template <>
  constexpr plUInt32 NumBits<double>()
  {
    return 64;
  }

  //////////////////////////////////////////////////////////////////////////

} // namespace plMath

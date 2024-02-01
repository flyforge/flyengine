#pragma once

#include <algorithm>

namespace plMath
{
  template <typename T>
  constexpr PL_ALWAYS_INLINE T Square(T f)
  {
    return (f * f);
  }

  template <typename T>
  constexpr PL_ALWAYS_INLINE T Sign(T f)
  {
    return (f < 0 ? T(-1) : f > 0 ? T(1)
                                  : 0);
  }

  template <typename T>
  constexpr PL_ALWAYS_INLINE T Abs(T f)
  {
    return (f < 0 ? -f : f);
  }

  template <typename T>
  constexpr PL_ALWAYS_INLINE T Min(T f1, T f2)
  {
    return (f2 < f1 ? f2 : f1);
  }

  template <typename T, typename... ARGS>
  constexpr PL_ALWAYS_INLINE T Min(T f1, T f2, ARGS... f)
  {
    return Min(Min(f1, f2), f...);
  }

  template <typename T>
  constexpr PL_ALWAYS_INLINE T Max(T f1, T f2)
  {
    return (f1 < f2 ? f2 : f1);
  }

  template <typename T, typename... ARGS>
  constexpr PL_ALWAYS_INLINE T Max(T f1, T f2, ARGS... f)
  {
    return Max(Max(f1, f2), f...);
  }

  template <typename T>
  constexpr PL_ALWAYS_INLINE T Clamp(T value, T min_val, T max_val)
  {
    return value < min_val ? min_val : (max_val < value ? max_val : value);
  }

  template <typename T>
  constexpr PL_ALWAYS_INLINE T Saturate(T value)
  {
    return Clamp(value, T(0), T(1));
  }

  template <typename Type>
  constexpr Type Invert(Type f)
  {
    static_assert(std::is_floating_point_v<Type>);

    return ((Type)1) / f;
  }

  PL_ALWAYS_INLINE plUInt32 FirstBitLow(plUInt32 value)
  {
    PL_ASSERT_DEBUG(value != 0, "FirstBitLow is undefined for 0");

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
    unsigned long uiIndex = 0;
    _BitScanForward(&uiIndex, value);
    return uiIndex;
#elif PL_ENABLED(PL_COMPILER_GCC) || PL_ENABLED(PL_COMPILER_CLANG)
    return __builtin_ctz(value);
#else
    PL_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  PL_ALWAYS_INLINE plUInt32 FirstBitLow(plUInt64 value)
  {
    PL_ASSERT_DEBUG(value != 0, "FirstBitLow is undefined for 0");

#if __castxml__
    return 0;
#elif PL_ENABLED(PL_PLATFORM_WINDOWS)
    unsigned long uiIndex = 0;
#  if PL_ENABLED(PL_PLATFORM_64BIT)

    _BitScanForward64(&uiIndex, value);
#  else
    uint32_t lower = static_cast<uint32_t>(value);
    unsigned char returnCode = _BitScanForward(&uiIndex, lower);
    if (returnCode == 0)
    {
      uint32_t upper = static_cast<uint32_t>(value >> 32);
      returnCode = _BitScanForward(&uiIndex, upper);
      if (returnCode > 0) // Only can happen in Release build when PL_ASSERT_DEBUG(value != 0) would fail.
      {
        uiIndex += 32; // Add length of lower to index.
      }
    }
#  endif
    return uiIndex;
#elif PL_ENABLED(PL_COMPILER_GCC) || PL_ENABLED(PL_COMPILER_CLANG)
    return __builtin_ctzll(value);
#else
    PL_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  PL_ALWAYS_INLINE plUInt32 FirstBitHigh(plUInt32 value)
  {
    PL_ASSERT_DEBUG(value != 0, "FirstBitHigh is undefined for 0");

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
    unsigned long uiIndex = 0;
    _BitScanReverse(&uiIndex, value);
    return uiIndex;
#elif PL_ENABLED(PL_COMPILER_GCC) || PL_ENABLED(PL_COMPILER_CLANG)
    return 31 - __builtin_clz(value);
#else
    PL_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  PL_ALWAYS_INLINE plUInt32 FirstBitHigh(plUInt64 value)
  {
    PL_ASSERT_DEBUG(value != 0, "FirstBitHigh is undefined for 0");

#if __castxml__
    return 0;
#elif PL_ENABLED(PL_PLATFORM_WINDOWS)
    unsigned long uiIndex = 0;
#  if PL_ENABLED(PL_PLATFORM_64BIT)
    _BitScanReverse64(&uiIndex, value);
#  else
    uint32_t upper = static_cast<uint32_t>(value >> 32);
    unsigned char returnCode = _BitScanReverse(&uiIndex, upper);
    if (returnCode == 0)
    {
      uint32_t lower = static_cast<uint32_t>(value);
      returnCode = _BitScanReverse(&uiIndex, lower);
    }
    else
    {
      uiIndex += 32; // Add length of upper to index.
    }
#  endif
    return uiIndex;
#elif PL_ENABLED(PL_COMPILER_GCC) || PL_ENABLED(PL_COMPILER_CLANG)
    return 63 - __builtin_clzll(value);
#else
    PL_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  PL_ALWAYS_INLINE plUInt32 CountTrailingZeros(plUInt32 uiBitmask)
  {
    return (uiBitmask == 0) ? 32 : FirstBitLow(uiBitmask);
  }

  PL_ALWAYS_INLINE plUInt32 CountTrailingZeros(plUInt64 uiBitmask)
  {
    const plUInt32 numLow = CountTrailingZeros(static_cast<plUInt32>(uiBitmask & 0xFFFFFFFF));
    const plUInt32 numHigh = CountTrailingZeros(static_cast<plUInt32>((uiBitmask >> 32u) & 0xFFFFFFFF));

    return (numLow == 32) ? (32 + numHigh) : numLow;
  }

  PL_ALWAYS_INLINE plUInt32 CountLeadingZeros(plUInt32 uiBitmask)
  {
    return (uiBitmask == 0) ? 32 : (31u - FirstBitHigh(uiBitmask));
  }


  PL_ALWAYS_INLINE plUInt32 CountBits(plUInt32 value)
  {
#if PL_ENABLED(PL_COMPILER_MSVC) && (PL_ENABLED(PL_PLATFORM_ARCH_X86) || (PL_ENABLED(PL_PLATFORM_ARCH_ARM) && PL_ENABLED(PL_PLATFORM_32BIT)))
#  if PL_ENABLED(PL_PLATFORM_ARCH_X86)
    return __popcnt(value);
#  else
    return _CountOneBits(value);
#  endif
#elif PL_ENABLED(PL_COMPILER_GCC) || PL_ENABLED(PL_COMPILER_CLANG)
    return __builtin_popcount(value);
#else
    value = value - ((value >> 1) & 0x55555555u);
    value = (value & 0x33333333u) + ((value >> 2) & 0x33333333u);
    return ((value + (value >> 4) & 0xF0F0F0Fu) * 0x1010101u) >> 24;
#endif
  }

  PL_ALWAYS_INLINE plUInt32 CountBits(plUInt64 value)
  {
    plUInt32 result = 0;
    result += CountBits(plUInt32(value));
    result += CountBits(plUInt32(value >> 32));
    return result;
  }

  template <typename Type>
  PL_ALWAYS_INLINE Type Bitmask_LowN(plUInt32 uiNumBitsToSet)
  {
    return (uiNumBitsToSet >= sizeof(Type) * 8) ? ~static_cast<Type>(0) : ((static_cast<Type>(1) << uiNumBitsToSet) - static_cast<Type>(1));
  }

  template <typename Type>
  PL_ALWAYS_INLINE Type Bitmask_HighN(plUInt32 uiNumBitsToSet)
  {
    return (uiNumBitsToSet == 0) ? 0 : ~static_cast<Type>(0) << ((sizeof(Type) * 8) - plMath::Min<plUInt32>(uiNumBitsToSet, sizeof(Type) * 8));
  }

  template <typename T>
  PL_ALWAYS_INLINE void Swap(T& ref_f1, T& ref_f2)
  {
    std::swap(ref_f1, ref_f2);
  }

  template <typename T>
  PL_FORCE_INLINE T Lerp(T f1, T f2, float fFactor)
  {
    // value is not included in format string, to prevent requirement on FormatString.h, to break #include cycles
    PL_ASSERT_DEBUG((fFactor >= -0.00001f) && (fFactor <= 1.0f + 0.00001f), "lerp: factor is not in the range [0; 1]");

    return (T)(f1 + (fFactor * (f2 - f1)));
  }

  template <typename T>
  PL_FORCE_INLINE T Lerp(T f1, T f2, double fFactor)
  {
    // value is not included in format string, to prevent requirement on FormatString.h, to break #include cycles
    PL_ASSERT_DEBUG((fFactor >= -0.00001) && (fFactor <= 1.0 + 0.00001), "lerp: factor is not in the range [0; 1]");

    return (T)(f1 + (fFactor * (f2 - f1)));
  }

  template <typename T>
  PL_FORCE_INLINE constexpr float Unlerp(T fMin, T fMax, T fValue)
  {
    return static_cast<float>(fValue - fMin) / static_cast<float>(fMax - fMin);
  }

  ///  Returns 0, if value < edge, and 1, if value >= edge.
  template <typename T>
  constexpr PL_FORCE_INLINE T Step(T value, T edge)
  {
    return (value >= edge ? T(1) : T(0));
  }

  constexpr PL_FORCE_INLINE bool IsPowerOf2(plInt32 value)
  {
    return (value < 1) ? false : ((value & (value - 1)) == 0);
  }

  constexpr PL_FORCE_INLINE bool IsPowerOf2(plUInt32 value)
  {
    return (value < 1) ? false : ((value & (value - 1)) == 0);
  }

  constexpr PL_FORCE_INLINE bool IsPowerOf2(plUInt64 value)
  {
    return (value < 1) ? false : ((value & (value - 1)) == 0);
  }

  template <typename Type>
  constexpr bool IsEqual(Type lhs, Type rhs, Type fEpsilon)
  {
    return ((rhs >= lhs - fEpsilon) && (rhs <= lhs + fEpsilon));
  }

  template <typename T>
  constexpr inline bool IsInRange(T value, T minVal, T maxVal)
  {
    return minVal < maxVal ? (value >= minVal) && (value <= maxVal) : (value <= minVal) && (value >= maxVal);
  }

  template <typename Type>
  bool IsZero(Type f, Type fEpsilon)
  {
    PL_ASSERT_DEBUG(fEpsilon >= 0, "Epsilon may not be negative.");

    return ((f >= -fEpsilon) && (f <= fEpsilon));
  }

  template <typename Type>
  PL_ALWAYS_INLINE Type Trunc(Type f)
  {
    if (f > 0)
      return Floor(f);

    return Ceil(f);
  }

  template <typename Type>
  PL_ALWAYS_INLINE Type Fraction(Type f)
  {
    return (f - Trunc(f));
  }

  template <typename Type>
  inline Type SmoothStep(Type x, Type edge1, Type edge2)
  {
    const Type divider = edge2 - edge1;

    if (divider == (Type)0)
    {
      return (x >= edge2) ? 1 : 0;
    }

    x = Saturate((x - edge1) / divider);

    return (x * x * ((Type)3 - ((Type)2 * x)));
  }

  template <typename Type>
  inline Type SmootherStep(Type x, Type edge1, Type edge2)
  {
    const Type divider = edge2 - edge1;

    if (divider == (Type)0)
    {
      return (x >= edge2) ? 1 : 0;
    }

    x = Saturate((x - edge1) / divider);

    return (x * x * x * (x * ((Type)6 * x - (Type)15) + (Type)10));
  }

  inline plUInt8 ColorFloatToByte(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      return static_cast<plUInt8>(Saturate(value) * 255.0f + 0.5f);
    }
  }

  inline plUInt16 ColorFloatToShort(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      return static_cast<plUInt16>(Saturate(value) * 65535.0f + 0.5f);
    }
  }

  inline plInt8 ColorFloatToSignedByte(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      value = Clamp(value, -1.0f, 1.0f) * 127.0f;
      if (value >= 0.0f)
      {
        value += 0.5f;
      }
      else
      {
        value -= 0.5f;
      }
      return static_cast<plInt8>(value);
    }
  }

  inline plInt16 ColorFloatToSignedShort(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      value = Clamp(value, -1.0f, 1.0f) * 32767.0f;
      if (value >= 0.0f)
      {
        value += 0.5f;
      }
      else
      {
        value -= 0.5f;
      }
      return static_cast<plInt16>(value);
    }
  }

  constexpr inline float ColorByteToFloat(plUInt8 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return value * (1.0f / 255.0f);
  }

  constexpr inline float ColorShortToFloat(plUInt16 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return value * (1.0f / 65535.0f);
  }

  constexpr inline float ColorSignedByteToFloat(plInt8 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return (value == -128) ? -1.0f : value * (1.0f / 127.0f);
  }

  constexpr inline float ColorSignedShortToFloat(plInt16 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return (value == -32768) ? -1.0f : value * (1.0f / 32767.0f);
  }

  template <typename T, typename T2>
  T EvaluateBplierCurve(T2 t, const T& startPoint, const T& controlPoint1, const T& controlPoint2, const T& endPoint)
  {
    const T2 mt = 1 - t;

    const T2 f1 = mt * mt * mt;
    const T2 f2 = 3 * mt * mt * t;
    const T2 f3 = 3 * mt * t * t;
    const T2 f4 = t * t * t;

    return f1 * startPoint + f2 * controlPoint1 + f3 * controlPoint2 + f4 * endPoint;
  }
} // namespace plMath

constexpr PL_FORCE_INLINE plAngle plAngle::AngleBetween(plAngle a, plAngle b)
{
  // taken from http://gamedev.stackexchange.com/questions/4467/comparing-angles-and-working-out-the-difference
  return plAngle(Pi<float>() - plMath::Abs(plMath::Abs(a.GetRadian() - b.GetRadian()) - Pi<float>()));
}

constexpr PL_FORCE_INLINE plInt32 plMath::FloatToInt(float value)
{
  return static_cast<plInt32>(value);
}

#if PL_DISABLED(PL_PLATFORM_ARCH_X86) || (_MSC_VER <= 1916)
constexpr PL_FORCE_INLINE plInt64 plMath::FloatToInt(double value)
{
  return static_cast<plInt64>(value);
}
#endif

PL_ALWAYS_INLINE plResult plMath::TryConvertToSizeT(size_t& out_uiResult, plUInt64 uiValue)
{
#if PL_ENABLED(PL_PLATFORM_32BIT)
  if (uiValue <= MaxValue<size_t>())
  {
    out_uiResult = static_cast<size_t>(uiValue);
    return PL_SUCCESS;
  }

  return PL_FAILURE;
#else
  out_uiResult = static_cast<size_t>(uiValue);
  return PL_SUCCESS;
#endif
}

#if PL_ENABLED(PL_PLATFORM_64BIT)
PL_ALWAYS_INLINE size_t plMath::SafeConvertToSizeT(plUInt64 uiValue)
{
  return uiValue;
}
#endif

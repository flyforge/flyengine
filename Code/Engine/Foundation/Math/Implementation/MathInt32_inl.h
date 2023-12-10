#pragma once

namespace plMath
{
  constexpr PLASMA_ALWAYS_INLINE plInt32 RoundUp(plInt32 value, plUInt16 uiMultiple)
  {
    //
    return (value >= 0) ? ((value + uiMultiple - 1) / uiMultiple) * uiMultiple : (value / uiMultiple) * uiMultiple;
  }

  constexpr PLASMA_ALWAYS_INLINE plInt32 RoundDown(plInt32 value, plUInt16 uiMultiple)
  {
    //
    return (value <= 0) ? ((value - uiMultiple + 1) / uiMultiple) * uiMultiple : (value / uiMultiple) * uiMultiple;
  }

  constexpr PLASMA_ALWAYS_INLINE plUInt32 RoundUp(plUInt32 value, plUInt16 uiMultiple)
  {
    //
    return ((value + uiMultiple - 1) / uiMultiple) * uiMultiple;
  }

  constexpr PLASMA_ALWAYS_INLINE plUInt32 RoundDown(plUInt32 value, plUInt16 uiMultiple)
  {
    //
    return (value / uiMultiple) * uiMultiple;
  }

  constexpr PLASMA_ALWAYS_INLINE bool IsOdd(plInt32 i)
  {
    //
    return ((i & 1) != 0);
  }

  constexpr PLASMA_ALWAYS_INLINE bool IsEven(plInt32 i)
  {
    //
    return ((i & 1) == 0);
  }

  PLASMA_ALWAYS_INLINE plUInt32 Log2i(plUInt32 uiVal)
  {
    return (uiVal != 0) ? FirstBitHigh(uiVal) : -1;
  }

  constexpr PLASMA_ALWAYS_INLINE int Pow2(int i)
  {
    //
    return (1 << i);
  }

  inline int Pow(int iBase, int iExp)
  {
    int res = 1;
    while (iExp > 0)
    {
      res *= iBase;
      --iExp;
    }

    return res;
  }

} // namespace plMath

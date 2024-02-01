#pragma once

#include <Foundation/Math/FixedPoint.h>

/*
namespace plMath
{
#define FIXEDPOINT_OVERLOADS(Bits)                                                                                                       \
  template <>                                                                                                                            \
  PL_ALWAYS_INLINE plFixedPoint<Bits> BasicType<plFixedPoint<Bits>>::MaxValue() { return (plFixedPoint<Bits>)((1 << (31 - Bits)) - 1); } \
  template <>                                                                                                                            \
  PL_ALWAYS_INLINE plFixedPoint<Bits> BasicType<plFixedPoint<Bits>>::SmallEpsilon() { return (plFixedPoint<Bits>)0.0001; }               \
  template <>                                                                                                                            \
  PL_ALWAYS_INLINE plFixedPoint<Bits> BasicType<plFixedPoint<Bits>>::DefaultEpsilon() { return (plFixedPoint<Bits>)0.001; }              \
  template <>                                                                                                                            \
  PL_ALWAYS_INLINE plFixedPoint<Bits> BasicType<plFixedPoint<Bits>>::LargeEpsilon() { return (plFixedPoint<Bits>)0.01; }                 \
  template <>                                                                                                                            \
  PL_ALWAYS_INLINE plFixedPoint<Bits> BasicType<plFixedPoint<Bits>>::HugeEpsilon() { return (plFixedPoint<Bits>)0.1; }

  FIXEDPOINT_OVERLOADS(1);
  FIXEDPOINT_OVERLOADS(2);
  FIXEDPOINT_OVERLOADS(3);
  FIXEDPOINT_OVERLOADS(4);
  FIXEDPOINT_OVERLOADS(5);
  FIXEDPOINT_OVERLOADS(6);
  FIXEDPOINT_OVERLOADS(7);
  FIXEDPOINT_OVERLOADS(8);
  FIXEDPOINT_OVERLOADS(9);
  FIXEDPOINT_OVERLOADS(10);
  FIXEDPOINT_OVERLOADS(11);
  FIXEDPOINT_OVERLOADS(12);
  FIXEDPOINT_OVERLOADS(13);
  FIXEDPOINT_OVERLOADS(14);
  FIXEDPOINT_OVERLOADS(15);
  FIXEDPOINT_OVERLOADS(16);
  FIXEDPOINT_OVERLOADS(17);
  FIXEDPOINT_OVERLOADS(18);
  FIXEDPOINT_OVERLOADS(19);
  FIXEDPOINT_OVERLOADS(20);
  FIXEDPOINT_OVERLOADS(21);
  FIXEDPOINT_OVERLOADS(22);
  FIXEDPOINT_OVERLOADS(23);
  FIXEDPOINT_OVERLOADS(24);
  FIXEDPOINT_OVERLOADS(25);
  FIXEDPOINT_OVERLOADS(26);
  FIXEDPOINT_OVERLOADS(27);
  FIXEDPOINT_OVERLOADS(28);
  FIXEDPOINT_OVERLOADS(29);
  FIXEDPOINT_OVERLOADS(30);
  //FIXEDPOINT_OVERLOADS(31);

  template <plUInt8 DecimalBits>
  PL_FORCE_INLINE plFixedPoint<DecimalBits> Floor(plFixedPoint<DecimalBits> f)
  {
    PL_REPORT_FAILURE("This function is not really implemented yet.");

    return (plFixedPoint<DecimalBits>)floor(f.ToDouble());
  }

  template <plUInt8 DecimalBits>
  PL_FORCE_INLINE plFixedPoint<DecimalBits> Ceil(plFixedPoint<DecimalBits> f)
  {
    PL_REPORT_FAILURE("This function is not really implemented yet.");

    return (plFixedPoint<DecimalBits>)ceil(f.ToDouble());
  }

  template <plUInt8 DecimalBits>
  inline plFixedPoint<DecimalBits> Floor(plFixedPoint<DecimalBits> f, plFixedPoint<DecimalBits> fMultiple)
  {
    PL_REPORT_FAILURE("This function is not really implemented yet.");

    plFixedPoint<DecimalBits> fDivides = f / fMultiple;
    plFixedPoint<DecimalBits> fFactor = Floor(fDivides);
    return fFactor * fMultiple;
  }

  template <plUInt8 DecimalBits>
  inline plFixedPoint<DecimalBits> Ceil(plFixedPoint<DecimalBits> f, plFixedPoint<DecimalBits> fMultiple)
  {
    PL_REPORT_FAILURE("This function is not really implemented yet.");

    plFixedPoint<DecimalBits> fDivides = f / fMultiple;
    plFixedPoint<DecimalBits> fFactor = Ceil(fDivides);
    return fFactor * fMultiple;
  }

  template <plUInt8 DecimalBits>
  PL_FORCE_INLINE plFixedPoint<DecimalBits> Exp(plFixedPoint<DecimalBits> f)
  {
    PL_REPORT_FAILURE("This function is not really implemented yet.");

    return (plFixedPoint<DecimalBits>)exp(f.ToDouble());
  }

  template <plUInt8 DecimalBits>
  PL_FORCE_INLINE plFixedPoint<DecimalBits> Ln(plFixedPoint<DecimalBits> f)
  {
    PL_REPORT_FAILURE("This function is not really implemented yet.");

    return (plFixedPoint<DecimalBits>)log(f.ToDouble());
  }

  template <plUInt8 DecimalBits>
  PL_FORCE_INLINE plFixedPoint<DecimalBits> Log2(plFixedPoint<DecimalBits> f)
  {
    PL_REPORT_FAILURE("This function is not really implemented yet.");

    return (plFixedPoint<DecimalBits>)(log10(f.ToDouble()) / log10(2.0));
  }

  template <plUInt8 DecimalBits>
  PL_FORCE_INLINE plFixedPoint<DecimalBits> Log10(plFixedPoint<DecimalBits> f)
  {
    PL_REPORT_FAILURE("This function is not really implemented yet.");

    return (plFixedPoint<DecimalBits>)log10(f.ToDouble());
  }

  template <plUInt8 DecimalBits>
  PL_FORCE_INLINE plFixedPoint<DecimalBits> Log(plFixedPoint<DecimalBits> fBase, plFixedPoint<DecimalBits> f)
  {
    PL_REPORT_FAILURE("This function is not really implemented yet.");

    return (plFixedPoint<DecimalBits>)(log10(f.ToDouble()) / log10(fBase.ToDouble()));
  }

  template <plUInt8 DecimalBits>
  PL_FORCE_INLINE plFixedPoint<DecimalBits> Pow2(plFixedPoint<DecimalBits> f)
  {
    PL_REPORT_FAILURE("This function is not really implemented yet.");

    return (plFixedPoint<DecimalBits>)pow(2.0, f.ToDouble());
  }

  template <plUInt8 DecimalBits>
  PL_FORCE_INLINE plFixedPoint<DecimalBits> Pow(plFixedPoint<DecimalBits> base, plFixedPoint<DecimalBits> exp)
  {
    PL_REPORT_FAILURE("This function is not really implemented yet.");

    return (plFixedPoint<DecimalBits>)pow(base.ToDouble(), exp.ToDouble());
  }

  template <plUInt8 DecimalBits>
  PL_FORCE_INLINE plFixedPoint<DecimalBits> Root(plFixedPoint<DecimalBits> f, plFixedPoint<DecimalBits> NthRoot)
  {
    PL_REPORT_FAILURE("This function is not really implemented yet.");

    return (plFixedPoint<DecimalBits>)pow(f.ToDouble(), 1.0 / NthRoot.ToDouble());
  }

  template <plUInt8 DecimalBits>
  plFixedPoint<DecimalBits> Sqrt(plFixedPoint<DecimalBits> a)
  {
    return (plFixedPoint<DecimalBits>)sqrt(a.ToDouble());
    //if (a <= plFixedPoint<DecimalBits>(0))
    //  return plFixedPoint<DecimalBits>(0);

    //plFixedPoint<DecimalBits> x = a / 2;

    //for (plUInt32 i = 0; i < 8; ++i)
    //{
    //  plFixedPoint<DecimalBits> ax = a / x;
    //  plFixedPoint<DecimalBits> xpax = x + ax;
    //  x = xpax / 2;
    //}

    //return x;
  }

  template <plUInt8 DecimalBits>
  PL_FORCE_INLINE plFixedPoint<DecimalBits> Mod(plFixedPoint<DecimalBits> f, plFixedPoint<DecimalBits> div)
  {
    PL_REPORT_FAILURE("This function is not really implemented yet.");

    return (plFixedPoint<DecimalBits>)fmod(f.ToDouble(), div);
  }
}
*/

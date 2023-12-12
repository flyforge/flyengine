#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec2.h>

/// ********************* Binary to Int conversion *********************
/// Most significant bit comes first.
/// Adapted from http://bytes.com/topic/c/answers/219656-literal-binary
///
/// Sample usage:
/// PLASMA_8BIT(01010101) == 85
/// PLASMA_16BIT(10101010, 01010101) == 43605
/// PLASMA_32BIT(10000000, 11111111, 10101010, 01010101) == 2164238933
/// ********************************************************************
#define OCT__(n) 0##n##LU

#define PLASMA_8BIT__(iBits)                                                                                                           \
  (((iBits & 000000001) ? 1 : 0) + ((iBits & 000000010) ? 2 : 0) + ((iBits & 000000100) ? 4 : 0) + ((iBits & 000001000) ? 8 : 0) + \
    ((iBits & 000010000) ? 16 : 0) + ((iBits & 000100000) ? 32 : 0) + ((iBits & 001000000) ? 64 : 0) + ((iBits & 010000000) ? 128 : 0))

#define PLASMA_8BIT(B) ((plUInt8)PLASMA_8BIT__(OCT__(B)))

#define PLASMA_16BIT(B2, B1) (((plUInt8)PLASMA_8BIT(B2) << 8) + PLASMA_8BIT(B1))

#define PLASMA_32BIT(B4, B3, B2, B1) \
  ((unsigned long)PLASMA_8BIT(B4) << 24) + ((unsigned long)PLASMA_8BIT(B3) << 16) + ((unsigned long)PLASMA_8BIT(B2) << 8) + ((unsigned long)PLASMA_8BIT(B1))

namespace
{
  struct UniqueInt
  {
    int i, id;
    UniqueInt(int i, int id)
      : i(i)
      , id(id)
    {
    }

    bool operator<(const UniqueInt& rh) { return this->i < rh.i; }

    bool operator>(const UniqueInt& rh) { return this->i > rh.i; }
  };
}; // namespace


PLASMA_CREATE_SIMPLE_TEST_GROUP(Math);

PLASMA_CREATE_SIMPLE_TEST(Math, General)
{
  // PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constants")
  //{
  //  // Macro test
  //  PLASMA_TEST_BOOL(PLASMA_8BIT(01010101) == 85);
  //  PLASMA_TEST_BOOL(PLASMA_16BIT(10101010, 01010101) == 43605);
  //  PLASMA_TEST_BOOL(PLASMA_32BIT(10000000, 11111111, 10101010, 01010101) == 2164238933);

  //  // Infinity test
  //  //                           Sign:_
  //  //                       Exponent: _______  _
  //  //                       Fraction:           _______  ________  ________
  //  plIntFloatUnion uInf = { PLASMA_32BIT(01111111, 10000000, 00000000, 00000000) };
  //  PLASMA_TEST_BOOL(uInf.f == plMath::FloatInfinity());

  //  // FloatMax_Pos test
  //  plIntFloatUnion uMax = { PLASMA_32BIT(01111111, 01111111, 11111111, 11111111) };
  //  PLASMA_TEST_BOOL(uMax.f == plMath::FloatMax_Pos());

  //  // FloatMax_Neg test
  //  plIntFloatUnion uMin = { PLASMA_32BIT(11111111, 01111111, 11111111, 11111111) };
  //  PLASMA_TEST_BOOL(uMin.f == plMath::FloatMax_Neg());
  //}

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Sin")
  {
    PLASMA_TEST_FLOAT(plMath::Sin(plAngle::Degree(0.0f)), 0.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Sin(plAngle::Degree(90.0f)), 1.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Sin(plAngle::Degree(180.0f)), 0.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Sin(plAngle::Degree(270.0f)), -1.0f, 0.000001f);

    PLASMA_TEST_FLOAT(plMath::Sin(plAngle::Degree(45.0f)), 0.7071067f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Sin(plAngle::Degree(135.0f)), 0.7071067f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Sin(plAngle::Degree(225.0f)), -0.7071067f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Sin(plAngle::Degree(315.0f)), -0.7071067f, 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Cos")
  {
    PLASMA_TEST_FLOAT(plMath::Cos(plAngle::Degree(0.0f)), 1.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Cos(plAngle::Degree(90.0f)), 0.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Cos(plAngle::Degree(180.0f)), -1.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Cos(plAngle::Degree(270.0f)), 0.0f, 0.000001f);

    PLASMA_TEST_FLOAT(plMath::Cos(plAngle::Degree(45.0f)), 0.7071067f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Cos(plAngle::Degree(135.0f)), -0.7071067f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Cos(plAngle::Degree(225.0f)), -0.7071067f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Cos(plAngle::Degree(315.0f)), 0.7071067f, 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Tan")
  {
    PLASMA_TEST_FLOAT(plMath::Tan(plAngle::Degree(0.0f)), 0.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Tan(plAngle::Degree(45.0f)), 1.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Tan(plAngle::Degree(-45.0f)), -1.0f, 0.000001f);
    PLASMA_TEST_BOOL(plMath::Tan(plAngle::Degree(90.00001f)) < 1000000.0f);
    PLASMA_TEST_BOOL(plMath::Tan(plAngle::Degree(89.9999f)) > 100000.0f);

    // Testing the period of tan(x) centered at 0 and the adjacent ones
    plAngle angle = plAngle::Degree(-89.0f);
    while (angle.GetDegree() < 89.0f)
    {
      float fTan = plMath::Tan(angle);
      float fTanPrev = plMath::Tan(plAngle::Degree(angle.GetDegree() - 180.0f));
      float fTanNext = plMath::Tan(plAngle::Degree(angle.GetDegree() + 180.0f));
      float fSin = plMath::Sin(angle);
      float fCos = plMath::Cos(angle);

      PLASMA_TEST_FLOAT(fTan - fTanPrev, 0.0f, 0.002f);
      PLASMA_TEST_FLOAT(fTan - fTanNext, 0.0f, 0.002f);
      PLASMA_TEST_FLOAT(fTan - (fSin / fCos), 0.0f, 0.0005f);
      angle += plAngle::Degree(1.234f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ASin")
  {
    PLASMA_TEST_FLOAT(plMath::ASin(0.0f).GetDegree(), 0.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plMath::ASin(1.0f).GetDegree(), 90.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plMath::ASin(-1.0f).GetDegree(), -90.0f, 0.00001f);

    PLASMA_TEST_FLOAT(plMath::ASin(0.7071067f).GetDegree(), 45.0f, 0.0001f);
    PLASMA_TEST_FLOAT(plMath::ASin(-0.7071067f).GetDegree(), -45.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ACos")
  {
    PLASMA_TEST_FLOAT(plMath::ACos(0.0f).GetDegree(), 90.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plMath::ACos(1.0f).GetDegree(), 0.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plMath::ACos(-1.0f).GetDegree(), 180.0f, 0.0001f);

    PLASMA_TEST_FLOAT(plMath::ACos(0.7071067f).GetDegree(), 45.0f, 0.0001f);
    PLASMA_TEST_FLOAT(plMath::ACos(-0.7071067f).GetDegree(), 135.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ATan")
  {
    PLASMA_TEST_FLOAT(plMath::ATan(0.0f).GetDegree(), 0.0f, 0.0000001f);
    PLASMA_TEST_FLOAT(plMath::ATan(1.0f).GetDegree(), 45.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plMath::ATan(-1.0f).GetDegree(), -45.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plMath::ATan(10000000.0f).GetDegree(), 90.0f, 0.00002f);
    PLASMA_TEST_FLOAT(plMath::ATan(-10000000.0f).GetDegree(), -90.0f, 0.00002f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ATan2")
  {
    for (float fScale = 0.125f; fScale < 1000000.0f; fScale *= 2.0f)
    {
      PLASMA_TEST_FLOAT(plMath::ATan2(0.0f, fScale).GetDegree(), 0.0f, 0.0000001f);
      PLASMA_TEST_FLOAT(plMath::ATan2(fScale, fScale).GetDegree(), 45.0f, 0.00001f);
      PLASMA_TEST_FLOAT(plMath::ATan2(fScale, 0.0f).GetDegree(), 90.0f, 0.00001f);
      PLASMA_TEST_FLOAT(plMath::ATan2(-fScale, fScale).GetDegree(), -45.0f, 0.00001f);
      PLASMA_TEST_FLOAT(plMath::ATan2(-fScale, 0.0f).GetDegree(), -90.0f, 0.00001f);
      PLASMA_TEST_FLOAT(plMath::ATan2(0.0f, -fScale).GetDegree(), 180.0f, 0.0001f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Exp")
  {
    PLASMA_TEST_FLOAT(1.0f, plMath::Exp(0.0f), 0.000001f);
    PLASMA_TEST_FLOAT(2.7182818284f, plMath::Exp(1.0f), 0.000001f);
    PLASMA_TEST_FLOAT(7.3890560989f, plMath::Exp(2.0f), 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Ln")
  {
    PLASMA_TEST_FLOAT(0.0f, plMath::Ln(1.0f), 0.000001f);
    PLASMA_TEST_FLOAT(1.0f, plMath::Ln(2.7182818284f), 0.000001f);
    PLASMA_TEST_FLOAT(2.0f, plMath::Ln(7.3890560989f), 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Log2")
  {
    PLASMA_TEST_FLOAT(0.0f, plMath::Log2(1.0f), 0.000001f);
    PLASMA_TEST_FLOAT(1.0f, plMath::Log2(2.0f), 0.000001f);
    PLASMA_TEST_FLOAT(2.0f, plMath::Log2(4.0f), 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Log2i")
  {
    PLASMA_TEST_BOOL(plMath::Log2i(0) == plUInt32(-1));
    PLASMA_TEST_BOOL(plMath::Log2i(1) == 0);
    PLASMA_TEST_BOOL(plMath::Log2i(2) == 1);
    PLASMA_TEST_BOOL(plMath::Log2i(3) == 1);
    PLASMA_TEST_BOOL(plMath::Log2i(4) == 2);
    PLASMA_TEST_BOOL(plMath::Log2i(6) == 2);
    PLASMA_TEST_BOOL(plMath::Log2i(7) == 2);
    PLASMA_TEST_BOOL(plMath::Log2i(8) == 3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Log10")
  {
    PLASMA_TEST_FLOAT(0.0f, plMath::Log10(1.0f), 0.000001f);
    PLASMA_TEST_FLOAT(1.0f, plMath::Log10(10.0f), 0.000001f);
    PLASMA_TEST_FLOAT(2.0f, plMath::Log10(100.0f), 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Log")
  {
    PLASMA_TEST_FLOAT(0.0f, plMath::Log(2.7182818284f, 1.0f), 0.000001f);
    PLASMA_TEST_FLOAT(1.0f, plMath::Log(2.7182818284f, 2.7182818284f), 0.000001f);
    PLASMA_TEST_FLOAT(2.0f, plMath::Log(2.7182818284f, 7.3890560989f), 0.000001f);

    PLASMA_TEST_FLOAT(0.0f, plMath::Log(2.0f, 1.0f), 0.000001f);
    PLASMA_TEST_FLOAT(1.0f, plMath::Log(2.0f, 2.0f), 0.000001f);
    PLASMA_TEST_FLOAT(2.0f, plMath::Log(2.0f, 4.0f), 0.000001f);

    PLASMA_TEST_FLOAT(0.0f, plMath::Log(10.0f, 1.0f), 0.000001f);
    PLASMA_TEST_FLOAT(1.0f, plMath::Log(10.0f, 10.0f), 0.000001f);
    PLASMA_TEST_FLOAT(2.0f, plMath::Log(10.0f, 100.0f), 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Pow2")
  {
    PLASMA_TEST_FLOAT(1.0f, plMath::Pow2(0.0f), 0.000001f);
    PLASMA_TEST_FLOAT(2.0f, plMath::Pow2(1.0f), 0.000001f);
    PLASMA_TEST_FLOAT(4.0f, plMath::Pow2(2.0f), 0.000001f);

    PLASMA_TEST_BOOL(plMath::Pow2(0) == 1);
    PLASMA_TEST_BOOL(plMath::Pow2(1) == 2);
    PLASMA_TEST_BOOL(plMath::Pow2(2) == 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Pow")
  {
    PLASMA_TEST_FLOAT(1.0f, plMath::Pow(3.0f, 0.0f), 0.000001f);
    PLASMA_TEST_FLOAT(3.0f, plMath::Pow(3.0f, 1.0f), 0.000001f);
    PLASMA_TEST_FLOAT(9.0f, plMath::Pow(3.0f, 2.0f), 0.000001f);

    PLASMA_TEST_BOOL(plMath::Pow(3, 0) == 1);
    PLASMA_TEST_BOOL(plMath::Pow(3, 1) == 3);
    PLASMA_TEST_BOOL(plMath::Pow(3, 2) == 9);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Square")
  {
    PLASMA_TEST_FLOAT(0.0f, plMath::Square(0.0f), 0.000001f);
    PLASMA_TEST_FLOAT(1.0f, plMath::Square(1.0f), 0.000001f);
    PLASMA_TEST_FLOAT(4.0f, plMath::Square(2.0f), 0.000001f);
    PLASMA_TEST_FLOAT(4.0f, plMath::Square(-2.0f), 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Sqrt (float)")
  {
    PLASMA_TEST_FLOAT(0.0f, plMath::Sqrt(0.0f), 0.000001f);
    PLASMA_TEST_FLOAT(1.0f, plMath::Sqrt(1.0f), 0.000001f);
    PLASMA_TEST_FLOAT(2.0f, plMath::Sqrt(4.0f), 0.000001f);
    PLASMA_TEST_FLOAT(4.0f, plMath::Sqrt(16.0f), 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Sqrt (double)")
  {
    PLASMA_TEST_DOUBLE(0.0, plMath::Sqrt(0.0), 0.000001);
    PLASMA_TEST_DOUBLE(1.0, plMath::Sqrt(1.0), 0.000001);
    PLASMA_TEST_DOUBLE(2.0, plMath::Sqrt(4.0), 0.000001);
    PLASMA_TEST_DOUBLE(4.0, plMath::Sqrt(16.0), 0.000001);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Root")
  {
    PLASMA_TEST_FLOAT(3.0f, plMath::Root(27.0f, 3.0f), 0.000001f);
    PLASMA_TEST_FLOAT(3.0f, plMath::Root(81.0f, 4.0f), 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Sign")
  {
    PLASMA_TEST_FLOAT(0.0f, plMath::Sign(0.0f), 0.00000001f);
    PLASMA_TEST_FLOAT(1.0f, plMath::Sign(0.01f), 0.00000001f);
    PLASMA_TEST_FLOAT(-1.0f, plMath::Sign(-0.01f), 0.00000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Abs")
  {
    PLASMA_TEST_FLOAT(0.0f, plMath::Abs(0.0f), 0.00000001f);
    PLASMA_TEST_FLOAT(20.0f, plMath::Abs(20.0f), 0.00000001f);
    PLASMA_TEST_FLOAT(20.0f, plMath::Abs(-20.0f), 0.00000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Min")
  {
    PLASMA_TEST_FLOAT(0.0f, plMath::Min(0.0f, 23.0f), 0.00000001f);
    PLASMA_TEST_FLOAT(-23.0f, plMath::Min(0.0f, -23.0f), 0.00000001f);

    PLASMA_TEST_BOOL(plMath::Min(1, 2, 3) == 1);
    PLASMA_TEST_BOOL(plMath::Min(4, 2, 3) == 2);
    PLASMA_TEST_BOOL(plMath::Min(4, 5, 3) == 3);

    PLASMA_TEST_BOOL(plMath::Min(1, 2, 3, 4) == 1);
    PLASMA_TEST_BOOL(plMath::Min(5, 2, 3, 4) == 2);
    PLASMA_TEST_BOOL(plMath::Min(5, 6, 3, 4) == 3);
    PLASMA_TEST_BOOL(plMath::Min(5, 6, 7, 4) == 4);

    PLASMA_TEST_BOOL(plMath::Min(UniqueInt(1, 0), UniqueInt(1, 1)).id == 0);
    PLASMA_TEST_BOOL(plMath::Min(UniqueInt(1, 1), UniqueInt(1, 0)).id == 1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Max")
  {
    PLASMA_TEST_FLOAT(23.0f, plMath::Max(0.0f, 23.0f), 0.00000001f);
    PLASMA_TEST_FLOAT(0.0f, plMath::Max(0.0f, -23.0f), 0.00000001f);

    PLASMA_TEST_BOOL(plMath::Max(1, 2, 3) == 3);
    PLASMA_TEST_BOOL(plMath::Max(1, 2, 0) == 2);
    PLASMA_TEST_BOOL(plMath::Max(1, 0, 0) == 1);

    PLASMA_TEST_BOOL(plMath::Max(1, 2, 3, 4) == 4);
    PLASMA_TEST_BOOL(plMath::Max(1, 2, 3, 0) == 3);
    PLASMA_TEST_BOOL(plMath::Max(1, 2, 0, 0) == 2);
    PLASMA_TEST_BOOL(plMath::Max(1, 0, 0, 0) == 1);

    PLASMA_TEST_BOOL(plMath::Max(UniqueInt(1, 0), UniqueInt(1, 1)).id == 0);
    PLASMA_TEST_BOOL(plMath::Max(UniqueInt(1, 1), UniqueInt(1, 0)).id == 1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clamp")
  {
    PLASMA_TEST_FLOAT(15.0f, plMath::Clamp(23.0f, 12.0f, 15.0f), 0.00000001f);
    PLASMA_TEST_FLOAT(12.0f, plMath::Clamp(3.0f, 12.0f, 15.0f), 0.00000001f);
    PLASMA_TEST_FLOAT(14.0f, plMath::Clamp(14.0f, 12.0f, 15.0f), 0.00000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Saturate")
  {
    PLASMA_TEST_FLOAT(0.0f, plMath::Saturate(-1.5f), 0.00000001f);
    PLASMA_TEST_FLOAT(0.5f, plMath::Saturate(0.5f), 0.00000001f);
    PLASMA_TEST_FLOAT(1.0f, plMath::Saturate(12345.0f), 0.00000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Floor")
  {
    PLASMA_TEST_BOOL(12 == plMath::Floor(12.34f));
    PLASMA_TEST_BOOL(-13 == plMath::Floor(-12.34f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Ceil")
  {
    PLASMA_TEST_BOOL(13 == plMath::Ceil(12.34f));
    PLASMA_TEST_BOOL(-12 == plMath::Ceil(-12.34f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RoundDown (float)")
  {
    PLASMA_TEST_FLOAT(10.0f, plMath::RoundDown(12.34f, 5.0f), 0.0000001f);
    PLASMA_TEST_FLOAT(-15.0f, plMath::RoundDown(-12.34f, 5.0f), 0.0000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RoundUp (float)")
  {
    PLASMA_TEST_FLOAT(15.0f, plMath::RoundUp(12.34f, 5.0f), 0.0000001f);
    PLASMA_TEST_FLOAT(-10.0f, plMath::RoundUp(-12.34f, 5.0f), 0.0000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RoundDown (double)")
  {
    PLASMA_TEST_DOUBLE(10.0, plMath::RoundDown(12.34, 5.0), 0.0000001);
    PLASMA_TEST_DOUBLE(-15.0, plMath::RoundDown(-12.34, 5.0), 0.0000001);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RoundUp (double)")
  {
    PLASMA_TEST_DOUBLE(15.0, plMath::RoundUp(12.34, 5.0), 0.0000001);
    PLASMA_TEST_DOUBLE(-10.0, plMath::RoundUp(-12.34, 5.0), 0.0000001);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Trunc")
  {
    PLASMA_TEST_BOOL(plMath::Trunc(12.34f) == 12);
    PLASMA_TEST_BOOL(plMath::Trunc(-12.34f) == -12);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FloatToInt")
  {
    PLASMA_TEST_BOOL(plMath::FloatToInt(12.34f) == 12);
    PLASMA_TEST_BOOL(plMath::FloatToInt(-12.34f) == -12);

#if PLASMA_DISABLED(PLASMA_PLATFORM_ARCH_X86) || (_MSC_VER <= 1916)
    PLASMA_TEST_BOOL(plMath::FloatToInt(12000000000000.34) == 12000000000000);
    PLASMA_TEST_BOOL(plMath::FloatToInt(-12000000000000.34) == -12000000000000);
#endif
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Round")
  {
    PLASMA_TEST_BOOL(plMath::Round(12.34f) == 12);
    PLASMA_TEST_BOOL(plMath::Round(-12.34f) == -12);

    PLASMA_TEST_BOOL(plMath::Round(12.54f) == 13);
    PLASMA_TEST_BOOL(plMath::Round(-12.54f) == -13);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RoundClosest (float)")
  {
    PLASMA_TEST_FLOAT(plMath::RoundToMultiple(12.0f, 3.0f), 12.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plMath::RoundToMultiple(-12.0f, 3.0f), -12.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plMath::RoundToMultiple(12.34f, 7.0f), 14.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plMath::RoundToMultiple(-12.34f, 7.0f), -14.0f, 0.00001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RoundClosest (double)")
  {
    PLASMA_TEST_DOUBLE(plMath::RoundToMultiple(12.0, 3.0), 12.0, 0.00001);
    PLASMA_TEST_DOUBLE(plMath::RoundToMultiple(-12.0, 3.0), -12.0, 0.00001);
    PLASMA_TEST_DOUBLE(plMath::RoundToMultiple(12.34, 7.0), 14.0, 0.00001);
    PLASMA_TEST_DOUBLE(plMath::RoundToMultiple(-12.34, 7.0), -14.0, 0.00001);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RoundUp (int)")
  {
    PLASMA_TEST_INT(plMath::RoundUp(12, 7), 14);
    PLASMA_TEST_INT(plMath::RoundUp(-12, 7), -7);
    PLASMA_TEST_INT(plMath::RoundUp(16, 4), 16);
    PLASMA_TEST_INT(plMath::RoundUp(-16, 4), -16);
    PLASMA_TEST_INT(plMath::RoundUp(17, 4), 20);
    PLASMA_TEST_INT(plMath::RoundUp(-17, 4), -16);
    PLASMA_TEST_INT(plMath::RoundUp(15, 4), 16);
    PLASMA_TEST_INT(plMath::RoundUp(-15, 4), -12);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RoundDown (int)")
  {
    PLASMA_TEST_INT(plMath::RoundDown(12, 7), 7);
    PLASMA_TEST_INT(plMath::RoundDown(-12, 7), -14);
    PLASMA_TEST_INT(plMath::RoundDown(16, 4), 16);
    PLASMA_TEST_INT(plMath::RoundDown(-16, 4), -16);
    PLASMA_TEST_INT(plMath::RoundDown(17, 4), 16);
    PLASMA_TEST_INT(plMath::RoundDown(-17, 4), -20);
    PLASMA_TEST_INT(plMath::RoundDown(15, 4), 12);
    PLASMA_TEST_INT(plMath::RoundDown(-15, 4), -16);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RoundUp (unsigned int)")
  {
    PLASMA_TEST_INT(plMath::RoundUp(12u, 7), 14);
    PLASMA_TEST_INT(plMath::RoundUp(16u, 4), 16);
    PLASMA_TEST_INT(plMath::RoundUp(17u, 4), 20);
    PLASMA_TEST_INT(plMath::RoundUp(15u, 4), 16);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RoundDown (unsigned int)")
  {
    PLASMA_TEST_INT(plMath::RoundDown(12u, 7), 7);
    PLASMA_TEST_INT(plMath::RoundDown(16u, 4), 16);
    PLASMA_TEST_INT(plMath::RoundDown(17u, 4), 16);
    PLASMA_TEST_INT(plMath::RoundDown(15u, 4), 12);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Fraction")
  {
    PLASMA_TEST_FLOAT(plMath::Fraction(12.34f), 0.34f, 0.00001f);
    PLASMA_TEST_FLOAT(plMath::Fraction(-12.34f), -0.34f, 0.00001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Mod (float)")
  {
    PLASMA_TEST_FLOAT(2.34f, plMath::Mod(12.34f, 2.5f), 0.000001f);
    PLASMA_TEST_FLOAT(-2.34f, plMath::Mod(-12.34f, 2.5f), 0.000001f);

    PLASMA_TEST_FLOAT(2.34f, plMath::Mod(12.34f, -2.5f), 0.000001f);
    PLASMA_TEST_FLOAT(-2.34f, plMath::Mod(-12.34f, -2.5f), 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Mod (double)")
  {
    PLASMA_TEST_DOUBLE(2.34, plMath::Mod(12.34, 2.5), 0.000001);
    PLASMA_TEST_DOUBLE(-2.34, plMath::Mod(-12.34, 2.5), 0.000001);

    PLASMA_TEST_DOUBLE(2.34, plMath::Mod(12.34, -2.5), 0.000001);
    PLASMA_TEST_DOUBLE(-2.34, plMath::Mod(-12.34, -2.5), 0.000001);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Invert")
  {
    PLASMA_TEST_FLOAT(plMath::Invert(1.0f), 1.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Invert(2.0f), 0.5f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Invert(4.0f), 0.25f, 0.000001f);

    PLASMA_TEST_FLOAT(plMath::Invert(-1.0f), -1.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Invert(-2.0f), -0.5f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::Invert(-4.0f), -0.25f, 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Odd")
  {
    PLASMA_TEST_BOOL(plMath::IsOdd(0) == false);
    PLASMA_TEST_BOOL(plMath::IsOdd(1) == true);
    PLASMA_TEST_BOOL(plMath::IsOdd(2) == false);
    PLASMA_TEST_BOOL(plMath::IsOdd(-1) == true);
    PLASMA_TEST_BOOL(plMath::IsOdd(-2) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Even")
  {
    PLASMA_TEST_BOOL(plMath::IsEven(0) == true);
    PLASMA_TEST_BOOL(plMath::IsEven(1) == false);
    PLASMA_TEST_BOOL(plMath::IsEven(2) == true);
    PLASMA_TEST_BOOL(plMath::IsEven(-1) == false);
    PLASMA_TEST_BOOL(plMath::IsEven(-2) == true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Swap")
  {
    plInt32 a = 1;
    plInt32 b = 2;
    plMath::Swap(a, b);
    PLASMA_TEST_BOOL((a == 2) && (b == 1));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Lerp")
  {
    PLASMA_TEST_FLOAT(plMath::Lerp(-5.0f, 5.0f, 0.5f), 0.0f, 0.000001);
    PLASMA_TEST_FLOAT(plMath::Lerp(0.0f, 5.0f, 0.5f), 2.5f, 0.000001);
    PLASMA_TEST_FLOAT(plMath::Lerp(-5.0f, 5.0f, 0.0f), -5.0f, 0.000001);
    PLASMA_TEST_FLOAT(plMath::Lerp(-5.0f, 5.0f, 1.0f), 5.0f, 0.000001);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Step")
  {
    PLASMA_TEST_FLOAT(plMath::Step(0.5f, 0.4f), 1.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plMath::Step(0.3f, 0.4f), 0.0f, 0.00001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SmoothStep")
  {
    // Only test values that must be true for any symmetric step function.
    // How should one test smoothness?
    for (int iScale = -19; iScale <= 19; iScale += 2)
    {
      PLASMA_TEST_FLOAT(plMath::SmoothStep(0.0f * iScale, 0.1f * iScale, 0.4f * iScale), 0.0f, 0.000001);
      PLASMA_TEST_FLOAT(plMath::SmoothStep(0.1f * iScale, 0.1f * iScale, 0.4f * iScale), 0.0f, 0.000001);
      PLASMA_TEST_FLOAT(plMath::SmoothStep(0.4f * iScale, 0.1f * iScale, 0.4f * iScale), 1.0f, 0.000001);
      PLASMA_TEST_FLOAT(plMath::SmoothStep(0.25f * iScale, 0.1f * iScale, 0.4f * iScale), 0.5f, 0.000001);
      PLASMA_TEST_FLOAT(plMath::SmoothStep(0.5f * iScale, 0.1f * iScale, 0.4f * iScale), 1.0f, 0.000001);

      PLASMA_TEST_FLOAT(plMath::SmoothStep(0.5f * iScale, 0.4f * iScale, 0.1f * iScale), 0.0f, 0.000001);
      PLASMA_TEST_FLOAT(plMath::SmoothStep(0.4f * iScale, 0.4f * iScale, 0.1f * iScale), 0.0f, 0.000001);
      PLASMA_TEST_FLOAT(plMath::SmoothStep(0.1f * iScale, 0.4f * iScale, 0.1f * iScale), 1.0f, 0.000001);
      PLASMA_TEST_FLOAT(plMath::SmoothStep(0.25f * iScale, 0.1f * iScale, 0.4f * iScale), 0.5f, 0.000001);
      PLASMA_TEST_FLOAT(plMath::SmoothStep(0.0f * iScale, 0.4f * iScale, 0.1f * iScale), 1.0f, 0.000001);

      // For edge1 == edge2 SmoothStep should behave like Step
      PLASMA_TEST_FLOAT(plMath::SmoothStep(0.0f * iScale, 0.1f * iScale, 0.1f * iScale), iScale > 0 ? 0.0f : 1.0f, 0.000001);
      PLASMA_TEST_FLOAT(plMath::SmoothStep(0.2f * iScale, 0.1f * iScale, 0.1f * iScale), iScale < 0 ? 0.0f : 1.0f, 0.000001);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsPowerOf")
  {
    PLASMA_TEST_BOOL(plMath::IsPowerOf(4, 2) == true);
    PLASMA_TEST_BOOL(plMath::IsPowerOf(5, 2) == false);
    PLASMA_TEST_BOOL(plMath::IsPowerOf(0, 2) == false);
    PLASMA_TEST_BOOL(plMath::IsPowerOf(1, 2) == true);

    PLASMA_TEST_BOOL(plMath::IsPowerOf(4, 3) == false);
    PLASMA_TEST_BOOL(plMath::IsPowerOf(3, 3) == true);
    PLASMA_TEST_BOOL(plMath::IsPowerOf(1, 3) == true);
    PLASMA_TEST_BOOL(plMath::IsPowerOf(27, 3) == true);
    PLASMA_TEST_BOOL(plMath::IsPowerOf(28, 3) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsPowerOf2")
  {
    PLASMA_TEST_BOOL(plMath::IsPowerOf2(4) == true);
    PLASMA_TEST_BOOL(plMath::IsPowerOf2(5) == false);
    PLASMA_TEST_BOOL(plMath::IsPowerOf2(0) == false);
    PLASMA_TEST_BOOL(plMath::IsPowerOf2(1) == true);
    PLASMA_TEST_BOOL(plMath::IsPowerOf2(0x7FFFFFFFu) == false);
    PLASMA_TEST_BOOL(plMath::IsPowerOf2(0x80000000u) == true);
    PLASMA_TEST_BOOL(plMath::IsPowerOf2(0x80000001u) == false);
    PLASMA_TEST_BOOL(plMath::IsPowerOf2(0xFFFFFFFFu) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PowerOf2_Floor")
  {
    PLASMA_TEST_INT(plMath::PowerOfTwo_Floor(64), 64);
    PLASMA_TEST_INT(plMath::PowerOfTwo_Floor(33), 32);
    PLASMA_TEST_INT(plMath::PowerOfTwo_Floor(4), 4);
    PLASMA_TEST_INT(plMath::PowerOfTwo_Floor(5), 4);
    PLASMA_TEST_INT(plMath::PowerOfTwo_Floor(1), 1);
    PLASMA_TEST_INT(plMath::PowerOfTwo_Floor(0x80000000), 0x80000000);
    PLASMA_TEST_INT(plMath::PowerOfTwo_Floor(0x80000001), 0x80000000);
    // strange case...
    PLASMA_TEST_INT(plMath::PowerOfTwo_Floor(0), 1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PowerOf2_Ceil")
  {
    PLASMA_TEST_INT(plMath::PowerOfTwo_Ceil(64), 64);
    PLASMA_TEST_INT(plMath::PowerOfTwo_Ceil(33), 64);
    PLASMA_TEST_INT(plMath::PowerOfTwo_Ceil(4), 4);
    PLASMA_TEST_INT(plMath::PowerOfTwo_Ceil(5), 8);
    PLASMA_TEST_INT(plMath::PowerOfTwo_Ceil(1), 1);
    PLASMA_TEST_INT(plMath::PowerOfTwo_Ceil(0), 1);
    PLASMA_TEST_INT(plMath::PowerOfTwo_Ceil(0x7FFFFFFF), 0x80000000);
    PLASMA_TEST_INT(plMath::PowerOfTwo_Ceil(0x80000000), 0x80000000);
    // anything above 0x80000000 is undefined behavior due to how left-shift works
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GreatestCommonDivisor")
  {
    PLASMA_TEST_INT(plMath::GreatestCommonDivisor(13, 13), 13);
    PLASMA_TEST_INT(plMath::GreatestCommonDivisor(37, 600), 1);
    PLASMA_TEST_INT(plMath::GreatestCommonDivisor(20, 100), 20);
    PLASMA_TEST_INT(plMath::GreatestCommonDivisor(624129, 2061517), 18913);
  }


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqual")
  {
    PLASMA_TEST_BOOL(plMath::IsEqual(1.0f, 0.999f, 0.01f) == true);
    PLASMA_TEST_BOOL(plMath::IsEqual(1.0f, 1.001f, 0.01f) == true);
    PLASMA_TEST_BOOL(plMath::IsEqual(1.0f, 0.999f, 0.0001f) == false);
    PLASMA_TEST_BOOL(plMath::IsEqual(1.0f, 1.001f, 0.0001f) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "NaN_Infinity")
  {
    if (plMath::SupportsNaN<plMathTestType>())
    {
      PLASMA_TEST_BOOL(plMath::IsNaN(plMath::NaN<plMathTestType>()) == true);

      PLASMA_TEST_BOOL(plMath::Infinity<plMathTestType>() == plMath::Infinity<plMathTestType>() - (plMathTestType)1);
      PLASMA_TEST_BOOL(plMath::Infinity<plMathTestType>() == plMath::Infinity<plMathTestType>() + (plMathTestType)1);

      PLASMA_TEST_BOOL(plMath::IsNaN(plMath::Infinity<plMathTestType>() - plMath::Infinity<plMathTestType>()));

      PLASMA_TEST_BOOL(!plMath::IsFinite(plMath::Infinity<plMathTestType>()));
      PLASMA_TEST_BOOL(!plMath::IsFinite(-plMath::Infinity<plMathTestType>()));
      PLASMA_TEST_BOOL(!plMath::IsFinite(plMath::NaN<plMathTestType>()));
      PLASMA_TEST_BOOL(!plMath::IsNaN(plMath::Infinity<plMathTestType>()));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsInRange")
  {
    PLASMA_TEST_BOOL(plMath::IsInRange(1.0f, 0.0f, 2.0f) == true);
    PLASMA_TEST_BOOL(plMath::IsInRange(1.0f, 0.0f, 1.0f) == true);
    PLASMA_TEST_BOOL(plMath::IsInRange(1.0f, 1.0f, 2.0f) == true);
    PLASMA_TEST_BOOL(plMath::IsInRange(0.0f, 1.0f, 2.0f) == false);
    PLASMA_TEST_BOOL(plMath::IsInRange(3.0f, 0.0f, 2.0f) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsZero")
  {
    PLASMA_TEST_BOOL(plMath::IsZero(0.009f, 0.01f) == true);
    PLASMA_TEST_BOOL(plMath::IsZero(0.001f, 0.01f) == true);
    PLASMA_TEST_BOOL(plMath::IsZero(0.009f, 0.0001f) == false);
    PLASMA_TEST_BOOL(plMath::IsZero(0.001f, 0.0001f) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ColorFloatToByte")
  {
    PLASMA_TEST_INT(plMath::ColorFloatToByte(plMath::NaN<float>()), 0);
    PLASMA_TEST_INT(plMath::ColorFloatToByte(-1.0f), 0);
    PLASMA_TEST_INT(plMath::ColorFloatToByte(0.0f), 0);
    PLASMA_TEST_INT(plMath::ColorFloatToByte(0.4f), 102);
    PLASMA_TEST_INT(plMath::ColorFloatToByte(1.0f), 255);
    PLASMA_TEST_INT(plMath::ColorFloatToByte(1.5f), 255);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ColorFloatToShort")
  {
    PLASMA_TEST_INT(plMath::ColorFloatToShort(plMath::NaN<float>()), 0);
    PLASMA_TEST_INT(plMath::ColorFloatToShort(-1.0f), 0);
    PLASMA_TEST_INT(plMath::ColorFloatToShort(0.0f), 0);
    PLASMA_TEST_INT(plMath::ColorFloatToShort(0.4f), 26214);
    PLASMA_TEST_INT(plMath::ColorFloatToShort(1.0f), 65535);
    PLASMA_TEST_INT(plMath::ColorFloatToShort(1.5f), 65535);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ColorFloatToSignedByte")
  {
    PLASMA_TEST_INT(plMath::ColorFloatToSignedByte(plMath::NaN<float>()), 0);
    PLASMA_TEST_INT(plMath::ColorFloatToSignedByte(-1.0f), -127);
    PLASMA_TEST_INT(plMath::ColorFloatToSignedByte(0.0f), 0);
    PLASMA_TEST_INT(plMath::ColorFloatToSignedByte(0.4f), 51);
    PLASMA_TEST_INT(plMath::ColorFloatToSignedByte(1.0f), 127);
    PLASMA_TEST_INT(plMath::ColorFloatToSignedByte(1.5f), 127);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ColorFloatToSignedShort")
  {
    PLASMA_TEST_INT(plMath::ColorFloatToSignedShort(plMath::NaN<float>()), 0);
    PLASMA_TEST_INT(plMath::ColorFloatToSignedShort(-1.0f), -32767);
    PLASMA_TEST_INT(plMath::ColorFloatToSignedShort(0.0f), 0);
    PLASMA_TEST_INT(plMath::ColorFloatToSignedShort(0.4f), 13107);
    PLASMA_TEST_INT(plMath::ColorFloatToSignedShort(0.5f), 16384);
    PLASMA_TEST_INT(plMath::ColorFloatToSignedShort(1.0f), 32767);
    PLASMA_TEST_INT(plMath::ColorFloatToSignedShort(1.5f), 32767);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ColorByteToFloat")
  {
    PLASMA_TEST_FLOAT(plMath::ColorByteToFloat(0), 0.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::ColorByteToFloat(128), 0.501960784f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::ColorByteToFloat(255), 1.0f, 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ColorShortToFloat")
  {
    PLASMA_TEST_FLOAT(plMath::ColorShortToFloat(0), 0.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::ColorShortToFloat(32768), 0.5000076f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::ColorShortToFloat(65535), 1.0f, 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ColorSignedByteToFloat")
  {
    PLASMA_TEST_FLOAT(plMath::ColorSignedByteToFloat(-128), -1.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::ColorSignedByteToFloat(-127), -1.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::ColorSignedByteToFloat(0), 0.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::ColorSignedByteToFloat(64), 0.50393700787f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::ColorSignedByteToFloat(127), 1.0f, 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ColorSignedShortToFloat")
  {
    PLASMA_TEST_FLOAT(plMath::ColorSignedShortToFloat(-32768), -1.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::ColorSignedShortToFloat(-32767), -1.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::ColorSignedShortToFloat(0), 0.0f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::ColorSignedShortToFloat(16384), 0.50001526f, 0.000001f);
    PLASMA_TEST_FLOAT(plMath::ColorSignedShortToFloat(32767), 1.0f, 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "EvaluateBezierCurve")
  {
    // Determined through the scientific method of manually comparing the result of the function with an online Bezier curve generator:
    // https://www.desmos.com/calculator/cahqdxeshd
    const plVec2 res[] = {plVec2(1, 5), plVec2(0.893f, 4.455f), plVec2(1.112f, 4.008f), plVec2(1.557f, 3.631f), plVec2(2.136f, 3.304f), plVec2(2.750f, 3.000f),
      plVec2(3.303f, 2.695f), plVec2(3.701f, 2.368f), plVec2(3.847f, 1.991f), plVec2(3.645f, 1.543f), plVec2(3, 1)};

    const float step = 1.0f / (PLASMA_ARRAY_SIZE(res) - 1);
    for (int i = 0; i < PLASMA_ARRAY_SIZE(res); ++i)
    {
      const plVec2 r = plMath::EvaluateBezierCurve<plVec2>(step * i, plVec2(1, 5), plVec2(0, 3), plVec2(6, 3), plVec2(3, 1));
      PLASMA_TEST_VEC2(r, res[i], 0.002f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FirstBitLow")
  {
    PLASMA_TEST_INT(plMath::FirstBitLow(plUInt32(0b1111)), 0);
    PLASMA_TEST_INT(plMath::FirstBitLow(plUInt32(0b1110)), 1);
    PLASMA_TEST_INT(plMath::FirstBitLow(plUInt32(0b1100)), 2);
    PLASMA_TEST_INT(plMath::FirstBitLow(plUInt32(0b1000)), 3);
    PLASMA_TEST_INT(plMath::FirstBitLow(plUInt32(0xFFFFFFFF)), 0);

    PLASMA_TEST_INT(plMath::FirstBitLow(plUInt64(0xFF000000FF00000F)), 0);
    PLASMA_TEST_INT(plMath::FirstBitLow(plUInt64(0xFF000000FF00000E)), 1);
    PLASMA_TEST_INT(plMath::FirstBitLow(plUInt64(0xFF000000FF00000C)), 2);
    PLASMA_TEST_INT(plMath::FirstBitLow(plUInt64(0xFF000000FF000008)), 3);
    PLASMA_TEST_INT(plMath::FirstBitLow(plUInt64(0xFFFFFFFFFFFFFFFF)), 0);

    // Edge cases specifically for 32-bit systems where upper and lower 32-bit are handled individually.
    PLASMA_TEST_INT(plMath::FirstBitLow(plUInt64(0x00000000FFFFFFFF)), 0);
    PLASMA_TEST_INT(plMath::FirstBitLow(plUInt64(0xFFFFFFFF00000000)), 32);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FirstBitHigh")
  {
    PLASMA_TEST_INT(plMath::FirstBitHigh(plUInt32(0b1111)), 3);
    PLASMA_TEST_INT(plMath::FirstBitHigh(plUInt32(0b0111)), 2);
    PLASMA_TEST_INT(plMath::FirstBitHigh(plUInt32(0b0011)), 1);
    PLASMA_TEST_INT(plMath::FirstBitHigh(plUInt32(0b0001)), 0);
    PLASMA_TEST_INT(plMath::FirstBitHigh(plUInt32(0xFFFFFFFF)), 31);

    PLASMA_TEST_INT(plMath::FirstBitHigh(plUInt64(0x00FF000000FF000F)), 55);
    PLASMA_TEST_INT(plMath::FirstBitHigh(plUInt64(0x007F000000FF000F)), 54);
    PLASMA_TEST_INT(plMath::FirstBitHigh(plUInt64(0x003F000000FF000F)), 53);
    PLASMA_TEST_INT(plMath::FirstBitHigh(plUInt64(0x001F000000FF000F)), 52);
    PLASMA_TEST_INT(plMath::FirstBitHigh(plUInt64(0xFFFFFFFFFFFFFFFF)), 63);

    // Edge cases specifically for 32-bit systems where upper and lower 32-bit are handled individually.
    PLASMA_TEST_INT(plMath::FirstBitHigh(plUInt64(0x00000000FFFFFFFF)), 31);
    PLASMA_TEST_INT(plMath::FirstBitHigh(plUInt64(0xFFFFFFFF00000000)), 63);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CountTrailingZeros (32)")
  {
    PLASMA_TEST_INT(plMath::CountTrailingZeros(0b1111u), 0);
    PLASMA_TEST_INT(plMath::CountTrailingZeros(0b1110u), 1);
    PLASMA_TEST_INT(plMath::CountTrailingZeros(0b1100u), 2);
    PLASMA_TEST_INT(plMath::CountTrailingZeros(0b1000u), 3);
    PLASMA_TEST_INT(plMath::CountTrailingZeros(0xFFFFFFFF), 0);
    PLASMA_TEST_INT(plMath::CountTrailingZeros(0u), 32);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CountTrailingZeros (64)")
  {
    PLASMA_TEST_INT(plMath::CountTrailingZeros(0b1111llu), 0);
    PLASMA_TEST_INT(plMath::CountTrailingZeros(0b1110llu), 1);
    PLASMA_TEST_INT(plMath::CountTrailingZeros(0b1100llu), 2);
    PLASMA_TEST_INT(plMath::CountTrailingZeros(0b1000llu), 3);
    PLASMA_TEST_INT(plMath::CountTrailingZeros(0xFFFFFFFF0llu), 4);
    PLASMA_TEST_INT(plMath::CountTrailingZeros(0llu), 64);
    PLASMA_TEST_INT(plMath::CountTrailingZeros(0xFFFFFFFF00llu), 8);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CountLeadingZeros")
  {
    PLASMA_TEST_INT(plMath::CountLeadingZeros(0b1111), 28);
    PLASMA_TEST_INT(plMath::CountLeadingZeros(0b0111), 29);
    PLASMA_TEST_INT(plMath::CountLeadingZeros(0b0011), 30);
    PLASMA_TEST_INT(plMath::CountLeadingZeros(0b0001), 31);
    PLASMA_TEST_INT(plMath::CountLeadingZeros(0xFFFFFFFF), 0);
    PLASMA_TEST_INT(plMath::CountLeadingZeros(0), 32);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TryMultiply32")
  {
    plUInt32 res;

    res = 0;
    PLASMA_TEST_BOOL(plMath::TryMultiply32(res, 1, 1, 2, 3).Succeeded());
    PLASMA_TEST_INT(res, 6);

    res = 0;
    PLASMA_TEST_BOOL(plMath::TryMultiply32(res, 1, 1, 1, 0xFFFFFFFF).Succeeded());
    PLASMA_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    PLASMA_TEST_BOOL(plMath::TryMultiply32(res, 0xFFFF, 0x10001).Succeeded());
    PLASMA_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    PLASMA_TEST_BOOL(plMath::TryMultiply32(res, 0x3FFFFFF, 2, 4, 8).Succeeded());
    PLASMA_TEST_BOOL(res == 0xFFFFFFC0);

    res = 1;
    PLASMA_TEST_BOOL(plMath::TryMultiply32(res, 0xFFFFFFFF, 2).Failed());
    PLASMA_TEST_BOOL(res == 1);

    res = 0;
    PLASMA_TEST_BOOL(plMath::TryMultiply32(res, 0x80000000, 2).Failed()); // slightly above 0xFFFFFFFF
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TryMultiply64")
  {
    plUInt64 res;

    res = 0;
    PLASMA_TEST_BOOL(plMath::TryMultiply64(res, 1, 1, 2, 3).Succeeded());
    PLASMA_TEST_INT(res, 6);

    res = 0;
    PLASMA_TEST_BOOL(plMath::TryMultiply64(res, 1, 1, 1, 0xFFFFFFFF).Succeeded());
    PLASMA_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    PLASMA_TEST_BOOL(plMath::TryMultiply64(res, 0xFFFF, 0x10001).Succeeded());
    PLASMA_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    PLASMA_TEST_BOOL(plMath::TryMultiply64(res, 0x3FFFFFF, 2, 4, 8).Succeeded());
    PLASMA_TEST_BOOL(res == 0xFFFFFFC0);

    res = 0;
    PLASMA_TEST_BOOL(plMath::TryMultiply64(res, 0xFFFFFFFF, 2).Succeeded());
    PLASMA_TEST_BOOL(res == 0x1FFFFFFFE);

    res = 0;
    PLASMA_TEST_BOOL(plMath::TryMultiply64(res, 0x80000000, 2).Succeeded());
    PLASMA_TEST_BOOL(res == 0x100000000);

    res = 0;
    PLASMA_TEST_BOOL(plMath::TryMultiply64(res, 0xFFFFFFFF, 0xFFFFFFFF).Succeeded());
    PLASMA_TEST_BOOL(res == 0xFFFFFFFE00000001);

    res = 0;
    PLASMA_TEST_BOOL(plMath::TryMultiply64(res, 0xFFFFFFFFFFFFFFFF, 2).Failed());

    res = 0;
    PLASMA_TEST_BOOL(plMath::TryMultiply64(res, 0xFFFFFFFF, 0xFFFFFFFF, 2).Failed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TryConvertToSizeT")
  {
    plUInt64 x = plMath::MaxValue<plUInt32>();
    plUInt64 y = x + 1;

    size_t res = 0;

    PLASMA_TEST_BOOL(plMath::TryConvertToSizeT(res, x).Succeeded());
    PLASMA_TEST_BOOL(res == x);

    res = 0;
#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
    PLASMA_TEST_BOOL(plMath::TryConvertToSizeT(res, y).Failed());
#else
    PLASMA_TEST_BOOL(plMath::TryConvertToSizeT(res, y).Succeeded());
    PLASMA_TEST_BOOL(res == y);
#endif
  }
}

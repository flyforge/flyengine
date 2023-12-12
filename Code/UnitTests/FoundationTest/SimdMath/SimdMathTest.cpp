#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdMath.h>

namespace
{
  plSimdVec4f SimdDegree(float degree)
  {
    return plSimdVec4f(plAngle::Degree(degree));
  }
} // namespace

PLASMA_CREATE_SIMPLE_TEST(SimdMath, SimdMath)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Exp")
  {
    float testVals[] = {0.0f, 1.0f, 2.0f};
    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = plMath::Exp(v);
      PLASMA_TEST_BOOL(plSimdMath::Exp(plSimdVec4f(v)).IsEqual(plSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Ln")
  {
    float testVals[] = {1.0f, 2.7182818284f, 7.3890560989f};
    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = plMath::Ln(v);
      PLASMA_TEST_BOOL(plSimdMath::Ln(plSimdVec4f(v)).IsEqual(plSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Log2")
  {
    float testVals[] = {1.0f, 2.0f, 4.0f};
    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = plMath::Log2(v);
      PLASMA_TEST_BOOL(plSimdMath::Log2(plSimdVec4f(v)).IsEqual(plSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Log2i")
  {
    int testVals[] = {0, 1, 2, 3, 4, 6, 7, 8};
    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(testVals); ++i)
    {
      const int v = testVals[i];
      const int r = plMath::Log2i(v);
      PLASMA_TEST_BOOL((plSimdMath::Log2i(plSimdVec4i(v)) == plSimdVec4i(r)).AllSet());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Log10")
  {
    float testVals[] = {1.0f, 10.0f, 100.0f};
    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = plMath::Log10(v);
      PLASMA_TEST_BOOL(plSimdMath::Log10(plSimdVec4f(v)).IsEqual(plSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Pow2")
  {
    float testVals[] = {0.0f, 1.0f, 2.0f};
    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = plMath::Pow2(v);
      PLASMA_TEST_BOOL(plSimdMath::Pow2(plSimdVec4f(v)).IsEqual(plSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Sin")
  {
    PLASMA_TEST_BOOL(plSimdMath::Sin(SimdDegree(0.0f)).IsEqual(plSimdVec4f(0.0f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::Sin(SimdDegree(90.0f)).IsEqual(plSimdVec4f(1.0f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::Sin(SimdDegree(180.0f)).IsEqual(plSimdVec4f(0.0f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::Sin(SimdDegree(270.0f)).IsEqual(plSimdVec4f(-1.0f), 0.000001f).AllSet());

    PLASMA_TEST_BOOL(plSimdMath::Sin(SimdDegree(45.0f)).IsEqual(plSimdVec4f(0.7071067f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::Sin(SimdDegree(135.0f)).IsEqual(plSimdVec4f(0.7071067f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::Sin(SimdDegree(225.0f)).IsEqual(plSimdVec4f(-0.7071067f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::Sin(SimdDegree(315.0f)).IsEqual(plSimdVec4f(-0.7071067f), 0.000001f).AllSet());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Cos")
  {
    PLASMA_TEST_BOOL(plSimdMath::Cos(SimdDegree(0.0f)).IsEqual(plSimdVec4f(1.0f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::Cos(SimdDegree(90.0f)).IsEqual(plSimdVec4f(0.0f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::Cos(SimdDegree(180.0f)).IsEqual(plSimdVec4f(-1.0f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::Cos(SimdDegree(270.0f)).IsEqual(plSimdVec4f(0.0f), 0.000001f).AllSet());

    PLASMA_TEST_BOOL(plSimdMath::Cos(SimdDegree(45.0f)).IsEqual(plSimdVec4f(0.7071067f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::Cos(SimdDegree(135.0f)).IsEqual(plSimdVec4f(-0.7071067f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::Cos(SimdDegree(225.0f)).IsEqual(plSimdVec4f(-0.7071067f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::Cos(SimdDegree(315.0f)).IsEqual(plSimdVec4f(0.7071067f), 0.000001f).AllSet());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Tan")
  {
    PLASMA_TEST_BOOL(plSimdMath::Tan(SimdDegree(0.0f)).IsEqual(plSimdVec4f(0.0f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::Tan(SimdDegree(45.0f)).IsEqual(plSimdVec4f(1.0f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::Tan(SimdDegree(-45.0f)).IsEqual(plSimdVec4f(-1.0f), 0.000001f).AllSet());
    PLASMA_TEST_BOOL((plSimdMath::Tan(SimdDegree(90.00001f)) < plSimdVec4f(1000000.0f)).AllSet());
    PLASMA_TEST_BOOL((plSimdMath::Tan(SimdDegree(89.9999f)) > plSimdVec4f(100000.0f)).AllSet());

    // Testing the period of tan(x) centered at 0 and the adjacent ones
    plAngle angle = plAngle::Degree(-89.0f);
    while (angle.GetDegree() < 89.0f)
    {
      plSimdVec4f simdAngle(angle.GetRadian());

      plSimdVec4f fTan = plSimdMath::Tan(simdAngle);
      plSimdVec4f fTanPrev = plSimdMath::Tan(SimdDegree(angle.GetDegree() - 180.0f));
      plSimdVec4f fTanNext = plSimdMath::Tan(SimdDegree(angle.GetDegree() + 180.0f));
      plSimdVec4f fSin = plSimdMath::Sin(simdAngle);
      plSimdVec4f fCos = plSimdMath::Cos(simdAngle);

      PLASMA_TEST_BOOL((fTan - fTanPrev).IsEqual(plSimdVec4f::ZeroVector(), 0.002f).AllSet());
      PLASMA_TEST_BOOL((fTan - fTanNext).IsEqual(plSimdVec4f::ZeroVector(), 0.002f).AllSet());
      PLASMA_TEST_BOOL((fTan - fSin.CompDiv(fCos)).IsEqual(plSimdVec4f::ZeroVector(), 0.0005f).AllSet());
      angle += plAngle::Degree(1.234f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ASin")
  {
    PLASMA_TEST_BOOL(plSimdMath::ASin(plSimdVec4f(0.0f)).IsEqual(SimdDegree(0.0f), 0.0001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::ASin(plSimdVec4f(1.0f)).IsEqual(SimdDegree(90.0f), 0.00001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::ASin(plSimdVec4f(-1.0f)).IsEqual(SimdDegree(-90.0f), 0.00001f).AllSet());

    PLASMA_TEST_BOOL(plSimdMath::ASin(plSimdVec4f(0.7071067f)).IsEqual(SimdDegree(45.0f), 0.0001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::ASin(plSimdVec4f(-0.7071067f)).IsEqual(SimdDegree(-45.0f), 0.0001f).AllSet());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ACos")
  {
    PLASMA_TEST_BOOL(plSimdMath::ACos(plSimdVec4f(0.0f)).IsEqual(SimdDegree(90.0f), 0.0001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::ACos(plSimdVec4f(1.0f)).IsEqual(SimdDegree(0.0f), 0.00001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::ACos(plSimdVec4f(-1.0f)).IsEqual(SimdDegree(180.0f), 0.0001f).AllSet());

    PLASMA_TEST_BOOL(plSimdMath::ACos(plSimdVec4f(0.7071067f)).IsEqual(SimdDegree(45.0f), 0.0001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::ACos(plSimdVec4f(-0.7071067f)).IsEqual(SimdDegree(135.0f), 0.0001f).AllSet());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ATan")
  {
    PLASMA_TEST_BOOL(plSimdMath::ATan(plSimdVec4f(0.0f)).IsEqual(SimdDegree(0.0f), 0.0000001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::ATan(plSimdVec4f(1.0f)).IsEqual(SimdDegree(45.0f), 0.00001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::ATan(plSimdVec4f(-1.0f)).IsEqual(SimdDegree(-45.0f), 0.00001f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::ATan(plSimdVec4f(10000000.0f)).IsEqual(SimdDegree(90.0f), 0.00002f).AllSet());
    PLASMA_TEST_BOOL(plSimdMath::ATan(plSimdVec4f(-10000000.0f)).IsEqual(SimdDegree(-90.0f), 0.00002f).AllSet());
  }
}
#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Angle.h>

PLASMA_CREATE_SIMPLE_TEST(Math, Angle)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "DegToRad")
  {
    PLASMA_TEST_FLOAT(plAngle::DegToRad(0.0f), 0.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::DegToRad(45.0f), 0.785398163f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::DegToRad(90.0f), 1.570796327f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::DegToRad(120.0f), 2.094395102f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::DegToRad(170.0f), 2.967059728f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::DegToRad(180.0f), 3.141592654f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::DegToRad(250.0f), 4.36332313f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::DegToRad(320.0f), 5.585053606f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::DegToRad(360.0f), 6.283185307f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::DegToRad(700.0f), 12.217304764f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::DegToRad(-123.0f), -2.14675498f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::DegToRad(-1234.0f), -21.53736297f, 0.00001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RadToDeg")
  {
    PLASMA_TEST_FLOAT(plAngle::RadToDeg(0.0f), 0.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::RadToDeg(0.785398163f), 45.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::RadToDeg(1.570796327f), 90.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::RadToDeg(2.094395102f), 120.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::RadToDeg(2.967059728f), 170.0f, 0.0001f);
    PLASMA_TEST_FLOAT(plAngle::RadToDeg(3.141592654f), 180.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::RadToDeg(4.36332313f), 250.0f, 0.0001f);
    PLASMA_TEST_FLOAT(plAngle::RadToDeg(5.585053606f), 320.0f, 0.0001f);
    PLASMA_TEST_FLOAT(plAngle::RadToDeg(6.283185307f), 360.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::RadToDeg(12.217304764f), 700.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::RadToDeg(-2.14675498f), -123.0f, 0.00001f);
    PLASMA_TEST_FLOAT(plAngle::RadToDeg(-21.53736297f), -1234.0f, 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Init")
  {
    plAngle a0;
    PLASMA_TEST_FLOAT(a0.GetRadian(), 0.0f, 0.0f);
    PLASMA_TEST_FLOAT(a0.GetDegree(), 0.0f, 0.0f);

    plAngle a1 = plAngle::Radian(1.570796327f);
    PLASMA_TEST_FLOAT(a1.GetRadian(), 1.570796327f, 0.00001f);
    PLASMA_TEST_FLOAT(a1.GetDegree(), 90.0f, 0.00001f);

    plAngle a2 = plAngle::Degree(90);
    PLASMA_TEST_FLOAT(a2.GetRadian(), 1.570796327f, 0.00001f);
    PLASMA_TEST_FLOAT(a2.GetDegree(), 90.0f, 0.00001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "NormalizeRange / IsEqual ")
  {
    plAngle a;

    for (plInt32 i = 1; i < 359; i++)
    {
      a = plAngle::Degree((float)i);
      a.NormalizeRange();
      PLASMA_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = plAngle::Degree((float)i);
      a.NormalizeRange();
      PLASMA_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = plAngle::Degree((float)i + 360.0f);
      a.NormalizeRange();
      PLASMA_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = plAngle::Degree((float)i - 360.0f);
      a.NormalizeRange();
      PLASMA_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = plAngle::Degree((float)i + 3600.0f);
      a.NormalizeRange();
      PLASMA_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = plAngle::Degree((float)i - 3600.0f);
      a.NormalizeRange();
      PLASMA_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = plAngle::Degree((float)i + 36000.0f);
      a.NormalizeRange();
      PLASMA_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = plAngle::Degree((float)i - 36000.0f);
      a.NormalizeRange();
      PLASMA_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
    }

    for (plInt32 i = 0; i < 360; i++)
    {
      a = plAngle::Degree((float)i);
      PLASMA_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
      a = plAngle::Degree((float)i);
      PLASMA_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
      a = plAngle::Degree((float)i + 360.0f);
      PLASMA_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
      a = plAngle::Degree((float)i - 360.0f);
      PLASMA_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
      a = plAngle::Degree((float)i + 3600.0f);
      PLASMA_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
      a = plAngle::Degree((float)i - 3600.0f);
      PLASMA_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
      a = plAngle::Degree((float)i + 36000.0f);
      PLASMA_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
      a = plAngle::Degree((float)i - 36000.0f);
      PLASMA_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
    }

    for (plInt32 i = 0; i < 360; i++)
    {
      a = plAngle::Degree((float)i);
      PLASMA_TEST_BOOL(a.IsEqualNormalized(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
      a = plAngle::Degree((float)i);
      PLASMA_TEST_BOOL(a.IsEqualNormalized(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
      a = plAngle::Degree((float)i + 360.0f);
      PLASMA_TEST_BOOL(a.IsEqualNormalized(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
      a = plAngle::Degree((float)i - 360.0f);
      PLASMA_TEST_BOOL(a.IsEqualNormalized(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
      a = plAngle::Degree((float)i + 3600.0f);
      PLASMA_TEST_BOOL(a.IsEqualNormalized(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
      a = plAngle::Degree((float)i - 3600.0f);
      PLASMA_TEST_BOOL(a.IsEqualNormalized(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
      a = plAngle::Degree((float)i + 36000.0f);
      PLASMA_TEST_BOOL(a.IsEqualNormalized(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
      a = plAngle::Degree((float)i - 36000.0f);
      PLASMA_TEST_BOOL(a.IsEqualNormalized(plAngle::Degree((float)i), plAngle::Degree(0.01f)));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AngleBetween")
  {
    PLASMA_TEST_FLOAT(plAngle::AngleBetween(plAngle::Degree(0), plAngle::Degree(0)).GetDegree(), 0.0f, 0.0001f);
    PLASMA_TEST_FLOAT(plAngle::AngleBetween(plAngle::Degree(0), plAngle::Degree(360)).GetDegree(), 0.0f, 0.0001f);
    PLASMA_TEST_FLOAT(plAngle::AngleBetween(plAngle::Degree(360), plAngle::Degree(360)).GetDegree(), 0.0f, 0.0001f);
    PLASMA_TEST_FLOAT(plAngle::AngleBetween(plAngle::Degree(360), plAngle::Degree(0)).GetDegree(), 0.0f, 0.0001f);

    PLASMA_TEST_FLOAT(plAngle::AngleBetween(plAngle::Degree(5), plAngle::Degree(186)).GetDegree(), 179.0f, 0.0001f);
    PLASMA_TEST_FLOAT(plAngle::AngleBetween(plAngle::Degree(-5), plAngle::Degree(-186)).GetDegree(), 179.0f, 0.0001f);

    PLASMA_TEST_FLOAT(plAngle::AngleBetween(plAngle::Degree(360.0f + 5), plAngle::Degree(360.0f + 186)).GetDegree(), 179.0f, 0.0001f);
    PLASMA_TEST_FLOAT(plAngle::AngleBetween(plAngle::Degree(360.0f + -5), plAngle::Degree(360.0f - 186)).GetDegree(), 179.0f, 0.0001f);

    for (plInt32 i = 0; i <= 179; ++i)
      PLASMA_TEST_FLOAT(plAngle::AngleBetween(plAngle::Degree((float)i), plAngle::Degree((float)(i + i))).GetDegree(), (float)i, 0.0001f);

    for (plInt32 i = -179; i <= 0; ++i)
      PLASMA_TEST_FLOAT(plAngle::AngleBetween(plAngle::Degree((float)i), plAngle::Degree((float)(i + i))).GetDegree(), (float)-i, 0.0001f);
  }
}

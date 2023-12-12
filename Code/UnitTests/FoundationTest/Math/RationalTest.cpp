#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Rational.h>
#include <Foundation/Strings/StringBuilder.h>

PLASMA_CREATE_SIMPLE_TEST(Math, Rational)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Rational")
  {
    plRational r1(100, 1);

    PLASMA_TEST_BOOL(r1.IsValid());
    PLASMA_TEST_BOOL(r1.IsIntegral());

    plRational r2(100, 0);
    PLASMA_TEST_BOOL(!r2.IsValid());

    PLASMA_TEST_BOOL(r1 != r2);

    plRational r3(100, 1);
    PLASMA_TEST_BOOL(r3 == r1);

    plRational r4(0, 0);
    PLASMA_TEST_BOOL(r4.IsValid());


    plRational r5(30, 6);
    PLASMA_TEST_BOOL(r5.IsIntegral());
    PLASMA_TEST_INT(r5.GetIntegralResult(), 5);
    PLASMA_TEST_FLOAT(r5.GetFloatingPointResult(), 5, plMath::SmallEpsilon<double>());

    plRational reducedTest(5, 1);
    PLASMA_TEST_BOOL(r5.ReduceIntegralFraction() == reducedTest);

    plRational r6(31, 6);
    PLASMA_TEST_BOOL(!r6.IsIntegral());
    PLASMA_TEST_FLOAT(r6.GetFloatingPointResult(), 5.16666666666, plMath::SmallEpsilon<double>());


    PLASMA_TEST_INT(r6.GetDenominator(), 6);
    PLASMA_TEST_INT(r6.GetNumerator(), 31);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Rational String Formatting")
  {
    plRational r1(50, 25);

    plStringBuilder sb;
    sb.Format("Rational: {}", r1);
    PLASMA_TEST_STRING(sb, "Rational: 2");


    plRational r2(233, 76);
    sb.Format("Rational: {}", r2);
    PLASMA_TEST_STRING(sb, "Rational: 233/76");
  }
}

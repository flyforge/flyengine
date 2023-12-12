#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/MathExpression.h>

PLASMA_CREATE_SIMPLE_TEST(CodeUtils, MathExpression)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Basics")
  {
    {
      plMathExpression expr("");
      PLASMA_TEST_BOOL(!expr.IsValid());

      expr.Reset("");
      PLASMA_TEST_BOOL(!expr.IsValid());
    }
    {
      plMathExpression expr(nullptr);
      PLASMA_TEST_BOOL(!expr.IsValid());

      expr.Reset(nullptr);
      PLASMA_TEST_BOOL(!expr.IsValid());
    }
    {
      plMathExpression expr("1.5 + 2.5");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 4.0, 0.0);
    }
    {
      plMathExpression expr("1- 2");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), -1.0, 0.0);
    }
    {
      plMathExpression expr("1 *2");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
    {
      plMathExpression expr(" 1.0/2 ");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 0.5, 0.0);
    }
    {
      plMathExpression expr("1 - -1");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
    {
      plMathExpression expr("abs(-3)");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
    }
    {
      plMathExpression expr("sqrt(4)");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
    {
      plMathExpression expr("saturate(4)");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 1.0, 0.0);
    }
    {
      plMathExpression expr("min(3, 4)");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
    }
    {
      plMathExpression expr("max(3, 4)");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 4.0, 0.0);
    }
    {
      plMathExpression expr("clamp(2, 3, 4)");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
    }
    {
      plMathExpression expr("clamp(5, 3, 4)");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 4.0, 0.0);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Operator Priority")
  {
    {
      plMathExpression expr("1 - 2 * 4");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), -7.0, 0.0);
    }
    {
      plMathExpression expr("-1 - 2 * 4");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), -9.0, 0.0);
    }
    {
      plMathExpression expr("1 - 2.0 / 4");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 0.5, 0.0);
    }
    {
      plMathExpression expr("abs (-4 + 2)");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Braces")
  {
    {
      plMathExpression expr("(1 - 2) * 4");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), -4.0, 0.0);
    }
    {
      plMathExpression expr("(((((0)))))");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 0.0, 0.0);
    }
    {
      plMathExpression expr("(1 + 2) * (3 - 2)");
      PLASMA_TEST_BOOL(expr.IsValid());
      PLASMA_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Variables")
  {
    plHybridArray<plMathExpression::Input, 4> inputs;
    inputs.SetCount(4);

    {
      plMathExpression expr("_var1 + v2Ar");
      PLASMA_TEST_BOOL(expr.IsValid());

      inputs[0] = {plMakeHashedString("_var1"), 1.0};
      inputs[1] = {plMakeHashedString("v2Ar"), 2.0};

      double result = expr.Evaluate(inputs);
      PLASMA_TEST_DOUBLE(result, 3.0, 0.0);

      inputs[0].m_fValue = 2.0;
      inputs[1].m_fValue = 0.5;

      result = expr.Evaluate(inputs);
      PLASMA_TEST_DOUBLE(result, 2.5, 0.0);
    }

    // Make sure we got the spaces right and don't count it as part of the variable.
    {
      plMathExpression expr("  a +  b /c*d");
      PLASMA_TEST_BOOL(expr.IsValid());

      inputs[0] = {plMakeHashedString("a"), 1.0};
      inputs[1] = {plMakeHashedString("b"), 4.0};
      inputs[2] = {plMakeHashedString("c"), 2.0};
      inputs[3] = {plMakeHashedString("d"), 3.0};

      double result = expr.Evaluate(inputs);
      PLASMA_TEST_DOUBLE(result, 7.0, 0.0);
    }
  }


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Invalid Expressions")
  {
    plMuteLog logErrorSink;
    plLogSystemScope ls(&logErrorSink);

    {
      plMathExpression expr("1+");
      PLASMA_TEST_BOOL(!expr.IsValid());
    }
    {
      plMathExpression expr("1+/1");
      PLASMA_TEST_BOOL(!expr.IsValid());
    }
    {
      plMathExpression expr("(((((0))))");
      PLASMA_TEST_BOOL(!expr.IsValid());
    }
    {
      plMathExpression expr("_va£r + asdf");
      PLASMA_TEST_BOOL(!expr.IsValid());
    }
    {
      plMathExpression expr("sqrt(2, 4)");
      PLASMA_TEST_BOOL(!expr.IsValid());
    }
  }
}

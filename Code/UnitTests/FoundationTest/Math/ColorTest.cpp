#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat4.h>

PLASMA_CREATE_SIMPLE_TEST(Math, Color)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor empty")
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    if (plMath::SupportsNaN<plMathTestType>())
    {
      // In debug the default constructor initializes everything with NaN.
      plColor defCtor;
      PLASMA_TEST_BOOL(plMath::IsNaN(defCtor.r) && plMath::IsNaN(defCtor.g) && plMath::IsNaN(defCtor.b) && plMath::IsNaN(defCtor.a));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float testBlock[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    plColor* pDefCtor = ::new ((void*)&testBlock[0]) plColor;
    PLASMA_TEST_BOOL(pDefCtor->r == 1.0f && pDefCtor->g == 2.0f && pDefCtor->b == 3.0f && pDefCtor->a == 4.0f);
#endif

    // Make sure the class didn't accidentally change in size
    PLASMA_TEST_BOOL(sizeof(plColor) == sizeof(float) * 4);
  }
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor components")
  {
    plColor init3F(0.5f, 0.6f, 0.7f);
    PLASMA_TEST_BOOL(init3F.r == 0.5f && init3F.g == 0.6f && init3F.b == 0.7f && init3F.a == 1.0f);

    plColor init4F(0.5f, 0.6f, 0.7f, 0.8f);
    PLASMA_TEST_BOOL(init4F.r == 0.5f && init4F.g == 0.6f && init4F.b == 0.7f && init4F.a == 0.8f);
  }
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor copy")
  {
    plColor init4F(0.5f, 0.6f, 0.7f, 0.8f);
    plColor copy(init4F);
    PLASMA_TEST_BOOL(copy.r == 0.5f && copy.g == 0.6f && copy.b == 0.7f && copy.a == 0.8f);
  }

  {
    plColor cornflowerBlue(plColor(0.39f, 0.58f, 0.93f));

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Conversion float")
    {
      float* pFloats = cornflowerBlue.GetData();
      PLASMA_TEST_BOOL(
        pFloats[0] == cornflowerBlue.r && pFloats[1] == cornflowerBlue.g && pFloats[2] == cornflowerBlue.b && pFloats[3] == cornflowerBlue.a);

      const float* pConstFloats = cornflowerBlue.GetData();
      PLASMA_TEST_BOOL(pConstFloats[0] == cornflowerBlue.r && pConstFloats[1] == cornflowerBlue.g && pConstFloats[2] == cornflowerBlue.b &&
                   pConstFloats[3] == cornflowerBlue.a);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "HSV conversion")
  {
    plColor normalizedColor(0.0f, 1.0f, 0.999f, 0.0001f);
    PLASMA_TEST_BOOL(normalizedColor.IsNormalized());
    plColor notNormalizedColor0(-0.01f, 1.0f, 0.999f, 0.0001f);
    PLASMA_TEST_BOOL(!notNormalizedColor0.IsNormalized());
    plColor notNormalizedColor1(0.5f, 1.1f, 0.9f, 0.1f);
    PLASMA_TEST_BOOL(!notNormalizedColor1.IsNormalized());
    plColor notNormalizedColor2(0.1f, 1.0f, 1.999f, 0.1f);
    PLASMA_TEST_BOOL(!notNormalizedColor2.IsNormalized());
    plColor notNormalizedColor3(0.1f, 1.0f, 1.0f, -0.1f);
    PLASMA_TEST_BOOL(!notNormalizedColor3.IsNormalized());


    // hsv test - took some samples from http://www.javascripter.net/faq/rgb2hsv.htm
    const plColorGammaUB rgb[] = {plColorGammaUB(255, 255, 255), plColorGammaUB(0, 0, 0), plColorGammaUB(123, 12, 1), plColorGammaUB(31, 112, 153)};
    const plVec3 hsv[] = {plVec3(0, 0, 1), plVec3(0, 0, 0), plVec3(5.4f, 0.991f, 0.48f), plVec3(200.2f, 0.797f, 0.600f)};

    for (int i = 0; i < 4; ++i)
    {
      const plColor color = rgb[i];
      float hue, sat, val;
      color.GetHSV(hue, sat, val);

      PLASMA_TEST_FLOAT(hue, hsv[i].x, 0.1f);
      PLASMA_TEST_FLOAT(sat, hsv[i].y, 0.1f);
      PLASMA_TEST_FLOAT(val, hsv[i].z, 0.1f);

      plColor fromHSV;
      fromHSV.SetHSV(hsv[i].x, hsv[i].y, hsv[i].z);
      PLASMA_TEST_FLOAT(fromHSV.r, color.r, 0.01f);
      PLASMA_TEST_FLOAT(fromHSV.g, color.g, 0.01f);
      PLASMA_TEST_FLOAT(fromHSV.b, color.b, 0.01f);
    }
  }

  {
    if (plMath::SupportsNaN<plMathTestType>())
    {
      float fNaN = plMath::NaN<float>();
      const plColor nanArray[4] = {
        plColor(fNaN, 0.0f, 0.0f, 0.0f), plColor(0.0f, fNaN, 0.0f, 0.0f), plColor(0.0f, 0.0f, fNaN, 0.0f), plColor(0.0f, 0.0f, 0.0f, fNaN)};
      const plColor compArray[4] = {
        plColor(1.0f, 0.0f, 0.0f, 0.0f), plColor(0.0f, 1.0f, 0.0f, 0.0f), plColor(0.0f, 0.0f, 1.0f, 0.0f), plColor(0.0f, 0.0f, 0.0f, 1.0f)};


      PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNaN")
      {
        for (int i = 0; i < 4; ++i)
        {
          PLASMA_TEST_BOOL(nanArray[i].IsNaN());
          PLASMA_TEST_BOOL(!compArray[i].IsNaN());
        }
      }

      PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsValid")
      {
        for (int i = 0; i < 4; ++i)
        {
          PLASMA_TEST_BOOL(!nanArray[i].IsValid());
          PLASMA_TEST_BOOL(compArray[i].IsValid());

          PLASMA_TEST_BOOL(!(compArray[i] * plMath::Infinity<float>()).IsValid());
          PLASMA_TEST_BOOL(!(compArray[i] * -plMath::Infinity<float>()).IsValid());
        }
      }
    }
  }

  {
    const plColor op1(-4.0, 0.2f, -7.0f, -0.0f);
    const plColor op2(2.0, 0.3f, 0.0f, 1.0f);
    const plColor compArray[4] = {
      plColor(1.0f, 0.0f, 0.0f, 0.0f), plColor(0.0f, 1.0f, 0.0f, 0.0f), plColor(0.0f, 0.0f, 1.0f, 0.0f), plColor(0.0f, 0.0f, 0.0f, 1.0f)};

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetRGB / SetRGBA")
    {
      plColor c1(0, 0, 0, 0);

      c1.SetRGBA(1, 2, 3, 4);

      PLASMA_TEST_BOOL(c1 == plColor(1, 2, 3, 4));

      c1.SetRGB(5, 6, 7);

      PLASMA_TEST_BOOL(c1 == plColor(5, 6, 7, 4));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsIdenticalRGB")
    {
      plColor c1(0, 0, 0, 0);
      plColor c2(0, 0, 0, 1);

      PLASMA_TEST_BOOL(c1.IsIdenticalRGB(c2));
      PLASMA_TEST_BOOL(!c1.IsIdenticalRGBA(c2));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsIdenticalRGBA")
    {
      PLASMA_TEST_BOOL(op1.IsIdenticalRGBA(op1));
      for (int i = 0; i < 4; ++i)
      {
        PLASMA_TEST_BOOL(!op1.IsIdenticalRGBA(op1 + plMath::SmallEpsilon<float>() * compArray[i]));
        PLASMA_TEST_BOOL(!op1.IsIdenticalRGBA(op1 - plMath::SmallEpsilon<float>() * compArray[i]));
      }
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqualRGB")
    {
      plColor c1(0, 0, 0, 0);
      plColor c2(0, 0, 0.2f, 1);

      PLASMA_TEST_BOOL(!c1.IsEqualRGB(c2, 0.1f));
      PLASMA_TEST_BOOL(c1.IsEqualRGB(c2, 0.3f));
      PLASMA_TEST_BOOL(!c1.IsEqualRGBA(c2, 0.3f));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqualRGBA")
    {
      PLASMA_TEST_BOOL(op1.IsEqualRGBA(op1, 0.0f));
      for (int i = 0; i < 4; ++i)
      {
        PLASMA_TEST_BOOL(op1.IsEqualRGBA(op1 + plMath::SmallEpsilon<float>() * compArray[i], 2 * plMath::SmallEpsilon<float>()));
        PLASMA_TEST_BOOL(op1.IsEqualRGBA(op1 - plMath::SmallEpsilon<float>() * compArray[i], 2 * plMath::SmallEpsilon<float>()));
        PLASMA_TEST_BOOL(op1.IsEqualRGBA(op1 + plMath::DefaultEpsilon<float>() * compArray[i], 2 * plMath::DefaultEpsilon<float>()));
        PLASMA_TEST_BOOL(op1.IsEqualRGBA(op1 - plMath::DefaultEpsilon<float>() * compArray[i], 2 * plMath::DefaultEpsilon<float>()));
      }
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator+= (plColor)")
    {
      plColor plusAssign = op1;
      plusAssign += op2;
      PLASMA_TEST_BOOL(plusAssign.IsEqualRGBA(plColor(-2.0f, 0.5f, -7.0f, 1.0f), plMath::SmallEpsilon<float>()));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator-= (plColor)")
    {
      plColor minusAssign = op1;
      minusAssign -= op2;
      PLASMA_TEST_BOOL(minusAssign.IsEqualRGBA(plColor(-6.0f, -0.1f, -7.0f, -1.0f), plMath::SmallEpsilon<float>()));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ooperator*= (float)")
    {
      plColor mulFloat = op1;
      mulFloat *= 2.0f;
      PLASMA_TEST_BOOL(mulFloat.IsEqualRGBA(plColor(-8.0f, 0.4f, -14.0f, -0.0f), plMath::SmallEpsilon<float>()));
      mulFloat *= 0.0f;
      PLASMA_TEST_BOOL(mulFloat.IsEqualRGBA(plColor(0.0f, 0.0f, 0.0f, 0.0f), plMath::SmallEpsilon<float>()));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator/= (float)")
    {
      plColor vDivFloat = op1;
      vDivFloat /= 2.0f;
      PLASMA_TEST_BOOL(vDivFloat.IsEqualRGBA(plColor(-2.0f, 0.1f, -3.5f, -0.0f), plMath::SmallEpsilon<float>()));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator+ (plColor, plColor)")
    {
      plColor plus = (op1 + op2);
      PLASMA_TEST_BOOL(plus.IsEqualRGBA(plColor(-2.0f, 0.5f, -7.0f, 1.0f), plMath::SmallEpsilon<float>()));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator- (plColor, plColor)")
    {
      plColor minus = (op1 - op2);
      PLASMA_TEST_BOOL(minus.IsEqualRGBA(plColor(-6.0f, -0.1f, -7.0f, -1.0f), plMath::SmallEpsilon<float>()));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator* (float, plColor)")
    {
      plColor mulFloatVec4 = 2 * op1;
      PLASMA_TEST_BOOL(mulFloatVec4.IsEqualRGBA(plColor(-8.0f, 0.4f, -14.0f, -0.0f), plMath::SmallEpsilon<float>()));
      mulFloatVec4 = ((float)0 * op1);
      PLASMA_TEST_BOOL(mulFloatVec4.IsEqualRGBA(plColor(0.0f, 0.0f, 0.0f, 0.0f), plMath::SmallEpsilon<float>()));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator* (plColor, float)")
    {
      plColor mulVec4Float = op1 * 2;
      PLASMA_TEST_BOOL(mulVec4Float.IsEqualRGBA(plColor(-8.0f, 0.4f, -14.0f, -0.0f), plMath::SmallEpsilon<float>()));
      mulVec4Float = (op1 * (float)0);
      PLASMA_TEST_BOOL(mulVec4Float.IsEqualRGBA(plColor(0.0f, 0.0f, 0.0f, 0.0f), plMath::SmallEpsilon<float>()));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator/ (plColor, float)")
    {
      plColor vDivVec4Float = op1 / 2;
      PLASMA_TEST_BOOL(vDivVec4Float.IsEqualRGBA(plColor(-2.0f, 0.1f, -3.5f, -0.0f), plMath::SmallEpsilon<float>()));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator== (plColor, plColor)")
    {
      PLASMA_TEST_BOOL(op1 == op1);
      for (int i = 0; i < 4; ++i)
      {
        PLASMA_TEST_BOOL(!(op1 == (op1 + plMath::SmallEpsilon<float>() * compArray[i])));
        PLASMA_TEST_BOOL(!(op1 == (op1 - plMath::SmallEpsilon<float>() * compArray[i])));
      }
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator< (plColor, plColor)")
    {
      for (int i = 0; i < 4; ++i)
      {
        for (int j = 0; j < 4; ++j)
        {
          if (i == j)
          {
            PLASMA_TEST_BOOL(!(compArray[i] < compArray[j]));
            PLASMA_TEST_BOOL(!(compArray[j] < compArray[i]));
          }
          else if (i < j)
          {
            PLASMA_TEST_BOOL(!(compArray[i] < compArray[j]));
            PLASMA_TEST_BOOL(compArray[j] < compArray[i]);
          }
          else
          {
            PLASMA_TEST_BOOL(!(compArray[j] < compArray[i]));
            PLASMA_TEST_BOOL(compArray[i] < compArray[j]);
          }
        }
      }
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator!= (plColor, plColor)")
    {
      PLASMA_TEST_BOOL(!(op1 != op1));
      for (int i = 0; i < 4; ++i)
      {
        PLASMA_TEST_BOOL(op1 != (op1 + plMath::SmallEpsilon<float>() * compArray[i]));
        PLASMA_TEST_BOOL(op1 != (op1 - plMath::SmallEpsilon<float>() * compArray[i]));
      }
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator= (plColorLinearUB)")
    {
      plColor c;
      plColorLinearUB lin(50, 100, 150, 255);

      c = lin;

      PLASMA_TEST_FLOAT(c.r, 50 / 255.0f, 0.001f);
      PLASMA_TEST_FLOAT(c.g, 100 / 255.0f, 0.001f);
      PLASMA_TEST_FLOAT(c.b, 150 / 255.0f, 0.001f);
      PLASMA_TEST_FLOAT(c.a, 1.0f, 0.001f);
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator= (plColorGammaUB) / constructor(plColorGammaUB)")
    {
      plColor c;
      plColorGammaUB gamma(50, 100, 150, 255);

      c = gamma;
      plColor c3 = gamma;

      PLASMA_TEST_BOOL(c == c3);

      PLASMA_TEST_FLOAT(c.r, 0.031f, 0.001f);
      PLASMA_TEST_FLOAT(c.g, 0.127f, 0.001f);
      PLASMA_TEST_FLOAT(c.b, 0.304f, 0.001f);
      PLASMA_TEST_FLOAT(c.a, 1.0f, 0.001f);

      plColorGammaUB c2 = c;

      PLASMA_TEST_INT(c2.r, 50);
      PLASMA_TEST_INT(c2.g, 100);
      PLASMA_TEST_INT(c2.b, 150);
      PLASMA_TEST_INT(c2.a, 255);
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetInvertedColor")
    {
      const plColor c1(0.1f, 0.3f, 0.7f, 0.9f);

      plColor c2 = c1.GetInvertedColor();

      PLASMA_TEST_BOOL(c2.IsEqualRGBA(plColor(0.9f, 0.7f, 0.3f, 0.1f), 0.01f));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLuminance")
    {
      PLASMA_TEST_FLOAT(plColor::Black.GetLuminance(), 0.0f, 0.001f);
      PLASMA_TEST_FLOAT(plColor::White.GetLuminance(), 1.0f, 0.001f);

      PLASMA_TEST_FLOAT(plColor(0.5f, 0.5f, 0.5f).GetLuminance(), 0.2126f * 0.5f + 0.7152f * 0.5f + 0.0722f * 0.5f, 0.001f);
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetComplementaryColor")
    {
      // black and white have no complementary colors, or rather, they are their own complementary colors, apparently
      PLASMA_TEST_BOOL(plColor::Black.GetComplementaryColor().IsEqualRGBA(plColor::Black, 0.001f));
      PLASMA_TEST_BOOL(plColor::White.GetComplementaryColor().IsEqualRGBA(plColor::White, 0.001f));

      PLASMA_TEST_BOOL(plColor::Red.GetComplementaryColor().IsEqualRGBA(plColor::Cyan, 0.001f));
      PLASMA_TEST_BOOL(plColor::Lime.GetComplementaryColor().IsEqualRGBA(plColor::Magenta, 0.001f));
      PLASMA_TEST_BOOL(plColor::Blue.GetComplementaryColor().IsEqualRGBA(plColor::Yellow, 0.001f));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetSaturation")
    {
      PLASMA_TEST_FLOAT(plColor::Black.GetSaturation(), 0.0f, 0.001f);
      PLASMA_TEST_FLOAT(plColor::White.GetSaturation(), 0.0f, 0.001f);
      PLASMA_TEST_FLOAT(plColor::Red.GetSaturation(), 1.0f, 0.001f);
      PLASMA_TEST_FLOAT(plColor::Lime.GetSaturation(), 1.0f, 0.001f);
      ;
      PLASMA_TEST_FLOAT(plColor::Blue.GetSaturation(), 1.0f, 0.001f);
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator * / *= (plMat4)")
    {
      plMat4 m;
      m.SetIdentity();
      m.SetScalingMatrix(plVec3(0.5f, 0.75f, 0.25f));
      m.SetTranslationVector(plVec3(0.1f, 0.2f, 0.3f));

      plColor c1 = m * plColor::White;

      PLASMA_TEST_BOOL(c1.IsEqualRGBA(plColor(0.6f, 0.95f, 0.55f, 1.0f), 0.01f));
    }
  }
}

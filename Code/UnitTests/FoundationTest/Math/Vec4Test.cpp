#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>


PLASMA_CREATE_SIMPLE_TEST(Math, Vec4)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    if (plMath::SupportsNaN<plMathTestType>())
    {
      // In debug the default constructor initializes everything with NaN.
      plVec4T vDefCtor;
      PLASMA_TEST_BOOL(plMath::IsNaN(vDefCtor.x) && plMath::IsNaN(vDefCtor.y) /* && plMath::IsNaN(vDefCtor.z) && plMath::IsNaN(vDefCtor.w)*/);
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    plVec4T::ComponentType testBlock[4] = {
      (plVec4T::ComponentType)1, (plVec4T::ComponentType)2, (plVec4T::ComponentType)3, (plVec4T::ComponentType)4};
    plVec4T* pDefCtor = ::new ((void*)&testBlock[0]) plVec4T;
    PLASMA_TEST_BOOL(pDefCtor->x == (plVec4T::ComponentType)1 && pDefCtor->y == (plVec4T::ComponentType)2 && pDefCtor->z == (plVec4T::ComponentType)3 &&
                 pDefCtor->w == (plVec4T::ComponentType)4);
#endif

    // Make sure the class didn't accidentally change in size.
    PLASMA_TEST_BOOL(sizeof(plVec4) == 16);
    PLASMA_TEST_BOOL(sizeof(plVec4d) == 32);

    plVec4T vInit1F(2.0f);
    PLASMA_TEST_BOOL(vInit1F.x == 2.0f && vInit1F.y == 2.0f && vInit1F.z == 2.0f && vInit1F.w == 2.0f);

    plVec4T vInit4F(1.0f, 2.0f, 3.0f, 4.0f);
    PLASMA_TEST_BOOL(vInit4F.x == 1.0f && vInit4F.y == 2.0f && vInit4F.z == 3.0f && vInit4F.w == 4.0f);

    plVec4T vCopy(vInit4F);
    PLASMA_TEST_BOOL(vCopy.x == 1.0f && vCopy.y == 2.0f && vCopy.z == 3.0f && vCopy.w == 4.0f);

    plVec4T vZero = plVec4T::ZeroVector();
    PLASMA_TEST_BOOL(vZero.x == 0.0f && vZero.y == 0.0f && vZero.z == 0.0f && vZero.w == 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Conversion")
  {
    plVec4T vData(1.0f, 2.0f, 3.0f, 4.0f);
    plVec2T vToVec2 = vData.GetAsVec2();
    PLASMA_TEST_BOOL(vToVec2.x == vData.x && vToVec2.y == vData.y);

    plVec3T vToVec3 = vData.GetAsVec3();
    PLASMA_TEST_BOOL(vToVec3.x == vData.x && vToVec3.y == vData.y && vToVec3.z == vData.z);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Setter")
  {
    plVec4T vSet1F;
    vSet1F.Set(2.0f);
    PLASMA_TEST_BOOL(vSet1F.x == 2.0f && vSet1F.y == 2.0f && vSet1F.z == 2.0f && vSet1F.w == 2.0f);

    plVec4T vSet4F;
    vSet4F.Set(1.0f, 2.0f, 3.0f, 4.0f);
    PLASMA_TEST_BOOL(vSet4F.x == 1.0f && vSet4F.y == 2.0f && vSet4F.z == 3.0f && vSet4F.w == 4.0f);

    plVec4T vSetZero;
    vSetZero.SetZero();
    PLASMA_TEST_BOOL(vSetZero.x == 0.0f && vSetZero.y == 0.0f && vSetZero.z == 0.0f && vSetZero.w == 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Length")
  {
    const plVec4T vOp1(-4.0, 4.0f, -2.0f, -0.0f);
    const plVec4T compArray[4] = {
      plVec4T(1.0f, 0.0f, 0.0f, 0.0f), plVec4T(0.0f, 1.0f, 0.0f, 0.0f), plVec4T(0.0f, 0.0f, 1.0f, 0.0f), plVec4T(0.0f, 0.0f, 0.0f, 1.0f)};

    // GetLength
    PLASMA_TEST_FLOAT(vOp1.GetLength(), 6.0f, plMath::SmallEpsilon<plMathTestType>());

    // GetLengthSquared
    PLASMA_TEST_FLOAT(vOp1.GetLengthSquared(), 36.0f, plMath::SmallEpsilon<plMathTestType>());

    // GetLengthAndNormalize
    plVec4T vLengthAndNorm = vOp1;
    plMathTestType fLength = vLengthAndNorm.GetLengthAndNormalize();
    PLASMA_TEST_FLOAT(vLengthAndNorm.GetLength(), 1.0f, plMath::SmallEpsilon<plMathTestType>());
    PLASMA_TEST_FLOAT(fLength, 6.0f, plMath::SmallEpsilon<plMathTestType>());
    PLASMA_TEST_FLOAT(vLengthAndNorm.x * vLengthAndNorm.x + vLengthAndNorm.y * vLengthAndNorm.y + vLengthAndNorm.z * vLengthAndNorm.z +
                    vLengthAndNorm.w * vLengthAndNorm.w,
      1.0f, plMath::SmallEpsilon<plMathTestType>());
    PLASMA_TEST_BOOL(vLengthAndNorm.IsNormalized(plMath::SmallEpsilon<plMathTestType>()));

    // GetNormalized
    plVec4T vGetNorm = vOp1.GetNormalized();
    PLASMA_TEST_FLOAT(vGetNorm.x * vGetNorm.x + vGetNorm.y * vGetNorm.y + vGetNorm.z * vGetNorm.z + vGetNorm.w * vGetNorm.w, 1.0f,
      plMath::SmallEpsilon<plMathTestType>());
    PLASMA_TEST_BOOL(vGetNorm.IsNormalized(plMath::SmallEpsilon<plMathTestType>()));

    // Normalize
    plVec4T vNorm = vOp1;
    vNorm.Normalize();
    PLASMA_TEST_FLOAT(vNorm.x * vNorm.x + vNorm.y * vNorm.y + vNorm.z * vNorm.z + vNorm.w * vNorm.w, 1.0f, plMath::SmallEpsilon<plMathTestType>());
    PLASMA_TEST_BOOL(vNorm.IsNormalized(plMath::SmallEpsilon<plMathTestType>()));

    // NormalizeIfNotZero
    plVec4T vNormCond = vNorm * plMath::DefaultEpsilon<plMathTestType>();
    PLASMA_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, plMath::LargeEpsilon<plMathTestType>()) == PLASMA_FAILURE);
    PLASMA_TEST_BOOL(vNormCond == vOp1);
    vNormCond = vNorm * plMath::DefaultEpsilon<plMathTestType>();
    PLASMA_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, plMath::SmallEpsilon<plMathTestType>()) == PLASMA_SUCCESS);
    PLASMA_TEST_VEC4(vNormCond, vNorm, plMath::DefaultEpsilon<plVec4T::ComponentType>());

    // IsZero
    PLASMA_TEST_BOOL(plVec4T::ZeroVector().IsZero());
    for (int i = 0; i < 4; ++i)
    {
      PLASMA_TEST_BOOL(!compArray[i].IsZero());
    }

    // IsZero(float)
    PLASMA_TEST_BOOL(plVec4T::ZeroVector().IsZero(0.0f));
    for (int i = 0; i < 4; ++i)
    {
      PLASMA_TEST_BOOL(!compArray[i].IsZero(0.0f));
      PLASMA_TEST_BOOL(compArray[i].IsZero(1.0f));
      PLASMA_TEST_BOOL((-compArray[i]).IsZero(1.0f));
    }

    // IsNormalized (already tested above)
    for (int i = 0; i < 4; ++i)
    {
      PLASMA_TEST_BOOL(compArray[i].IsNormalized());
      PLASMA_TEST_BOOL((-compArray[i]).IsNormalized());
      PLASMA_TEST_BOOL((compArray[i] * (plMathTestType)2).IsNormalized((plMathTestType)4));
      PLASMA_TEST_BOOL((compArray[i] * (plMathTestType)2).IsNormalized((plMathTestType)4));
    }

    if (plMath::SupportsNaN<plMathTestType>())
    {
      plMathTestType TypeNaN = plMath::NaN<plMathTestType>();
      const plVec4T nanArray[4] = {plVec4T(TypeNaN, 0.0f, 0.0f, 0.0f), plVec4T(0.0f, TypeNaN, 0.0f, 0.0f), plVec4T(0.0f, 0.0f, TypeNaN, 0.0f),
        plVec4T(0.0f, 0.0f, 0.0f, TypeNaN)};

      // IsNaN
      for (int i = 0; i < 4; ++i)
      {
        PLASMA_TEST_BOOL(nanArray[i].IsNaN());
        PLASMA_TEST_BOOL(!compArray[i].IsNaN());
      }

      // IsValid
      for (int i = 0; i < 4; ++i)
      {
        PLASMA_TEST_BOOL(!nanArray[i].IsValid());
        PLASMA_TEST_BOOL(compArray[i].IsValid());

        PLASMA_TEST_BOOL(!(compArray[i] * plMath::Infinity<plMathTestType>()).IsValid());
        PLASMA_TEST_BOOL(!(compArray[i] * -plMath::Infinity<plMathTestType>()).IsValid());
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Operators")
  {
    const plVec4T vOp1(-4.0, 0.2f, -7.0f, -0.0f);
    const plVec4T vOp2(2.0, 0.3f, 0.0f, 1.0f);
    const plVec4T compArray[4] = {
      plVec4T(1.0f, 0.0f, 0.0f, 0.0f), plVec4T(0.0f, 1.0f, 0.0f, 0.0f), plVec4T(0.0f, 0.0f, 1.0f, 0.0f), plVec4T(0.0f, 0.0f, 0.0f, 1.0f)};
    // IsIdentical
    PLASMA_TEST_BOOL(vOp1.IsIdentical(vOp1));
    for (int i = 0; i < 4; ++i)
    {
      PLASMA_TEST_BOOL(!vOp1.IsIdentical(vOp1 + (plMathTestType)plMath::SmallEpsilon<plMathTestType>() * compArray[i]));
      PLASMA_TEST_BOOL(!vOp1.IsIdentical(vOp1 - (plMathTestType)plMath::SmallEpsilon<plMathTestType>() * compArray[i]));
    }

    // IsEqual
    PLASMA_TEST_BOOL(vOp1.IsEqual(vOp1, 0.0f));
    for (int i = 0; i < 4; ++i)
    {
      PLASMA_TEST_BOOL(vOp1.IsEqual(vOp1 + plMath::SmallEpsilon<plMathTestType>() * compArray[i], 2 * plMath::SmallEpsilon<plMathTestType>()));
      PLASMA_TEST_BOOL(vOp1.IsEqual(vOp1 - plMath::SmallEpsilon<plMathTestType>() * compArray[i], 2 * plMath::SmallEpsilon<plMathTestType>()));
      PLASMA_TEST_BOOL(vOp1.IsEqual(vOp1 + plMath::DefaultEpsilon<plMathTestType>() * compArray[i], 2 * plMath::DefaultEpsilon<plMathTestType>()));
      PLASMA_TEST_BOOL(vOp1.IsEqual(vOp1 - plMath::DefaultEpsilon<plMathTestType>() * compArray[i], 2 * plMath::DefaultEpsilon<plMathTestType>()));
    }

    // operator-
    plVec4T vNegated = -vOp1;
    PLASMA_TEST_BOOL(vOp1.x == -vNegated.x && vOp1.y == -vNegated.y && vOp1.z == -vNegated.z && vOp1.w == -vNegated.w);

    // operator+= (plVec4T)
    plVec4T vPlusAssign = vOp1;
    vPlusAssign += vOp2;
    PLASMA_TEST_BOOL(vPlusAssign.IsEqual(plVec4T(-2.0f, 0.5f, -7.0f, 1.0f), plMath::SmallEpsilon<plMathTestType>()));

    // operator-= (plVec4T)
    plVec4T vMinusAssign = vOp1;
    vMinusAssign -= vOp2;
    PLASMA_TEST_BOOL(vMinusAssign.IsEqual(plVec4T(-6.0f, -0.1f, -7.0f, -1.0f), plMath::SmallEpsilon<plMathTestType>()));

    // operator*= (float)
    plVec4T vMulFloat = vOp1;
    vMulFloat *= 2.0f;
    PLASMA_TEST_BOOL(vMulFloat.IsEqual(plVec4T(-8.0f, 0.4f, -14.0f, -0.0f), plMath::SmallEpsilon<plMathTestType>()));
    vMulFloat *= 0.0f;
    PLASMA_TEST_BOOL(vMulFloat.IsEqual(plVec4T::ZeroVector(), plMath::SmallEpsilon<plMathTestType>()));

    // operator/= (float)
    plVec4T vDivFloat = vOp1;
    vDivFloat /= 2.0f;
    PLASMA_TEST_BOOL(vDivFloat.IsEqual(plVec4T(-2.0f, 0.1f, -3.5f, -0.0f), plMath::SmallEpsilon<plMathTestType>()));

    // operator+ (plVec4T, plVec4T)
    plVec4T vPlus = (vOp1 + vOp2);
    PLASMA_TEST_BOOL(vPlus.IsEqual(plVec4T(-2.0f, 0.5f, -7.0f, 1.0f), plMath::SmallEpsilon<plMathTestType>()));

    // operator- (plVec4T, plVec4T)
    plVec4T vMinus = (vOp1 - vOp2);
    PLASMA_TEST_BOOL(vMinus.IsEqual(plVec4T(-6.0f, -0.1f, -7.0f, -1.0f), plMath::SmallEpsilon<plMathTestType>()));

    // operator* (float, plVec4T)
    plVec4T vMulFloatVec4 = ((plMathTestType)2 * vOp1);
    PLASMA_TEST_BOOL(vMulFloatVec4.IsEqual(plVec4T(-8.0f, 0.4f, -14.0f, -0.0f), plMath::SmallEpsilon<plMathTestType>()));
    vMulFloatVec4 = ((plMathTestType)0 * vOp1);
    PLASMA_TEST_BOOL(vMulFloatVec4.IsEqual(plVec4T::ZeroVector(), plMath::SmallEpsilon<plMathTestType>()));

    // operator* (plVec4T, float)
    plVec4T vMulVec4Float = (vOp1 * (plMathTestType)2);
    PLASMA_TEST_BOOL(vMulVec4Float.IsEqual(plVec4T(-8.0f, 0.4f, -14.0f, -0.0f), plMath::SmallEpsilon<plMathTestType>()));
    vMulVec4Float = (vOp1 * (plMathTestType)0);
    PLASMA_TEST_BOOL(vMulVec4Float.IsEqual(plVec4T::ZeroVector(), plMath::SmallEpsilon<plMathTestType>()));

    // operator/ (plVec4T, float)
    plVec4T vDivVec4Float = (vOp1 / (plMathTestType)2);
    PLASMA_TEST_BOOL(vDivVec4Float.IsEqual(plVec4T(-2.0f, 0.1f, -3.5f, -0.0f), plMath::SmallEpsilon<plMathTestType>()));

    // operator== (plVec4T, plVec4T)
    PLASMA_TEST_BOOL(vOp1 == vOp1);
    for (int i = 0; i < 4; ++i)
    {
      PLASMA_TEST_BOOL(!(vOp1 == (vOp1 + (plMathTestType)plMath::SmallEpsilon<plMathTestType>() * compArray[i])));
      PLASMA_TEST_BOOL(!(vOp1 == (vOp1 - (plMathTestType)plMath::SmallEpsilon<plMathTestType>() * compArray[i])));
    }

    // operator!= (plVec4T, plVec4T)
    PLASMA_TEST_BOOL(!(vOp1 != vOp1));
    for (int i = 0; i < 4; ++i)
    {
      PLASMA_TEST_BOOL(vOp1 != (vOp1 + (plMathTestType)plMath::SmallEpsilon<plMathTestType>() * compArray[i]));
      PLASMA_TEST_BOOL(vOp1 != (vOp1 - (plMathTestType)plMath::SmallEpsilon<plMathTestType>() * compArray[i]));
    }

    // operator< (plVec4T, plVec4T)
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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Common")
  {
    const plVec4T vOp1(-4.0, 0.2f, -7.0f, -0.0f);
    const plVec4T vOp2(2.0, -0.3f, 0.5f, 1.0f);

    // Dot
    PLASMA_TEST_FLOAT(vOp1.Dot(vOp2), -11.56f, plMath::SmallEpsilon<plMathTestType>());
    PLASMA_TEST_FLOAT(vOp2.Dot(vOp1), -11.56f, plMath::SmallEpsilon<plMathTestType>());

    // CompMin
    PLASMA_TEST_BOOL(vOp1.CompMin(vOp2).IsEqual(plVec4T(-4.0f, -0.3f, -7.0f, -0.0f), plMath::SmallEpsilon<plMathTestType>()));
    PLASMA_TEST_BOOL(vOp2.CompMin(vOp1).IsEqual(plVec4T(-4.0f, -0.3f, -7.0f, -0.0f), plMath::SmallEpsilon<plMathTestType>()));

    // CompMax
    PLASMA_TEST_BOOL(vOp1.CompMax(vOp2).IsEqual(plVec4T(2.0f, 0.2f, 0.5f, 1.0f), plMath::SmallEpsilon<plMathTestType>()));
    PLASMA_TEST_BOOL(vOp2.CompMax(vOp1).IsEqual(plVec4T(2.0f, 0.2f, 0.5f, 1.0f), plMath::SmallEpsilon<plMathTestType>()));

    // CompClamp
    PLASMA_TEST_BOOL(vOp1.CompClamp(vOp1, vOp2).IsEqual(plVec4T(-4.0f, -0.3f, -7.0f, -0.0f), plMath::SmallEpsilon<plMathTestType>()));
    PLASMA_TEST_BOOL(vOp2.CompClamp(vOp1, vOp2).IsEqual(plVec4T(2.0f, 0.2f, 0.5f, 1.0f), plMath::SmallEpsilon<plMathTestType>()));

    // CompMul
    PLASMA_TEST_BOOL(vOp1.CompMul(vOp2).IsEqual(plVec4T(-8.0f, -0.06f, -3.5f, 0.0f), plMath::SmallEpsilon<plMathTestType>()));
    PLASMA_TEST_BOOL(vOp2.CompMul(vOp1).IsEqual(plVec4T(-8.0f, -0.06f, -3.5f, 0.0f), plMath::SmallEpsilon<plMathTestType>()));

    // CompDiv
    PLASMA_TEST_BOOL(vOp1.CompDiv(vOp2).IsEqual(plVec4T(-2.0f, -0.66666666f, -14.0f, 0.0f), plMath::SmallEpsilon<plMathTestType>()));

    // Abs
    PLASMA_TEST_VEC4(vOp1.Abs(), plVec4T(4.0, 0.2f, 7.0f, 0.0f), plMath::SmallEpsilon<plMathTestType>());
  }
}

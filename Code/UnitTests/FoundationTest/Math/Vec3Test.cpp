#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>


PLASMA_CREATE_SIMPLE_TEST(Math, Vec3)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    if (plMath::SupportsNaN<plVec3T::ComponentType>())
    {
      // In debug the default constructor initializes everything with NaN.
      plVec3T vDefCtor;
      PLASMA_TEST_BOOL(plMath::IsNaN(vDefCtor.x) && plMath::IsNaN(vDefCtor.y) && plMath::IsNaN(vDefCtor.z));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    plVec3T::ComponentType testBlock[3] = {(plVec3T::ComponentType)1, (plVec3T::ComponentType)2, (plVec3T::ComponentType)3};
    plVec3T* pDefCtor = ::new ((void*)&testBlock[0]) plVec3T;
    PLASMA_TEST_BOOL(pDefCtor->x == (plVec3T::ComponentType)1 && pDefCtor->y == (plVec3T::ComponentType)2 && pDefCtor->z == (plVec3T::ComponentType)3.);
#endif

    // Make sure the class didn't accidentally change in size.
    PLASMA_TEST_BOOL(sizeof(plVec3) == 12);
    PLASMA_TEST_BOOL(sizeof(plVec3d) == 24);

    plVec3T vInit1F(2.0f);
    PLASMA_TEST_BOOL(vInit1F.x == 2.0f && vInit1F.y == 2.0f && vInit1F.z == 2.0f);

    plVec3T vInit4F(1.0f, 2.0f, 3.0f);
    PLASMA_TEST_BOOL(vInit4F.x == 1.0f && vInit4F.y == 2.0f && vInit4F.z == 3.0f);

    plVec3T vCopy(vInit4F);
    PLASMA_TEST_BOOL(vCopy.x == 1.0f && vCopy.y == 2.0f && vCopy.z == 3.0f);

    plVec3T vZero = plVec3T::ZeroVector();
    PLASMA_TEST_BOOL(vZero.x == 0.0f && vZero.y == 0.0f && vZero.z == 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Conversion")
  {
    plVec3T vData(1.0f, 2.0f, 3.0f);
    plVec2T vToVec2 = vData.GetAsVec2();
    PLASMA_TEST_BOOL(vToVec2.x == vData.x && vToVec2.y == vData.y);

    plVec4T vToVec4 = vData.GetAsVec4(42.0f);
    PLASMA_TEST_BOOL(vToVec4.x == vData.x && vToVec4.y == vData.y && vToVec4.z == vData.z && vToVec4.w == 42.0f);

    plVec4T vToVec4Pos = vData.GetAsPositionVec4();
    PLASMA_TEST_BOOL(vToVec4Pos.x == vData.x && vToVec4Pos.y == vData.y && vToVec4Pos.z == vData.z && vToVec4Pos.w == 1.0f);

    plVec4T vToVec4Dir = vData.GetAsDirectionVec4();
    PLASMA_TEST_BOOL(vToVec4Dir.x == vData.x && vToVec4Dir.y == vData.y && vToVec4Dir.z == vData.z && vToVec4Dir.w == 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Setter")
  {
    plVec3T vSet1F;
    vSet1F.Set(2.0f);
    PLASMA_TEST_BOOL(vSet1F.x == 2.0f && vSet1F.y == 2.0f && vSet1F.z == 2.0f);

    plVec3T vSet4F;
    vSet4F.Set(1.0f, 2.0f, 3.0f);
    PLASMA_TEST_BOOL(vSet4F.x == 1.0f && vSet4F.y == 2.0f && vSet4F.z == 3.0f);

    plVec3T vSetZero;
    vSetZero.SetZero();
    PLASMA_TEST_BOOL(vSetZero.x == 0.0f && vSetZero.y == 0.0f && vSetZero.z == 0.0f);
  }


  {
    const plVec3T vOp1(-4.0, 4.0f, -2.0f);
    const plVec3T compArray[3] = {plVec3T(1.0f, 0.0f, 0.0f), plVec3T(0.0f, 1.0f, 0.0f), plVec3T(0.0f, 0.0f, 1.0f)};

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLength") { PLASMA_TEST_FLOAT(vOp1.GetLength(), 6.0f, plMath::SmallEpsilon<plMathTestType>()); }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetLength")
    {
      plVec3T vSetLength = vOp1.GetNormalized() * plMath::DefaultEpsilon<plMathTestType>();
      PLASMA_TEST_BOOL(vSetLength.SetLength(4.0f, plMath::LargeEpsilon<plMathTestType>()) == PLASMA_FAILURE);
      PLASMA_TEST_BOOL(vSetLength == plVec3T::ZeroVector());
      vSetLength = vOp1.GetNormalized() * (plMathTestType)0.001;
      PLASMA_TEST_BOOL(vSetLength.SetLength(4.0f, (plMathTestType)plMath::DefaultEpsilon<plMathTestType>()) == PLASMA_SUCCESS);
      PLASMA_TEST_FLOAT(vSetLength.GetLength(), 4.0f, plMath::SmallEpsilon<plMathTestType>());
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLengthSquared") { PLASMA_TEST_FLOAT(vOp1.GetLengthSquared(), 36.0f, plMath::SmallEpsilon<plMathTestType>()); }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLengthAndNormalize")
    {
      plVec3T vLengthAndNorm = vOp1;
      plMathTestType fLength = vLengthAndNorm.GetLengthAndNormalize();
      PLASMA_TEST_FLOAT(vLengthAndNorm.GetLength(), 1.0f, plMath::SmallEpsilon<plMathTestType>());
      PLASMA_TEST_FLOAT(fLength, 6.0f, plMath::SmallEpsilon<plMathTestType>());
      PLASMA_TEST_FLOAT(vLengthAndNorm.x * vLengthAndNorm.x + vLengthAndNorm.y * vLengthAndNorm.y + vLengthAndNorm.z * vLengthAndNorm.z, 1.0f,
        plMath::SmallEpsilon<plMathTestType>());
      PLASMA_TEST_BOOL(vLengthAndNorm.IsNormalized(plMath::SmallEpsilon<plMathTestType>()));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetNormalized")
    {
      plVec3T vGetNorm = vOp1.GetNormalized();
      PLASMA_TEST_FLOAT(vGetNorm.x * vGetNorm.x + vGetNorm.y * vGetNorm.y + vGetNorm.z * vGetNorm.z, 1.0f, plMath::SmallEpsilon<plMathTestType>());
      PLASMA_TEST_BOOL(vGetNorm.IsNormalized(plMath::SmallEpsilon<plMathTestType>()));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Normalize")
    {
      plVec3T vNorm = vOp1;
      vNorm.Normalize();
      PLASMA_TEST_FLOAT(vNorm.x * vNorm.x + vNorm.y * vNorm.y + vNorm.z * vNorm.z, 1.0f, plMath::SmallEpsilon<plMathTestType>());
      PLASMA_TEST_BOOL(vNorm.IsNormalized(plMath::SmallEpsilon<plMathTestType>()));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "NormalizeIfNotZero")
    {
      plVec3T vNorm = vOp1;
      vNorm.Normalize();

      plVec3T vNormCond = vNorm * plMath::DefaultEpsilon<plMathTestType>();
      PLASMA_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, plMath::LargeEpsilon<plMathTestType>()) == PLASMA_FAILURE);
      PLASMA_TEST_BOOL(vNormCond == vOp1);
      vNormCond = vNorm * plMath::DefaultEpsilon<plMathTestType>();
      PLASMA_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, plMath::SmallEpsilon<plMathTestType>()) == PLASMA_SUCCESS);
      PLASMA_TEST_VEC3(vNormCond, vNorm, plMath::DefaultEpsilon<plVec3T::ComponentType>());
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsZero")
    {
      PLASMA_TEST_BOOL(plVec3T::ZeroVector().IsZero());
      for (int i = 0; i < 3; ++i)
      {
        PLASMA_TEST_BOOL(!compArray[i].IsZero());
      }
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsZero(float)")
    {
      PLASMA_TEST_BOOL(plVec3T::ZeroVector().IsZero(0.0f));
      for (int i = 0; i < 3; ++i)
      {
        PLASMA_TEST_BOOL(!compArray[i].IsZero(0.0f));
        PLASMA_TEST_BOOL(compArray[i].IsZero(1.0f));
        PLASMA_TEST_BOOL((-compArray[i]).IsZero(1.0f));
      }
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNormalized (2)")
    {
      for (int i = 0; i < 3; ++i)
      {
        PLASMA_TEST_BOOL(compArray[i].IsNormalized());
        PLASMA_TEST_BOOL((-compArray[i]).IsNormalized());
        PLASMA_TEST_BOOL((compArray[i] * (plMathTestType)2).IsNormalized((plMathTestType)4));
        PLASMA_TEST_BOOL((compArray[i] * (plMathTestType)2).IsNormalized((plMathTestType)4));
      }
    }

    if (plMath::SupportsNaN<plMathTestType>())
    {
      plMathTestType fNaN = plMath::NaN<plMathTestType>();
      const plVec3T nanArray[3] = {plVec3T(fNaN, 0.0f, 0.0f), plVec3T(0.0f, fNaN, 0.0f), plVec3T(0.0f, 0.0f, fNaN)};

      PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNaN")
      {
        for (int i = 0; i < 3; ++i)
        {
          PLASMA_TEST_BOOL(nanArray[i].IsNaN());
          PLASMA_TEST_BOOL(!compArray[i].IsNaN());
        }
      }

      PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsValid")
      {
        for (int i = 0; i < 3; ++i)
        {
          PLASMA_TEST_BOOL(!nanArray[i].IsValid());
          PLASMA_TEST_BOOL(compArray[i].IsValid());

          PLASMA_TEST_BOOL(!(compArray[i] * plMath::Infinity<plMathTestType>()).IsValid());
          PLASMA_TEST_BOOL(!(compArray[i] * -plMath::Infinity<plMathTestType>()).IsValid());
        }
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Operators")
  {
    const plVec3T vOp1(-4.0, 0.2f, -7.0f);
    const plVec3T vOp2(2.0, 0.3f, 0.0f);
    const plVec3T compArray[3] = {plVec3T(1.0f, 0.0f, 0.0f), plVec3T(0.0f, 1.0f, 0.0f), plVec3T(0.0f, 0.0f, 1.0f)};
    // IsIdentical
    PLASMA_TEST_BOOL(vOp1.IsIdentical(vOp1));
    for (int i = 0; i < 3; ++i)
    {
      PLASMA_TEST_BOOL(!vOp1.IsIdentical(vOp1 + (plMathTestType)plMath::SmallEpsilon<plMathTestType>() * compArray[i]));
      PLASMA_TEST_BOOL(!vOp1.IsIdentical(vOp1 - (plMathTestType)plMath::SmallEpsilon<plMathTestType>() * compArray[i]));
    }

    // IsEqual
    PLASMA_TEST_BOOL(vOp1.IsEqual(vOp1, 0.0f));
    for (int i = 0; i < 3; ++i)
    {
      PLASMA_TEST_BOOL(vOp1.IsEqual(vOp1 + plMath::SmallEpsilon<plMathTestType>() * compArray[i], 2 * plMath::SmallEpsilon<plMathTestType>()));
      PLASMA_TEST_BOOL(vOp1.IsEqual(vOp1 - plMath::SmallEpsilon<plMathTestType>() * compArray[i], 2 * plMath::SmallEpsilon<plMathTestType>()));
      PLASMA_TEST_BOOL(vOp1.IsEqual(vOp1 + plMath::DefaultEpsilon<plMathTestType>() * compArray[i], 2 * plMath::DefaultEpsilon<plMathTestType>()));
      PLASMA_TEST_BOOL(vOp1.IsEqual(vOp1 - plMath::DefaultEpsilon<plMathTestType>() * compArray[i], 2 * plMath::DefaultEpsilon<plMathTestType>()));
    }

    // operator-
    plVec3T vNegated = -vOp1;
    PLASMA_TEST_BOOL(vOp1.x == -vNegated.x && vOp1.y == -vNegated.y && vOp1.z == -vNegated.z);

    // operator+= (plVec3T)
    plVec3T vPlusAssign = vOp1;
    vPlusAssign += vOp2;
    PLASMA_TEST_BOOL(vPlusAssign.IsEqual(plVec3T(-2.0f, 0.5f, -7.0f), plMath::SmallEpsilon<plMathTestType>()));

    // operator-= (plVec3T)
    plVec3T vMinusAssign = vOp1;
    vMinusAssign -= vOp2;
    PLASMA_TEST_BOOL(vMinusAssign.IsEqual(plVec3T(-6.0f, -0.1f, -7.0f), plMath::SmallEpsilon<plMathTestType>()));

    // operator*= (float)
    plVec3T vMulFloat = vOp1;
    vMulFloat *= 2.0f;
    PLASMA_TEST_BOOL(vMulFloat.IsEqual(plVec3T(-8.0f, 0.4f, -14.0f), plMath::SmallEpsilon<plMathTestType>()));
    vMulFloat *= 0.0f;
    PLASMA_TEST_BOOL(vMulFloat.IsEqual(plVec3T::ZeroVector(), plMath::SmallEpsilon<plMathTestType>()));

    // operator/= (float)
    plVec3T vDivFloat = vOp1;
    vDivFloat /= 2.0f;
    PLASMA_TEST_BOOL(vDivFloat.IsEqual(plVec3T(-2.0f, 0.1f, -3.5f), plMath::SmallEpsilon<plMathTestType>()));

    // operator+ (plVec3T, plVec3T)
    plVec3T vPlus = (vOp1 + vOp2);
    PLASMA_TEST_BOOL(vPlus.IsEqual(plVec3T(-2.0f, 0.5f, -7.0f), plMath::SmallEpsilon<plMathTestType>()));

    // operator- (plVec3T, plVec3T)
    plVec3T vMinus = (vOp1 - vOp2);
    PLASMA_TEST_BOOL(vMinus.IsEqual(plVec3T(-6.0f, -0.1f, -7.0f), plMath::SmallEpsilon<plMathTestType>()));

    // operator* (float, plVec3T)
    plVec3T vMulFloatVec3 = ((plMathTestType)2 * vOp1);
    PLASMA_TEST_BOOL(
      vMulFloatVec3.IsEqual(plVec3T((plMathTestType)-8.0, (plMathTestType)0.4, (plMathTestType)-14.0), plMath::SmallEpsilon<plMathTestType>()));
    vMulFloatVec3 = ((plMathTestType)0 * vOp1);
    PLASMA_TEST_BOOL(vMulFloatVec3.IsEqual(plVec3T::ZeroVector(), plMath::SmallEpsilon<plMathTestType>()));

    // operator* (plVec3T, float)
    plVec3T vMulVec3Float = (vOp1 * (plMathTestType)2);
    PLASMA_TEST_BOOL(vMulVec3Float.IsEqual(plVec3T(-8.0f, 0.4f, -14.0f), plMath::SmallEpsilon<plMathTestType>()));
    vMulVec3Float = (vOp1 * (plMathTestType)0);
    PLASMA_TEST_BOOL(vMulVec3Float.IsEqual(plVec3T::ZeroVector(), plMath::SmallEpsilon<plMathTestType>()));

    // operator/ (plVec3T, float)
    plVec3T vDivVec3Float = (vOp1 / (plMathTestType)2);
    PLASMA_TEST_BOOL(vDivVec3Float.IsEqual(plVec3T(-2.0f, 0.1f, -3.5f), plMath::SmallEpsilon<plMathTestType>()));

    // operator== (plVec3T, plVec3T)
    PLASMA_TEST_BOOL(vOp1 == vOp1);
    for (int i = 0; i < 3; ++i)
    {
      PLASMA_TEST_BOOL(!(vOp1 == (vOp1 + (plMathTestType)plMath::SmallEpsilon<plMathTestType>() * compArray[i])));
      PLASMA_TEST_BOOL(!(vOp1 == (vOp1 - (plMathTestType)plMath::SmallEpsilon<plMathTestType>() * compArray[i])));
    }

    // operator!= (plVec3T, plVec3T)
    PLASMA_TEST_BOOL(!(vOp1 != vOp1));
    for (int i = 0; i < 3; ++i)
    {
      PLASMA_TEST_BOOL(vOp1 != (vOp1 + (plMathTestType)plMath::SmallEpsilon<plMathTestType>() * compArray[i]));
      PLASMA_TEST_BOOL(vOp1 != (vOp1 - (plMathTestType)plMath::SmallEpsilon<plMathTestType>() * compArray[i]));
    }

    // operator< (plVec3T, plVec3T)
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
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
    const plVec3T vOp1(-4.0, 0.2f, -7.0f);
    const plVec3T vOp2(2.0, -0.3f, 0.5f);

    const plVec3T compArray[3] = {plVec3T(1.0f, 0.0f, 0.0f), plVec3T(0.0f, 1.0f, 0.0f), plVec3T(0.0f, 0.0f, 1.0f)};

    // GetAngleBetween
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        PLASMA_TEST_FLOAT(compArray[i].GetAngleBetween(compArray[j]).GetDegree(), i == j ? 0.0f : 90.0f, 0.00001f);
      }
    }

    // Dot
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        PLASMA_TEST_FLOAT(compArray[i].Dot(compArray[j]), i == j ? 1.0f : 0.0f, plMath::SmallEpsilon<plMathTestType>());
      }
    }
    PLASMA_TEST_FLOAT(vOp1.Dot(vOp2), -11.56f, plMath::SmallEpsilon<plMathTestType>());
    PLASMA_TEST_FLOAT(vOp2.Dot(vOp1), -11.56f, plMath::SmallEpsilon<plMathTestType>());

    // Cross
    // Right-handed coordinate system check
    PLASMA_TEST_BOOL(compArray[0].CrossRH(compArray[1]).IsEqual(compArray[2], plMath::SmallEpsilon<plMathTestType>()));
    PLASMA_TEST_BOOL(compArray[1].CrossRH(compArray[2]).IsEqual(compArray[0], plMath::SmallEpsilon<plMathTestType>()));
    PLASMA_TEST_BOOL(compArray[2].CrossRH(compArray[0]).IsEqual(compArray[1], plMath::SmallEpsilon<plMathTestType>()));

    // CompMin
    PLASMA_TEST_BOOL(vOp1.CompMin(vOp2).IsEqual(plVec3T(-4.0f, -0.3f, -7.0f), plMath::SmallEpsilon<plMathTestType>()));
    PLASMA_TEST_BOOL(vOp2.CompMin(vOp1).IsEqual(plVec3T(-4.0f, -0.3f, -7.0f), plMath::SmallEpsilon<plMathTestType>()));

    // CompMax
    PLASMA_TEST_BOOL(vOp1.CompMax(vOp2).IsEqual(plVec3T(2.0f, 0.2f, 0.5f), plMath::SmallEpsilon<plMathTestType>()));
    PLASMA_TEST_BOOL(vOp2.CompMax(vOp1).IsEqual(plVec3T(2.0f, 0.2f, 0.5f), plMath::SmallEpsilon<plMathTestType>()));

    // CompClamp
    PLASMA_TEST_BOOL(vOp1.CompClamp(vOp1, vOp2).IsEqual(plVec3T(-4.0f, -0.3f, -7.0f), plMath::SmallEpsilon<plMathTestType>()));
    PLASMA_TEST_BOOL(vOp2.CompClamp(vOp1, vOp2).IsEqual(plVec3T(2.0f, 0.2f, 0.5f), plMath::SmallEpsilon<plMathTestType>()));

    // CompMul
    PLASMA_TEST_BOOL(vOp1.CompMul(vOp2).IsEqual(plVec3T(-8.0f, -0.06f, -3.5f), plMath::SmallEpsilon<plMathTestType>()));
    PLASMA_TEST_BOOL(vOp2.CompMul(vOp1).IsEqual(plVec3T(-8.0f, -0.06f, -3.5f), plMath::SmallEpsilon<plMathTestType>()));

    // CompDiv
    PLASMA_TEST_BOOL(vOp1.CompDiv(vOp2).IsEqual(plVec3T(-2.0f, -0.66666666f, -14.0f), plMath::SmallEpsilon<plMathTestType>()));

    // Abs
    PLASMA_TEST_VEC3(vOp1.Abs(), plVec3T(4.0, 0.2f, 7.0f), plMath::SmallEpsilon<plMathTestType>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CalculateNormal")
  {
    plVec3T n;
    PLASMA_TEST_BOOL(n.CalculateNormal(plVec3T(-1, 0, 1), plVec3T(1, 0, 1), plVec3T(0, 0, -1)) == PLASMA_SUCCESS);
    PLASMA_TEST_VEC3(n, plVec3T(0, 1, 0), 0.001f);

    PLASMA_TEST_BOOL(n.CalculateNormal(plVec3T(-1, 0, -1), plVec3T(1, 0, -1), plVec3T(0, 0, 1)) == PLASMA_SUCCESS);
    PLASMA_TEST_VEC3(n, plVec3T(0, -1, 0), 0.001f);

    PLASMA_TEST_BOOL(n.CalculateNormal(plVec3T(-1, 0, 1), plVec3T(1, 0, 1), plVec3T(1, 0, 1)) == PLASMA_FAILURE);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "MakeOrthogonalTo")
  {
    plVec3T v;

    v.Set(1, 1, 0);
    v.MakeOrthogonalTo(plVec3T(1, 0, 0));
    PLASMA_TEST_VEC3(v, plVec3T(0, 1, 0), 0.001f);

    v.Set(1, 1, 0);
    v.MakeOrthogonalTo(plVec3T(0, 1, 0));
    PLASMA_TEST_VEC3(v, plVec3T(1, 0, 0), 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetOrthogonalVector")
  {
    plVec3T v;

    for (float i = 1; i < 360; i += 3.0f)
    {
      v.Set(i, i * 3, i * 7);
      PLASMA_TEST_FLOAT(v.GetOrthogonalVector().Dot(v), 0.0f, 0.001f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetReflectedVector")
  {
    plVec3T v, v2;

    v.Set(1, 1, 0);
    v2 = v.GetReflectedVector(plVec3T(0, -1, 0));
    PLASMA_TEST_VEC3(v2, plVec3T(1, -1, 0), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CreateRandomPointInSphere")
  {
    plVec3T v;

    plRandom rng;
    rng.Initialize(0xEEFF0011AABBCCDDULL);

    plVec3T avg;
    avg.SetZero();

    const plUInt32 uiNumSamples = 100'000;
    for (plUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = plVec3T::CreateRandomPointInSphere(rng);

      PLASMA_TEST_BOOL(v.GetLength() <= 1.0f + plMath::SmallEpsilon<float>());
      PLASMA_TEST_BOOL(!v.IsZero());

      avg += v;
    }

    avg /= (float)uiNumSamples;

    // the average point cloud center should be within at least 10% of the sphere's center
    // otherwise the points aren't equally distributed
    PLASMA_TEST_BOOL(avg.IsZero(0.1f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CreateRandomDirection")
  {
    plVec3T v;

    plRandom rng;
    rng.InitializeFromCurrentTime();

    plVec3T avg;
    avg.SetZero();

    const plUInt32 uiNumSamples = 100'000;
    for (plUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = plVec3T::CreateRandomDirection(rng);

      PLASMA_TEST_BOOL(v.IsNormalized());

      avg += v;
    }

    avg /= (float)uiNumSamples;

    // the average point cloud center should be within at least 10% of the sphere's center
    // otherwise the points aren't equally distributed
    PLASMA_TEST_BOOL(avg.IsZero(0.1f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CreateRandomDeviationX")
  {
    plVec3T v;
    plVec3T avg;
    avg.SetZero();

    plRandom rng;
    rng.InitializeFromCurrentTime();

    const plAngle dev = plAngle::Degree(65);
    const plUInt32 uiNumSamples = 100'000;
    const plVec3 vAxis(1, 0, 0);

    for (plUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = plVec3T::CreateRandomDeviationX(rng, dev);

      PLASMA_TEST_BOOL(v.IsNormalized());

      PLASMA_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + plMath::DefaultEpsilon<float>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    PLASMA_TEST_BOOL(avg.IsEqual(vAxis, 0.1f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CreateRandomDeviationY")
  {
    plVec3T v;
    plVec3T avg;
    avg.SetZero();

    plRandom rng;
    rng.InitializeFromCurrentTime();

    const plAngle dev = plAngle::Degree(65);
    const plUInt32 uiNumSamples = 100'000;
    const plVec3 vAxis(0, 1, 0);

    for (plUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = plVec3T::CreateRandomDeviationY(rng, dev);

      PLASMA_TEST_BOOL(v.IsNormalized());

      PLASMA_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + plMath::DefaultEpsilon<float>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    PLASMA_TEST_BOOL(avg.IsEqual(vAxis, 0.1f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CreateRandomDeviationZ")
  {
    plVec3T v;
    plVec3T avg;
    avg.SetZero();

    plRandom rng;
    rng.InitializeFromCurrentTime();

    const plAngle dev = plAngle::Degree(65);
    const plUInt32 uiNumSamples = 100'000;
    const plVec3 vAxis(0, 0, 1);

    for (plUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = plVec3T::CreateRandomDeviationZ(rng, dev);

      PLASMA_TEST_BOOL(v.IsNormalized());

      PLASMA_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + plMath::DefaultEpsilon<float>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    PLASMA_TEST_BOOL(avg.IsEqual(vAxis, 0.1f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CreateRandomDeviation")
  {
    plVec3T v;

    plRandom rng;
    rng.InitializeFromCurrentTime();

    const plAngle dev = plAngle::Degree(65);
    const plUInt32 uiNumSamples = 100'000;
    plVec3 vAxis;

    for (plUInt32 i = 0; i < uiNumSamples; ++i)
    {
      vAxis = plVec3T::CreateRandomDirection(rng);

      v = plVec3T::CreateRandomDeviation(rng, dev, vAxis);

      PLASMA_TEST_BOOL(v.IsNormalized());

      PLASMA_TEST_BOOL(vAxis.GetAngleBetween(v).GetDegree() <= dev.GetDegree() + 1.0f);
    }
  }
}

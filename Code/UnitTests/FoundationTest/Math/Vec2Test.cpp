#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

#include <Foundation/Math/FixedPoint.h>

PLASMA_CREATE_SIMPLE_TEST(Math, Vec2)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    if (plMath::SupportsNaN<plMathTestType>())
    {
      // In debug the default constructor initializes everything with NaN.
      plVec2T vDefCtor;
      PLASMA_TEST_BOOL(plMath::IsNaN(vDefCtor.x) && plMath::IsNaN(vDefCtor.y));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    plVec2T::ComponentType testBlock[2] = {(plVec2T::ComponentType)1, (plVec2T::ComponentType)2};
    plVec2T* pDefCtor = ::new ((void*)&testBlock[0]) plVec2T;
    PLASMA_TEST_BOOL(pDefCtor->x == (plVec2T::ComponentType)1 && pDefCtor->y == (plVec2T::ComponentType)2);
#endif
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor(x,y)")
  {
    plVec2T v(1, 2);
    PLASMA_TEST_FLOAT(v.x, 1, 0);
    PLASMA_TEST_FLOAT(v.y, 2, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor(xy)")
  {
    plVec2T v(3);
    PLASMA_TEST_VEC2(v, plVec2T(3, 3), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ZeroVector") { PLASMA_TEST_VEC2(plVec2T::ZeroVector(), plVec2T(0, 0), 0); }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetAsVec3") { PLASMA_TEST_VEC3(plVec2T(2, 3).GetAsVec3(4), plVec3T(2, 3, 4), 0); }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetAsVec4") { PLASMA_TEST_VEC4(plVec2T(2, 3).GetAsVec4(4, 5), plVec4T(2, 3, 4, 5), 0); }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Set(x, y)")
  {
    plVec2T v;
    v.Set(2, 3);

    PLASMA_TEST_FLOAT(v.x, 2, 0);
    PLASMA_TEST_FLOAT(v.y, 3, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Set(xy)")
  {
    plVec2T v;
    v.Set(4);

    PLASMA_TEST_FLOAT(v.x, 4, 0);
    PLASMA_TEST_FLOAT(v.y, 4, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetZero")
  {
    plVec2T v;
    v.Set(4);
    v.SetZero();

    PLASMA_TEST_FLOAT(v.x, 0, 0);
    PLASMA_TEST_FLOAT(v.y, 0, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLength")
  {
    plVec2T v(0);
    PLASMA_TEST_FLOAT(v.GetLength(), 0, 0.0001f);

    v.Set(1, 0);
    PLASMA_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(0, 1);
    PLASMA_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(2, 3);
    PLASMA_TEST_FLOAT(v.GetLength(), plMath::Sqrt((plMathTestType)(4 + 9)), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLengthSquared")
  {
    plVec2T v(0);
    PLASMA_TEST_FLOAT(v.GetLengthSquared(), 0, 0.0001f);

    v.Set(1, 0);
    PLASMA_TEST_FLOAT(v.GetLengthSquared(), 1, 0.0001f);

    v.Set(0, 1);
    PLASMA_TEST_FLOAT(v.GetLengthSquared(), 1, 0.0001f);

    v.Set(2, 3);
    PLASMA_TEST_FLOAT(v.GetLengthSquared(), 4 + 9, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLengthAndNormalize")
  {
    plVec2T v(0.5f, 0);
    plVec2T::ComponentType l = v.GetLengthAndNormalize();
    PLASMA_TEST_FLOAT(l, 0.5f, 0.0001f);
    PLASMA_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(1, 0);
    l = v.GetLengthAndNormalize();
    PLASMA_TEST_FLOAT(l, 1, 0.0001f);
    PLASMA_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(0, 1);
    l = v.GetLengthAndNormalize();
    PLASMA_TEST_FLOAT(l, 1, 0.0001f);
    PLASMA_TEST_FLOAT(v.GetLength(), 1, 0.0001f);

    v.Set(2, 3);
    l = v.GetLengthAndNormalize();
    PLASMA_TEST_FLOAT(l, plMath::Sqrt((plMathTestType)(4 + 9)), 0.0001f);
    PLASMA_TEST_FLOAT(v.GetLength(), 1, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetNormalized")
  {
    plVec2T v;

    v.Set(10, 0);
    PLASMA_TEST_VEC2(v.GetNormalized(), plVec2T(1, 0), 0.001f);

    v.Set(0, 10);
    PLASMA_TEST_VEC2(v.GetNormalized(), plVec2T(0, 1), 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Normalize")
  {
    plVec2T v;

    v.Set(10, 0);
    v.Normalize();
    PLASMA_TEST_VEC2(v, plVec2T(1, 0), 0.001f);

    v.Set(0, 10);
    v.Normalize();
    PLASMA_TEST_VEC2(v, plVec2T(0, 1), 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "NormalizeIfNotZero")
  {
    plVec2T v;

    v.Set(10, 0);
    PLASMA_TEST_BOOL(v.NormalizeIfNotZero() == PLASMA_SUCCESS);
    PLASMA_TEST_VEC2(v, plVec2T(1, 0), 0.001f);

    v.Set(0, 10);
    PLASMA_TEST_BOOL(v.NormalizeIfNotZero() == PLASMA_SUCCESS);
    PLASMA_TEST_VEC2(v, plVec2T(0, 1), 0.001f);

    v.SetZero();
    PLASMA_TEST_BOOL(v.NormalizeIfNotZero() == PLASMA_FAILURE);
    PLASMA_TEST_VEC2(v, plVec2T(1, 0), 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsZero")
  {
    plVec2T v;

    v.Set(1);
    PLASMA_TEST_BOOL(v.IsZero() == false);

    v.Set(0.001f);
    PLASMA_TEST_BOOL(v.IsZero() == false);
    PLASMA_TEST_BOOL(v.IsZero(0.01f) == true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNormalized")
  {
    plVec2T v;

    v.SetZero();
    PLASMA_TEST_BOOL(v.IsNormalized(plMath::HugeEpsilon<plMathTestType>()) == false);

    v.Set(1, 0);
    PLASMA_TEST_BOOL(v.IsNormalized(plMath::HugeEpsilon<plMathTestType>()) == true);

    v.Set(0, 1);
    PLASMA_TEST_BOOL(v.IsNormalized(plMath::HugeEpsilon<plMathTestType>()) == true);

    v.Set(0.1f, 1);
    PLASMA_TEST_BOOL(v.IsNormalized(plMath::DefaultEpsilon<plMathTestType>()) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNaN")
  {
    if (plMath::SupportsNaN<plMathTestType>())
    {
      plVec2T v(0);

      PLASMA_TEST_BOOL(!v.IsNaN());

      v.x = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(v.IsNaN());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsValid")
  {
    if (plMath::SupportsNaN<plMathTestType>())
    {
      plVec2T v(0);

      PLASMA_TEST_BOOL(v.IsValid());

      v.x = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(!v.IsValid());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator-")
  {
    plVec2T v(1);

    PLASMA_TEST_VEC2(-v, plVec2T(-1), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator+=")
  {
    plVec2T v(1, 2);

    v += plVec2T(3, 4);
    PLASMA_TEST_VEC2(v, plVec2T(4, 6), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator-=")
  {
    plVec2T v(1, 2);

    v -= plVec2T(3, 5);
    PLASMA_TEST_VEC2(v, plVec2T(-2, -3), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*=(float)")
  {
    plVec2T v(1, 2);

    v *= 3;
    PLASMA_TEST_VEC2(v, plVec2T(3, 6), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator/=(float)")
  {
    plVec2T v(1, 2);

    v /= 2;
    PLASMA_TEST_VEC2(v, plVec2T(0.5f, 1), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsIdentical")
  {
    plVec2T v1(1, 2);
    plVec2T v2 = v1;

    PLASMA_TEST_BOOL(v1.IsIdentical(v2));

    v2.x += plVec2T::ComponentType(0.001f);
    PLASMA_TEST_BOOL(!v1.IsIdentical(v2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqual")
  {
    plVec2T v1(1, 2);
    plVec2T v2 = v1;

    PLASMA_TEST_BOOL(v1.IsEqual(v2, 0.00001f));

    v2.x += plVec2T::ComponentType(0.001f);
    PLASMA_TEST_BOOL(!v1.IsEqual(v2, 0.0001f));
    PLASMA_TEST_BOOL(v1.IsEqual(v2, 0.01f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetAngleBetween")
  {
    plVec2T v1(1, 0);
    plVec2T v2(0, 1);

    PLASMA_TEST_FLOAT(v1.GetAngleBetween(v1).GetDegree(), 0, 0.001f);
    PLASMA_TEST_FLOAT(v2.GetAngleBetween(v2).GetDegree(), 0, 0.001f);
    PLASMA_TEST_FLOAT(v1.GetAngleBetween(v2).GetDegree(), 90, 0.001f);
    PLASMA_TEST_FLOAT(v1.GetAngleBetween(-v1).GetDegree(), 180, 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Dot")
  {
    plVec2T v1(1, 0);
    plVec2T v2(0, 1);
    plVec2T v0(0, 0);

    PLASMA_TEST_FLOAT(v0.Dot(v0), 0, 0.001f);
    PLASMA_TEST_FLOAT(v1.Dot(v1), 1, 0.001f);
    PLASMA_TEST_FLOAT(v2.Dot(v2), 1, 0.001f);
    PLASMA_TEST_FLOAT(v1.Dot(v2), 0, 0.001f);
    PLASMA_TEST_FLOAT(v1.Dot(-v1), -1, 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompMin")
  {
    plVec2T v1(2, 3);
    plVec2T v2 = v1.CompMin(plVec2T(1, 4));
    PLASMA_TEST_VEC2(v2, plVec2T(1, 3), 0);

    v2 = v1.CompMin(plVec2T(3, 1));
    PLASMA_TEST_VEC2(v2, plVec2T(2, 1), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompMax")
  {
    plVec2T v1(2, 3.5f);
    plVec2T v2 = v1.CompMax(plVec2T(1, 4));
    PLASMA_TEST_VEC2(v2, plVec2T(2, 4), 0);

    v2 = v1.CompMax(plVec2T(3, 1));
    PLASMA_TEST_VEC2(v2, plVec2T(3, 3.5f), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompClamp")
  {
    const plVec2T vOp1(-4.0, 0.2f);
    const plVec2T vOp2(2.0, -0.3f);

    PLASMA_TEST_BOOL(vOp1.CompClamp(vOp1, vOp2).IsEqual(plVec2T(-4.0f, -0.3f), plMath::SmallEpsilon<plMathTestType>()));
    PLASMA_TEST_BOOL(vOp2.CompClamp(vOp1, vOp2).IsEqual(plVec2T(2.0f, 0.2f), plMath::SmallEpsilon<plMathTestType>()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompMul")
  {
    plVec2T v1(2, 3);
    plVec2T v2 = v1.CompMul(plVec2T(2, 4));
    PLASMA_TEST_VEC2(v2, plVec2T(4, 12), 0);

    v2 = v1.CompMul(plVec2T(3, 7));
    PLASMA_TEST_VEC2(v2, plVec2T(6, 21), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompDiv")
  {
    plVec2T v1(12, 32);
    plVec2T v2 = v1.CompDiv(plVec2T(3, 4));
    PLASMA_TEST_VEC2(v2, plVec2T(4, 8), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Abs")
  {
    plVec2T v1(-5, 7);
    plVec2T v2 = v1.Abs();
    PLASMA_TEST_VEC2(v2, plVec2T(5, 7), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "MakeOrthogonalTo")
  {
    plVec2T v;

    v.Set(1, 1);
    v.MakeOrthogonalTo(plVec2T(1, 0));
    PLASMA_TEST_VEC2(v, plVec2T(0, 1), 0.001f);

    v.Set(1, 1);
    v.MakeOrthogonalTo(plVec2T(0, 1));
    PLASMA_TEST_VEC2(v, plVec2T(1, 0), 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetOrthogonalVector")
  {
    plVec2T v;

    for (float i = 1; i < 360; i += 3)
    {
      v.Set(i, i * 3);
      PLASMA_TEST_FLOAT(v.GetOrthogonalVector().Dot(v), 0, 0.001f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetReflectedVector")
  {
    plVec2T v, v2;

    v.Set(1, 1);
    v2 = v.GetReflectedVector(plVec2T(0, -1));
    PLASMA_TEST_VEC2(v2, plVec2T(1, -1), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator+")
  {
    plVec2T v = plVec2T(1, 2) + plVec2T(3, 4);
    PLASMA_TEST_VEC2(v, plVec2T(4, 6), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator-")
  {
    plVec2T v = plVec2T(1, 2) - plVec2T(3, 5);
    PLASMA_TEST_VEC2(v, plVec2T(-2, -3), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator* (vec, float) | operator* (float, vec)")
  {
    plVec2T v = plVec2T(1, 2) * plVec2T::ComponentType(3);
    PLASMA_TEST_VEC2(v, plVec2T(3, 6), 0.0001f);

    v = plVec2T::ComponentType(7) * plVec2T(1, 2);
    PLASMA_TEST_VEC2(v, plVec2T(7, 14), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator/ (vec, float)")
  {
    plVec2T v = plVec2T(2, 4) / plVec2T::ComponentType(2);
    PLASMA_TEST_VEC2(v, plVec2T(1, 2), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator== | operator!=")
  {
    plVec2T v1(1, 2);
    plVec2T v2 = v1;

    PLASMA_TEST_BOOL(v1 == v2);

    v2.x += plVec2T::ComponentType(0.001f);
    PLASMA_TEST_BOOL(v1 != v2);
  }
}

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>

#define PLASMA_TEST_SIMD_VECTOR_EQUAL(NUM_COMPONENTS, A, B, EPSILON)                                                                                               \
  do                                                                                                                                                           \
  {                                                                                                                                                            \
    auto _plDiff = B - A;                                                                                                                                      \
    plTestBool((A).IsEqual((B), EPSILON).AllSet<NUM_COMPONENTS>(), "Test failed: " PLASMA_STRINGIZE(A) ".IsEqual(" PLASMA_STRINGIZE(B) ", " PLASMA_STRINGIZE(EPSILON) ")", \
      PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION,                                                                                                      \
      "Difference %lf %lf %lf %lf", _plDiff.x(), _plDiff.y(), _plDiff.z(), _plDiff.w());                                                                       \
  } while (false)


PLASMA_CREATE_SIMPLE_TEST(SimdMath, SimdBBox)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plSimdBBox b(plSimdVec4f(-1, -2, -3), plSimdVec4f(1, 2, 3));

    PLASMA_TEST_BOOL((b.m_Min == plSimdVec4f(-1, -2, -3)).AllSet<3>());
    PLASMA_TEST_BOOL((b.m_Max == plSimdVec4f(1, 2, 3)).AllSet<3>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetInvalid")
  {
    plSimdBBox b;
    b.SetInvalid();

    PLASMA_TEST_BOOL(!b.IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNaN")
  {
    plSimdBBox b;

    b.SetInvalid();
    PLASMA_TEST_BOOL(!b.IsNaN());

    b.SetInvalid();
    b.m_Min.SetX(plMath::NaN<plMathTestType>());
    PLASMA_TEST_BOOL(b.IsNaN());

    b.SetInvalid();
    b.m_Min.SetY(plMath::NaN<plMathTestType>());
    PLASMA_TEST_BOOL(b.IsNaN());

    b.SetInvalid();
    b.m_Min.SetZ(plMath::NaN<plMathTestType>());
    PLASMA_TEST_BOOL(b.IsNaN());

    b.SetInvalid();
    b.m_Max.SetX(plMath::NaN<plMathTestType>());
    PLASMA_TEST_BOOL(b.IsNaN());

    b.SetInvalid();
    b.m_Max.SetY(plMath::NaN<plMathTestType>());
    PLASMA_TEST_BOOL(b.IsNaN());

    b.SetInvalid();
    b.m_Max.SetZ(plMath::NaN<plMathTestType>());
    PLASMA_TEST_BOOL(b.IsNaN());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetCenterAndHalfExtents")
  {
    plSimdBBox b;
    b.SetCenterAndHalfExtents(plSimdVec4f(1, 2, 3), plSimdVec4f(4, 5, 6));

    PLASMA_TEST_BOOL((b.m_Min == plSimdVec4f(-3, -3, -3)).AllSet<3>());
    PLASMA_TEST_BOOL((b.m_Max == plSimdVec4f(5, 7, 9)).AllSet<3>());

    PLASMA_TEST_BOOL((b.GetCenter() == plSimdVec4f(1, 2, 3)).AllSet<3>());
    PLASMA_TEST_BOOL((b.GetExtents() == plSimdVec4f(8, 10, 12)).AllSet<3>());
    PLASMA_TEST_BOOL((b.GetHalfExtents() == plSimdVec4f(4, 5, 6)).AllSet<3>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFromPoints")
  {
    plSimdVec4f p[6] = {
      plSimdVec4f(-4, 0, 0),
      plSimdVec4f(5, 0, 0),
      plSimdVec4f(0, -6, 0),
      plSimdVec4f(0, 7, 0),
      plSimdVec4f(0, 0, -8),
      plSimdVec4f(0, 0, 9),
    };

    plSimdBBox b;
    b.SetFromPoints(p, 6);

    PLASMA_TEST_BOOL((b.m_Min == plSimdVec4f(-4, -6, -8)).AllSet<3>());
    PLASMA_TEST_BOOL((b.m_Max == plSimdVec4f(5, 7, 9)).AllSet<3>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclude (Point)")
  {
    plSimdBBox b;
    b.SetInvalid();
    b.ExpandToInclude(plSimdVec4f(1, 2, 3));

    PLASMA_TEST_BOOL((b.m_Min == plSimdVec4f(1, 2, 3)).AllSet<3>());
    PLASMA_TEST_BOOL((b.m_Max == plSimdVec4f(1, 2, 3)).AllSet<3>());


    b.ExpandToInclude(plSimdVec4f(2, 3, 4));

    PLASMA_TEST_BOOL((b.m_Min == plSimdVec4f(1, 2, 3)).AllSet<3>());
    PLASMA_TEST_BOOL((b.m_Max == plSimdVec4f(2, 3, 4)).AllSet<3>());

    b.ExpandToInclude(plSimdVec4f(0, 1, 2));

    PLASMA_TEST_BOOL((b.m_Min == plSimdVec4f(0, 1, 2)).AllSet<3>());
    PLASMA_TEST_BOOL((b.m_Max == plSimdVec4f(2, 3, 4)).AllSet<3>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclude (array)")
  {
    plSimdVec4f v[4] = {plSimdVec4f(1, 1, 1), plSimdVec4f(-1, -1, -1), plSimdVec4f(2, 2, 2), plSimdVec4f(4, 4, 4)};

    plSimdBBox b;
    b.SetInvalid();
    b.ExpandToInclude(v, 2, sizeof(plSimdVec4f) * 2);

    PLASMA_TEST_BOOL((b.m_Min == plSimdVec4f(1, 1, 1)).AllSet<3>());
    PLASMA_TEST_BOOL((b.m_Max == plSimdVec4f(2, 2, 2)).AllSet<3>());

    b.ExpandToInclude(v, 4);

    PLASMA_TEST_BOOL((b.m_Min == plSimdVec4f(-1, -1, -1)).AllSet<3>());
    PLASMA_TEST_BOOL((b.m_Max == plSimdVec4f(4, 4, 4)).AllSet<3>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclude (Box)")
  {
    plSimdBBox b1(plSimdVec4f(-1, -2, -3), plSimdVec4f(1, 2, 3));
    plSimdBBox b2(plSimdVec4f(0), plSimdVec4f(4, 5, 6));

    b1.ExpandToInclude(b2);

    PLASMA_TEST_BOOL((b1.m_Min == plSimdVec4f(-1, -2, -3)).AllSet<3>());
    PLASMA_TEST_BOOL((b1.m_Max == plSimdVec4f(4, 5, 6)).AllSet<3>());

    plSimdBBox b3;
    b3.SetInvalid();
    b3.ExpandToInclude(b1);
    PLASMA_TEST_BOOL(b3 == b1);

    b2.m_Min = plSimdVec4f(-4, -5, -6);
    b2.m_Max.SetZero();

    b1.ExpandToInclude(b2);

    PLASMA_TEST_BOOL((b1.m_Min == plSimdVec4f(-4, -5, -6)).AllSet<3>());
    PLASMA_TEST_BOOL((b1.m_Max == plSimdVec4f(4, 5, 6)).AllSet<3>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToCube")
  {
    plSimdBBox b;
    b.SetCenterAndHalfExtents(plSimdVec4f(1, 2, 3), plSimdVec4f(4, 5, 6));

    b.ExpandToCube();

    PLASMA_TEST_BOOL((b.GetCenter() == plSimdVec4f(1, 2, 3)).AllSet<3>());
    PLASMA_TEST_BOOL((b.GetHalfExtents() == plSimdVec4f(6, 6, 6)).AllSet<3>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains (Point)")
  {
    plSimdBBox b(plSimdVec4f(0), plSimdVec4f(0));

    PLASMA_TEST_BOOL(b.Contains(plSimdVec4f(0)));
    PLASMA_TEST_BOOL(!b.Contains(plSimdVec4f(1, 0, 0)));
    PLASMA_TEST_BOOL(!b.Contains(plSimdVec4f(-1, 0, 0)));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains (Box)")
  {
    plSimdBBox b1(plSimdVec4f(-3), plSimdVec4f(3));
    plSimdBBox b2(plSimdVec4f(-1), plSimdVec4f(1));
    plSimdBBox b3(plSimdVec4f(-1), plSimdVec4f(4));

    PLASMA_TEST_BOOL(b1.Contains(b1));
    PLASMA_TEST_BOOL(b2.Contains(b2));
    PLASMA_TEST_BOOL(b3.Contains(b3));

    PLASMA_TEST_BOOL(b1.Contains(b2));
    PLASMA_TEST_BOOL(!b1.Contains(b3));

    PLASMA_TEST_BOOL(!b2.Contains(b1));
    PLASMA_TEST_BOOL(!b2.Contains(b3));

    PLASMA_TEST_BOOL(!b3.Contains(b1));
    PLASMA_TEST_BOOL(b3.Contains(b2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains (Sphere)")
  {
    plSimdBBox b(plSimdVec4f(1), plSimdVec4f(5));

    PLASMA_TEST_BOOL(b.Contains(plSimdBSphere(plSimdVec4f(3), 2)));
    PLASMA_TEST_BOOL(!b.Contains(plSimdBSphere(plSimdVec4f(3), 2.1f)));
    PLASMA_TEST_BOOL(!b.Contains(plSimdBSphere(plSimdVec4f(8), 2)));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Overlaps (box)")
  {
    plSimdBBox b1(plSimdVec4f(-3), plSimdVec4f(3));
    plSimdBBox b2(plSimdVec4f(-1), plSimdVec4f(1));
    plSimdBBox b3(plSimdVec4f(1), plSimdVec4f(4));
    plSimdBBox b4(plSimdVec4f(-4, 1, 1), plSimdVec4f(4, 2, 2));

    PLASMA_TEST_BOOL(b1.Overlaps(b1));
    PLASMA_TEST_BOOL(b2.Overlaps(b2));
    PLASMA_TEST_BOOL(b3.Overlaps(b3));
    PLASMA_TEST_BOOL(b4.Overlaps(b4));

    PLASMA_TEST_BOOL(b1.Overlaps(b2));
    PLASMA_TEST_BOOL(b1.Overlaps(b3));
    PLASMA_TEST_BOOL(b1.Overlaps(b4));

    PLASMA_TEST_BOOL(!b2.Overlaps(b3));
    PLASMA_TEST_BOOL(!b2.Overlaps(b4));

    PLASMA_TEST_BOOL(b3.Overlaps(b4));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Overlaps (Sphere)")
  {
    plSimdBBox b(plSimdVec4f(1), plSimdVec4f(5));

    PLASMA_TEST_BOOL(b.Overlaps(plSimdBSphere(plSimdVec4f(3), 2)));
    PLASMA_TEST_BOOL(b.Overlaps(plSimdBSphere(plSimdVec4f(3), 2.1f)));
    PLASMA_TEST_BOOL(!b.Overlaps(plSimdBSphere(plSimdVec4f(8), 2)));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Grow")
  {
    plSimdBBox b(plSimdVec4f(1, 2, 3), plSimdVec4f(4, 5, 6));
    b.Grow(plSimdVec4f(2, 4, 6));

    PLASMA_TEST_BOOL((b.m_Min == plSimdVec4f(-1, -2, -3)).AllSet<3>());
    PLASMA_TEST_BOOL((b.m_Max == plSimdVec4f(6, 9, 12)).AllSet<3>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transform")
  {
    plSimdBBox b(plSimdVec4f(3), plSimdVec4f(5));

    plSimdTransform t(plSimdVec4f(4, 5, 6));
    t.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(90));
    t.m_Scale = plSimdVec4f(1, -2, -4);

    b.Transform(t);

    PLASMA_TEST_SIMD_VECTOR_EQUAL(3, b.m_Min, plSimdVec4f(10, 8, -14), 0.00001f);
    PLASMA_TEST_SIMD_VECTOR_EQUAL(3, b.m_Max, plSimdVec4f(14, 10, -6), 0.00001f);

    t.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(-30));

    b.m_Min = plSimdVec4f(3);
    b.m_Max = plSimdVec4f(5);
    b.Transform(t);

    // reference
    plBoundingBox referenceBox(plVec3(3), plVec3(5));
    {
      plQuat q;
      q.SetFromAxisAndAngle(plVec3(0, 0, 1), plAngle::Degree(-30));

      plTransform referenceTransform(plVec3(4, 5, 6), q, plVec3(1, -2, -4));

      referenceBox.TransformFromOrigin(referenceTransform.GetAsMat4());
    }

    PLASMA_TEST_SIMD_VECTOR_EQUAL(3, b.m_Min, plSimdConversion::ToVec3(referenceBox.m_vMin), 0.00001f);
    PLASMA_TEST_SIMD_VECTOR_EQUAL(3, b.m_Max, plSimdConversion::ToVec3(referenceBox.m_vMax), 0.00001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetClampedPoint")
  {
    plSimdBBox b(plSimdVec4f(-1, -2, -3), plSimdVec4f(1, 2, 3));

    PLASMA_TEST_BOOL((b.GetClampedPoint(plSimdVec4f(-2, 0, 0)) == plSimdVec4f(-1, 0, 0)).AllSet<3>());
    PLASMA_TEST_BOOL((b.GetClampedPoint(plSimdVec4f(2, 0, 0)) == plSimdVec4f(1, 0, 0)).AllSet<3>());

    PLASMA_TEST_BOOL((b.GetClampedPoint(plSimdVec4f(0, -3, 0)) == plSimdVec4f(0, -2, 0)).AllSet<3>());
    PLASMA_TEST_BOOL((b.GetClampedPoint(plSimdVec4f(0, 3, 0)) == plSimdVec4f(0, 2, 0)).AllSet<3>());

    PLASMA_TEST_BOOL((b.GetClampedPoint(plSimdVec4f(0, 0, -4)) == plSimdVec4f(0, 0, -3)).AllSet<3>());
    PLASMA_TEST_BOOL((b.GetClampedPoint(plSimdVec4f(0, 0, 4)) == plSimdVec4f(0, 0, 3)).AllSet<3>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDistanceSquaredTo (point)")
  {
    plSimdBBox b(plSimdVec4f(-1, -2, -3), plSimdVec4f(1, 2, 3));

    PLASMA_TEST_BOOL(b.GetDistanceSquaredTo(plSimdVec4f(-2, 0, 0)) == 1.0f);
    PLASMA_TEST_BOOL(b.GetDistanceSquaredTo(plSimdVec4f(2, 0, 0)) == 1.0f);

    PLASMA_TEST_BOOL(b.GetDistanceSquaredTo(plSimdVec4f(0, -4, 0)) == 4.0f);
    PLASMA_TEST_BOOL(b.GetDistanceSquaredTo(plSimdVec4f(0, 4, 0)) == 4.0f);

    PLASMA_TEST_BOOL(b.GetDistanceSquaredTo(plSimdVec4f(0, 0, -6)) == 9.0f);
    PLASMA_TEST_BOOL(b.GetDistanceSquaredTo(plSimdVec4f(0, 0, 6)) == 9.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDistanceTo (point)")
  {
    plSimdBBox b(plSimdVec4f(-1, -2, -3), plSimdVec4f(1, 2, 3));

    PLASMA_TEST_BOOL(b.GetDistanceTo(plSimdVec4f(-2, 0, 0)) == 1.0f);
    PLASMA_TEST_BOOL(b.GetDistanceTo(plSimdVec4f(2, 0, 0)) == 1.0f);

    PLASMA_TEST_BOOL(b.GetDistanceTo(plSimdVec4f(0, -4, 0)) == 2.0f);
    PLASMA_TEST_BOOL(b.GetDistanceTo(plSimdVec4f(0, 4, 0)) == 2.0f);

    PLASMA_TEST_BOOL(b.GetDistanceTo(plSimdVec4f(0, 0, -6)) == 3.0f);
    PLASMA_TEST_BOOL(b.GetDistanceTo(plSimdVec4f(0, 0, 6)) == 3.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Comparison")
  {
    plSimdBBox b1(plSimdVec4f(5, 0, 0), plSimdVec4f(1, 2, 3));
    plSimdBBox b2(plSimdVec4f(6, 0, 0), plSimdVec4f(1, 2, 3));

    PLASMA_TEST_BOOL(b1 == plSimdBBox(plSimdVec4f(5, 0, 0), plSimdVec4f(1, 2, 3)));
    PLASMA_TEST_BOOL(b1 != b2);
  }
}

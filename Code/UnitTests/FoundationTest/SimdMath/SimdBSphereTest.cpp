#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdBSphere.h>

PLASMA_CREATE_SIMPLE_TEST(SimdMath, SimdBSphere)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plSimdBSphere s(plSimdVec4f(1, 2, 3), 4);

    PLASMA_TEST_BOOL((s.m_CenterAndRadius == plSimdVec4f(1, 2, 3, 4)).AllSet());

    PLASMA_TEST_BOOL((s.GetCenter() == plSimdVec4f(1, 2, 3)).AllSet<3>());
    PLASMA_TEST_BOOL(s.GetRadius() == 4.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetInvalid / IsValid")
  {
    plSimdBSphere s(plSimdVec4f(1, 2, 3), 4);

    PLASMA_TEST_BOOL(s.IsValid());

    s.SetInvalid();

    PLASMA_TEST_BOOL(!s.IsValid());
    PLASMA_TEST_BOOL(!s.IsNaN());

    s = plSimdBSphere(plSimdVec4f(1, 2, 3), plMath::NaN<float>());
    PLASMA_TEST_BOOL(s.IsNaN());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclude(Point)")
  {
    plSimdBSphere s(plSimdVec4f::ZeroVector(), 0.0f);

    s.ExpandToInclude(plSimdVec4f(3, 0, 0));

    PLASMA_TEST_BOOL((s.m_CenterAndRadius == plSimdVec4f(0, 0, 0, 3)).AllSet());

    s.SetInvalid();

    s.ExpandToInclude(plSimdVec4f(0.25, 0, 0));

    PLASMA_TEST_BOOL((s.m_CenterAndRadius == plSimdVec4f(0, 0, 0, 0.25)).AllSet());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclude(array)")
  {
    plSimdBSphere s(plSimdVec4f(2, 2, 0), 0.0f);

    plSimdVec4f p[4] = {plSimdVec4f(0, 2, 0), plSimdVec4f(4, 2, 0), plSimdVec4f(2, 0, 0), plSimdVec4f(2, 4, 0)};

    s.ExpandToInclude(p, 4);

    PLASMA_TEST_BOOL((s.m_CenterAndRadius == plSimdVec4f(2, 2, 0, 2)).AllSet());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclude (sphere)")
  {
    plSimdBSphere s1(plSimdVec4f(5, 0, 0), 1);
    plSimdBSphere s2(plSimdVec4f(6, 0, 0), 1);
    plSimdBSphere s3(plSimdVec4f(5, 0, 0), 2);

    s1.ExpandToInclude(s2);
    PLASMA_TEST_BOOL((s1.m_CenterAndRadius == plSimdVec4f(5, 0, 0, 2)).AllSet());

    s1.ExpandToInclude(s3);
    PLASMA_TEST_BOOL((s1.m_CenterAndRadius == plSimdVec4f(5, 0, 0, 2)).AllSet());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transform")
  {
    plSimdBSphere s(plSimdVec4f(5, 0, 0), 2);

    plSimdTransform t(plSimdVec4f(4, 5, 6));
    t.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(90));
    t.m_Scale = plSimdVec4f(1, -2, -4);

    s.Transform(t);
    PLASMA_TEST_BOOL(s.m_CenterAndRadius.IsEqual(plSimdVec4f(4, 10, 6, 8), plSimdFloat(plMath::SmallEpsilon<float>())).AllSet());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDistanceTo (point)")
  {
    plSimdBSphere s(plSimdVec4f(5, 0, 0), 2);

    PLASMA_TEST_BOOL(s.GetDistanceTo(plSimdVec4f(5, 0, 0)) == -2.0f);
    PLASMA_TEST_BOOL(s.GetDistanceTo(plSimdVec4f(7, 0, 0)) == 0.0f);
    PLASMA_TEST_BOOL(s.GetDistanceTo(plSimdVec4f(9, 0, 0)) == 2.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDistanceTo (sphere)")
  {
    plSimdBSphere s1(plSimdVec4f(5, 0, 0), 2);
    plSimdBSphere s2(plSimdVec4f(10, 0, 0), 3);
    plSimdBSphere s3(plSimdVec4f(10, 0, 0), 1);

    PLASMA_TEST_BOOL(s1.GetDistanceTo(s2) == 0.0f);
    PLASMA_TEST_BOOL(s1.GetDistanceTo(s3) == 2.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains (point)")
  {
    plSimdBSphere s(plSimdVec4f(5, 0, 0), 2.0f);

    PLASMA_TEST_BOOL(s.Contains(plSimdVec4f(3, 0, 0)));
    PLASMA_TEST_BOOL(s.Contains(plSimdVec4f(5, 0, 0)));
    PLASMA_TEST_BOOL(s.Contains(plSimdVec4f(6, 0, 0)));
    PLASMA_TEST_BOOL(s.Contains(plSimdVec4f(7, 0, 0)));

    PLASMA_TEST_BOOL(!s.Contains(plSimdVec4f(2, 0, 0)));
    PLASMA_TEST_BOOL(!s.Contains(plSimdVec4f(8, 0, 0)));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains (sphere)")
  {
    plSimdBSphere s1(plSimdVec4f(5, 0, 0), 2);
    plSimdBSphere s2(plSimdVec4f(6, 0, 0), 1);
    plSimdBSphere s3(plSimdVec4f(6, 0, 0), 2);

    PLASMA_TEST_BOOL(s1.Contains(s1));
    PLASMA_TEST_BOOL(s2.Contains(s2));
    PLASMA_TEST_BOOL(s3.Contains(s3));

    PLASMA_TEST_BOOL(s1.Contains(s2));
    PLASMA_TEST_BOOL(!s1.Contains(s3));

    PLASMA_TEST_BOOL(!s2.Contains(s3));
    PLASMA_TEST_BOOL(s3.Contains(s2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Overlaps (sphere)")
  {
    plSimdBSphere s1(plSimdVec4f(5, 0, 0), 2);
    plSimdBSphere s2(plSimdVec4f(6, 0, 0), 2);
    plSimdBSphere s3(plSimdVec4f(8, 0, 0), 1);

    PLASMA_TEST_BOOL(s1.Overlaps(s1));
    PLASMA_TEST_BOOL(s2.Overlaps(s2));
    PLASMA_TEST_BOOL(s3.Overlaps(s3));

    PLASMA_TEST_BOOL(s1.Overlaps(s2));
    PLASMA_TEST_BOOL(!s1.Overlaps(s3));

    PLASMA_TEST_BOOL(s2.Overlaps(s3));
    PLASMA_TEST_BOOL(s3.Overlaps(s2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetClampedPoint")
  {
    plSimdBSphere s(plSimdVec4f(1, 2, 3), 2.0f);

    PLASMA_TEST_BOOL(s.GetClampedPoint(plSimdVec4f(2, 2, 3)).IsEqual(plSimdVec4f(2, 2, 3), 0.001f).AllSet<3>());
    PLASMA_TEST_BOOL(s.GetClampedPoint(plSimdVec4f(5, 2, 3)).IsEqual(plSimdVec4f(3, 2, 3), 0.001f).AllSet<3>());
    PLASMA_TEST_BOOL(s.GetClampedPoint(plSimdVec4f(1, 7, 3)).IsEqual(plSimdVec4f(1, 4, 3), 0.001f).AllSet<3>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Comparison")
  {
    plSimdBSphere s1(plSimdVec4f(5, 0, 0), 2);
    plSimdBSphere s2(plSimdVec4f(6, 0, 0), 1);

    PLASMA_TEST_BOOL(s1 == plSimdBSphere(plSimdVec4f(5, 0, 0), 2));
    PLASMA_TEST_BOOL(s1 != s2);
  }
}

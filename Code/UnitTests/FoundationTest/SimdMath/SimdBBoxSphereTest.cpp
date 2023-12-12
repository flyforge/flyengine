#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>
#include <Foundation/SimdMath/SimdConversion.h>

PLASMA_CREATE_SIMPLE_TEST(SimdMath, SimdBBoxSphere)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plSimdBBoxSphere b(plSimdVec4f(-1, -2, -3), plSimdVec4f(1, 2, 3), 2);

    PLASMA_TEST_BOOL((b.m_CenterAndRadius == plSimdVec4f(-1, -2, -3, 2)).AllSet<4>());
    PLASMA_TEST_BOOL((b.m_BoxHalfExtents == plSimdVec4f(1, 2, 3)).AllSet<3>());

    plSimdBBox box(plSimdVec4f(1, 1, 1), plSimdVec4f(3, 3, 3));
    plSimdBSphere sphere(plSimdVec4f(2, 2, 2), 1);

    b = plSimdBBoxSphere(box, sphere);

    PLASMA_TEST_BOOL((b.m_CenterAndRadius == plSimdVec4f(2, 2, 2, 1)).AllSet<4>());
    PLASMA_TEST_BOOL((b.m_BoxHalfExtents == plSimdVec4f(1, 1, 1)).AllSet<3>());
    PLASMA_TEST_BOOL(b.GetBox() == box);
    PLASMA_TEST_BOOL(b.GetSphere() == sphere);

    b = plSimdBBoxSphere(box);

    PLASMA_TEST_BOOL(b.m_CenterAndRadius.IsEqual(plSimdVec4f(2, 2, 2, plMath::Sqrt(3.0f)), 0.00001f).AllSet<4>());
    PLASMA_TEST_BOOL((b.m_BoxHalfExtents == plSimdVec4f(1, 1, 1)).AllSet<3>());
    PLASMA_TEST_BOOL(b.GetBox() == box);

    b = plSimdBBoxSphere(sphere);

    PLASMA_TEST_BOOL((b.m_CenterAndRadius == plSimdVec4f(2, 2, 2, 1)).AllSet<4>());
    PLASMA_TEST_BOOL((b.m_BoxHalfExtents == plSimdVec4f(1, 1, 1)).AllSet<3>());
    PLASMA_TEST_BOOL(b.GetSphere() == sphere);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetInvalid")
  {
    plSimdBBoxSphere b;
    b.SetInvalid();

    PLASMA_TEST_BOOL(!b.IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNaN")
  {
    if (plMath::SupportsNaN<float>())
    {
      plSimdBBoxSphere b;

      b.SetInvalid();
      PLASMA_TEST_BOOL(!b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetX(plMath::NaN<float>());
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetY(plMath::NaN<float>());
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetZ(plMath::NaN<float>());
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_CenterAndRadius.SetW(plMath::NaN<float>());
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_BoxHalfExtents.SetX(plMath::NaN<float>());
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_BoxHalfExtents.SetY(plMath::NaN<float>());
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_BoxHalfExtents.SetZ(plMath::NaN<float>());
      PLASMA_TEST_BOOL(b.IsNaN());
    }
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

    plSimdBBoxSphere b;
    b.SetFromPoints(p, 6);

    PLASMA_TEST_BOOL((b.m_CenterAndRadius == plSimdVec4f(0.5, 0.5, 0.5)).AllSet<3>());
    PLASMA_TEST_BOOL((b.m_BoxHalfExtents == plSimdVec4f(4.5, 6.5, 8.5)).AllSet<3>());
    PLASMA_TEST_BOOL(b.m_CenterAndRadius.w().IsEqual(plSimdVec4f(0.5, 0.5, 8.5).GetLength<3>(), 0.00001f));
    PLASMA_TEST_BOOL(b.m_CenterAndRadius.w() <= b.m_BoxHalfExtents.GetLength<3>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclude")
  {
    plSimdBBoxSphere b1;
    b1.SetInvalid();
    plSimdBBoxSphere b2(plSimdBBox(plSimdVec4f(2, 2, 2), plSimdVec4f(4, 4, 4)));

    b1.ExpandToInclude(b2);
    PLASMA_TEST_BOOL(b1 == b2);

    plSimdBSphere sphere(plSimdVec4f(2, 2, 2), 2);
    b2 = plSimdBBoxSphere(sphere);

    b1.ExpandToInclude(b2);
    PLASMA_TEST_BOOL(b1 != b2);

    PLASMA_TEST_BOOL((b1.m_CenterAndRadius == plSimdVec4f(2, 2, 2)).AllSet<3>());
    PLASMA_TEST_BOOL((b1.m_BoxHalfExtents == plSimdVec4f(2, 2, 2)).AllSet<3>());
    PLASMA_TEST_FLOAT(b1.m_CenterAndRadius.w(), plMath::Sqrt(3.0f) * 2.0f, 0.00001f);
    PLASMA_TEST_BOOL(b1.m_CenterAndRadius.w() <= b1.m_BoxHalfExtents.GetLength<3>());

    b1.SetInvalid();
    b2 = plSimdBBox(plSimdVec4f(0.25, 0.25, 0.25), plSimdVec4f(0.5, 0.5, 0.5));

    b1.ExpandToInclude(b2);
    PLASMA_TEST_BOOL(b1 == b2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transform")
  {
    plSimdBBoxSphere b(plSimdVec4f(1), plSimdVec4f(5), 5);

    plSimdTransform t(plSimdVec4f(1, 1, 1), plSimdQuat::IdentityQuaternion(), plSimdVec4f(2, 3, -2));

    b.Transform(t);

    PLASMA_TEST_BOOL((b.m_CenterAndRadius == plSimdVec4f(3, 4, -1, 15)).AllSet<4>());
    PLASMA_TEST_BOOL((b.m_BoxHalfExtents == plSimdVec4f(10, 15, 10)).AllSet<3>());

    // verification
    plRandom rnd;
    rnd.Initialize(0x736454);

    plDynamicArray<plSimdVec4f, plAlignedAllocatorWrapper> points;
    points.SetCountUninitialized(10);
    float fSize = 10;

    for (plUInt32 i = 0; i < points.GetCount(); ++i)
    {
      float x = (float)rnd.DoubleMinMax(-fSize, fSize);
      float y = (float)rnd.DoubleMinMax(-fSize, fSize);
      float z = (float)rnd.DoubleMinMax(-fSize, fSize);
      points[i] = plSimdVec4f(x, y, z);
    }

    b.SetFromPoints(points.GetData(), points.GetCount());

    t.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(-30));
    b.Transform(t);

    for (plUInt32 i = 0; i < points.GetCount(); ++i)
    {
      plSimdVec4f tp = t.TransformPosition(points[i]);

      plSimdFloat boxDist = b.GetBox().GetDistanceTo(tp);
      PLASMA_TEST_BOOL(boxDist < plMath::DefaultEpsilon<float>());

      plSimdFloat sphereDist = b.GetSphere().GetDistanceTo(tp);
      PLASMA_TEST_BOOL(sphereDist < plMath::DefaultEpsilon<float>());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Comparison")
  {
    plSimdBBoxSphere b1(plSimdBBox(plSimdVec4f(5, 0, 0), plSimdVec4f(1, 2, 3)));
    plSimdBBoxSphere b2(plSimdBBox(plSimdVec4f(6, 0, 0), plSimdVec4f(1, 2, 3)));

    PLASMA_TEST_BOOL(b1 == plSimdBBoxSphere(plSimdBBox(plSimdVec4f(5, 0, 0), plSimdVec4f(1, 2, 3))));
    PLASMA_TEST_BOOL(b1 != b2);
  }
}

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>

PLASMA_CREATE_SIMPLE_TEST(Math, BoundingSphere)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plBoundingSphereT s(plVec3T(1, 2, 3), 4);

    PLASMA_TEST_BOOL(s.m_vCenter == plVec3T(1, 2, 3));
    PLASMA_TEST_BOOL(s.m_fRadius == 4.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetInvalid / IsValid")
  {
    plBoundingSphereT s;
    s.SetElements(plVec3T(1, 2, 3), 4);

    PLASMA_TEST_BOOL(s.IsValid());

    s.SetInvalid();

    PLASMA_TEST_BOOL(!s.IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetZero / IsZero")
  {
    plBoundingSphereT s;
    s.SetZero();

    PLASMA_TEST_BOOL(s.IsValid());
    PLASMA_TEST_BOOL(s.m_vCenter.IsZero());
    PLASMA_TEST_BOOL(s.m_fRadius == 0.0f);
    PLASMA_TEST_BOOL(s.IsZero());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetElements")
  {
    plBoundingSphereT s;
    s.SetElements(plVec3T(1, 2, 3), 4);

    PLASMA_TEST_BOOL(s.m_vCenter == plVec3T(1, 2, 3));
    PLASMA_TEST_BOOL(s.m_fRadius == 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFromPoints")
  {
    plBoundingSphereT s;

    plVec3T p[4] = {plVec3T(2, 6, 0), plVec3T(4, 2, 0), plVec3T(2, 0, 0), plVec3T(0, 4, 0)};

    s.SetFromPoints(p, 4);

    PLASMA_TEST_BOOL(s.m_vCenter == plVec3T(2, 3, 0));
    PLASMA_TEST_BOOL(s.m_fRadius == 3);

    for (int i = 0; i < PLASMA_ARRAY_SIZE(p); ++i)
    {
      PLASMA_TEST_BOOL(s.Contains(p[i]));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclude(Point)")
  {
    plBoundingSphereT s;
    s.SetZero();

    s.ExpandToInclude(plVec3T(3, 0, 0));

    PLASMA_TEST_BOOL(s.m_vCenter == plVec3T(0, 0, 0));
    PLASMA_TEST_BOOL(s.m_fRadius == 3);

    s.SetInvalid();

    s.ExpandToInclude(plVec3T(0.25, 0.0, 0.0));

    PLASMA_TEST_BOOL(s.m_vCenter == plVec3T(0, 0, 0));
    PLASMA_TEST_BOOL(s.m_fRadius == 0.25);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclude(array)")
  {
    plBoundingSphereT s;
    s.SetElements(plVec3T(2, 2, 0), 0.0f);

    plVec3T p[4] = {plVec3T(0, 2, 0), plVec3T(4, 2, 0), plVec3T(2, 0, 0), plVec3T(2, 4, 0)};

    s.ExpandToInclude(p, 4);

    PLASMA_TEST_BOOL(s.m_vCenter == plVec3T(2, 2, 0));
    PLASMA_TEST_BOOL(s.m_fRadius == 2);

    for (int i = 0; i < PLASMA_ARRAY_SIZE(p); ++i)
    {
      PLASMA_TEST_BOOL(s.Contains(p[i]));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclude (sphere)")
  {
    plBoundingSphereT s1, s2, s3;
    s1.SetElements(plVec3T(5, 0, 0), 1);
    s2.SetElements(plVec3T(6, 0, 0), 1);
    s3.SetElements(plVec3T(5, 0, 0), 2);

    s1.ExpandToInclude(s2);
    PLASMA_TEST_BOOL(s1.m_vCenter == plVec3T(5, 0, 0));
    PLASMA_TEST_BOOL(s1.m_fRadius == 2);

    s1.ExpandToInclude(s3);
    PLASMA_TEST_BOOL(s1.m_vCenter == plVec3T(5, 0, 0));
    PLASMA_TEST_BOOL(s1.m_fRadius == 2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclude (box)")
  {
    plBoundingSphereT s;
    s.SetElements(plVec3T(1, 2, 3), 1);

    plBoundingBoxT b;
    b.SetCenterAndHalfExtents(plVec3T(1, 2, 3), plVec3T(2.0f));

    s.ExpandToInclude(b);

    PLASMA_TEST_BOOL(s.m_vCenter == plVec3T(1, 2, 3));
    PLASMA_TEST_FLOAT(s.m_fRadius, plMath::Sqrt((plMathTestType)12), 0.000001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Grow")
  {
    plBoundingSphereT s;
    s.SetElements(plVec3T(1, 2, 3), 4);

    s.Grow(5);

    PLASMA_TEST_BOOL(s.m_vCenter == plVec3T(1, 2, 3));
    PLASMA_TEST_BOOL(s.m_fRadius == 9);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsIdentical, ==, !=")
  {
    plBoundingSphereT s1, s2, s3;

    s1.SetElements(plVec3T(1, 2, 3), 4);
    s2.SetElements(plVec3T(1, 2, 3), 4);
    s3.SetElements(plVec3T(1.001f, 2.001f, 3.001f), 4.001f);

    PLASMA_TEST_BOOL(s1 == s1);
    PLASMA_TEST_BOOL(s2 == s2);
    PLASMA_TEST_BOOL(s3 == s3);

    PLASMA_TEST_BOOL(s1 == s2);
    PLASMA_TEST_BOOL(s2 == s1);

    PLASMA_TEST_BOOL(s1 != s3);
    PLASMA_TEST_BOOL(s2 != s3);
    PLASMA_TEST_BOOL(s3 != s1);
    PLASMA_TEST_BOOL(s3 != s2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqual")
  {
    plBoundingSphereT s1, s2, s3;

    s1.SetElements(plVec3T(1, 2, 3), 4);
    s2.SetElements(plVec3T(1, 2, 3), 4);
    s3.SetElements(plVec3T(1.001f, 2.001f, 3.001f), 4.001f);

    PLASMA_TEST_BOOL(s1.IsEqual(s1));
    PLASMA_TEST_BOOL(s2.IsEqual(s2));
    PLASMA_TEST_BOOL(s3.IsEqual(s3));

    PLASMA_TEST_BOOL(s1.IsEqual(s2));
    PLASMA_TEST_BOOL(s2.IsEqual(s1));

    PLASMA_TEST_BOOL(!s1.IsEqual(s3, 0.0001f));
    PLASMA_TEST_BOOL(!s2.IsEqual(s3, 0.0001f));
    PLASMA_TEST_BOOL(!s3.IsEqual(s1, 0.0001f));
    PLASMA_TEST_BOOL(!s3.IsEqual(s2, 0.0001f));

    PLASMA_TEST_BOOL(s1.IsEqual(s3, 0.002f));
    PLASMA_TEST_BOOL(s2.IsEqual(s3, 0.002f));
    PLASMA_TEST_BOOL(s3.IsEqual(s1, 0.002f));
    PLASMA_TEST_BOOL(s3.IsEqual(s2, 0.002f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Translate")
  {
    plBoundingSphereT s;
    s.SetElements(plVec3T(1, 2, 3), 4);

    s.Translate(plVec3T(4, 5, 6));

    PLASMA_TEST_BOOL(s.m_vCenter == plVec3T(5, 7, 9));
    PLASMA_TEST_BOOL(s.m_fRadius == 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ScaleFromCenter")
  {
    plBoundingSphereT s;
    s.SetElements(plVec3T(1, 2, 3), 4);

    s.ScaleFromCenter(5.0f);

    PLASMA_TEST_BOOL(s.m_vCenter == plVec3T(1, 2, 3));
    PLASMA_TEST_BOOL(s.m_fRadius == 20);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ScaleFromOrigin")
  {
    plBoundingSphereT s;
    s.SetElements(plVec3T(1, 2, 3), 4);

    s.ScaleFromOrigin(plVec3T(2, 3, 4));

    PLASMA_TEST_BOOL(s.m_vCenter == plVec3T(2, 6, 12));
    PLASMA_TEST_BOOL(s.m_fRadius == 16);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDistanceTo (point)")
  {
    plBoundingSphereT s;
    s.SetElements(plVec3T(5, 0, 0), 2);

    PLASMA_TEST_BOOL(s.GetDistanceTo(plVec3T(5, 0, 0)) == -2.0f);
    PLASMA_TEST_BOOL(s.GetDistanceTo(plVec3T(7, 0, 0)) == 0.0f);
    PLASMA_TEST_BOOL(s.GetDistanceTo(plVec3T(9, 0, 0)) == 2.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDistanceTo (sphere)")
  {
    plBoundingSphereT s1, s2, s3;
    s1.SetElements(plVec3T(5, 0, 0), 2);
    s2.SetElements(plVec3T(10, 0, 0), 3);
    s3.SetElements(plVec3T(10, 0, 0), 1);

    PLASMA_TEST_BOOL(s1.GetDistanceTo(s2) == 0.0f);
    PLASMA_TEST_BOOL(s1.GetDistanceTo(s3) == 2.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDistanceTo (array)")
  {
    plBoundingSphereT s;
    s.SetElements(plVec3T(0.0f), 0.0f);

    plVec3T p[4] = {
      plVec3T(5),
      plVec3T(10),
      plVec3T(15),
      plVec3T(7),
    };

    PLASMA_TEST_FLOAT(s.GetDistanceTo(p, 4), plVec3T(5).GetLength(), 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains (point)")
  {
    plBoundingSphereT s;
    s.SetElements(plVec3T(5, 0, 0), 2.0f);

    PLASMA_TEST_BOOL(s.Contains(plVec3T(3, 0, 0)));
    PLASMA_TEST_BOOL(s.Contains(plVec3T(5, 0, 0)));
    PLASMA_TEST_BOOL(s.Contains(plVec3T(6, 0, 0)));
    PLASMA_TEST_BOOL(s.Contains(plVec3T(7, 0, 0)));

    PLASMA_TEST_BOOL(!s.Contains(plVec3T(2, 0, 0)));
    PLASMA_TEST_BOOL(!s.Contains(plVec3T(8, 0, 0)));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains (array)")
  {
    plBoundingSphereT s(plVec3T(0.0f), 6.0f);

    plVec3T p[4] = {
      plVec3T(3),
      plVec3T(10),
      plVec3T(2),
      plVec3T(7),
    };

    PLASMA_TEST_BOOL(s.Contains(p, 2, sizeof(plVec3T) * 2));
    PLASMA_TEST_BOOL(!s.Contains(p + 1, 2, sizeof(plVec3T) * 2));
    PLASMA_TEST_BOOL(!s.Contains(p, 4, sizeof(plVec3T)));
  }

  // Disabled because MSVC 2017 has code generation issues in Release builds
  PLASMA_TEST_BLOCK(plTestBlock::Disabled, "Contains (sphere)")
  {
    plBoundingSphereT s1, s2, s3;
    s1.SetElements(plVec3T(5, 0, 0), 2);
    s2.SetElements(plVec3T(6, 0, 0), 1);
    s3.SetElements(plVec3T(6, 0, 0), 2);

    PLASMA_TEST_BOOL(s1.Contains(s1));
    PLASMA_TEST_BOOL(s2.Contains(s2));
    PLASMA_TEST_BOOL(s3.Contains(s3));

    PLASMA_TEST_BOOL(s1.Contains(s2));
    PLASMA_TEST_BOOL(!s1.Contains(s3));

    PLASMA_TEST_BOOL(!s2.Contains(s3));
    PLASMA_TEST_BOOL(s3.Contains(s2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains (box)")
  {
    plBoundingSphereT s(plVec3T(1, 2, 3), 4);
    plBoundingBoxT b1(plVec3T(1, 2, 3) - plVec3T(1), plVec3T(1, 2, 3) + plVec3T(1));
    plBoundingBoxT b2(plVec3T(1, 2, 3) - plVec3T(1), plVec3T(1, 2, 3) + plVec3T(3));

    PLASMA_TEST_BOOL(s.Contains(b1));
    PLASMA_TEST_BOOL(!s.Contains(b2));

    plVec3T vDir(1, 1, 1);
    vDir.SetLength(3.99f).IgnoreResult();
    plBoundingBoxT b3(plVec3T(1, 2, 3) - plVec3T(1), plVec3T(1, 2, 3) + vDir);

    PLASMA_TEST_BOOL(s.Contains(b3));

    vDir.SetLength(4.01f).IgnoreResult();
    plBoundingBoxT b4(plVec3T(1, 2, 3) - plVec3T(1), plVec3T(1, 2, 3) + vDir);

    PLASMA_TEST_BOOL(!s.Contains(b4));
  }


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Overlaps (array)")
  {
    plBoundingSphereT s(plVec3T(0.0f), 6.0f);

    plVec3T p[4] = {
      plVec3T(3),
      plVec3T(10),
      plVec3T(2),
      plVec3T(7),
    };

    PLASMA_TEST_BOOL(s.Overlaps(p, 2, sizeof(plVec3T) * 2));
    PLASMA_TEST_BOOL(!s.Overlaps(p + 1, 2, sizeof(plVec3T) * 2));
    PLASMA_TEST_BOOL(s.Overlaps(p, 4, sizeof(plVec3T)));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Overlaps (sphere)")
  {
    plBoundingSphereT s1, s2, s3;
    s1.SetElements(plVec3T(5, 0, 0), 2);
    s2.SetElements(plVec3T(6, 0, 0), 2);
    s3.SetElements(plVec3T(8, 0, 0), 1);

    PLASMA_TEST_BOOL(s1.Overlaps(s1));
    PLASMA_TEST_BOOL(s2.Overlaps(s2));
    PLASMA_TEST_BOOL(s3.Overlaps(s3));

    PLASMA_TEST_BOOL(s1.Overlaps(s2));
    PLASMA_TEST_BOOL(!s1.Overlaps(s3));

    PLASMA_TEST_BOOL(s2.Overlaps(s3));
    PLASMA_TEST_BOOL(s3.Overlaps(s2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Overlaps (box)")
  {
    plBoundingSphereT s(plVec3T(1, 2, 3), 2);
    plBoundingBoxT b1(plVec3T(1, 2, 3), plVec3T(1, 2, 3) + plVec3T(2));
    plBoundingBoxT b2(plVec3T(1, 2, 3) + plVec3T(2), plVec3T(1, 2, 3) + plVec3T(3));

    PLASMA_TEST_BOOL(s.Overlaps(b1));
    PLASMA_TEST_BOOL(!s.Overlaps(b2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetBoundingBox")
  {
    plBoundingSphereT s;
    s.SetElements(plVec3T(1, 2, 3), 2.0f);

    plBoundingBoxT b = s.GetBoundingBox();

    PLASMA_TEST_BOOL(b.m_vMin == plVec3T(-1, 0, 1));
    PLASMA_TEST_BOOL(b.m_vMax == plVec3T(3, 4, 5));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetClampedPoint")
  {
    plBoundingSphereT s(plVec3T(1, 2, 3), 2.0f);

    PLASMA_TEST_VEC3(s.GetClampedPoint(plVec3T(2, 2, 3)), plVec3T(2, 2, 3), 0.001);
    PLASMA_TEST_VEC3(s.GetClampedPoint(plVec3T(5, 2, 3)), plVec3T(3, 2, 3), 0.001);
    PLASMA_TEST_VEC3(s.GetClampedPoint(plVec3T(1, 7, 3)), plVec3T(1, 4, 3), 0.001);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetRayIntersection")
  {
    plBoundingSphereT s(plVec3T(1, 2, 3), 4);

    for (plUInt32 i = 0; i < 10000; ++i)
    {
      const plVec3T vDir =
        plVec3T(plMath::Sin(plAngle::Degree(i * 1.0f)), plMath::Cos(plAngle::Degree(i * 3.0f)), plMath::Cos(plAngle::Degree(i * 1.0f)))
          .GetNormalized();
      const plVec3T vTarget = vDir * s.m_fRadius + s.m_vCenter;
      const plVec3T vSource = vTarget + vDir * (plMathTestType)5;

      PLASMA_TEST_FLOAT((vSource - vTarget).GetLength(), 5.0f, 0.001f);

      plMathTestType fIntersection;
      plVec3T vIntersection;
      PLASMA_TEST_BOOL(s.GetRayIntersection(vSource, -vDir, &fIntersection, &vIntersection) == true);
      PLASMA_TEST_FLOAT(fIntersection, (vSource - vTarget).GetLength(), 0.0001f);
      PLASMA_TEST_BOOL(vIntersection.IsEqual(vTarget, 0.0001f));

      PLASMA_TEST_BOOL(s.GetRayIntersection(vSource, vDir, &fIntersection, &vIntersection) == false);

      PLASMA_TEST_BOOL(s.GetRayIntersection(vTarget - vDir, vDir, &fIntersection, &vIntersection) == true);
      PLASMA_TEST_FLOAT(fIntersection, 1, 0.0001f);
      PLASMA_TEST_BOOL(vIntersection.IsEqual(vTarget, 0.0001f));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLineSegmentIntersection")
  {
    plBoundingSphereT s(plVec3T(1, 2, 3), 4);

    for (plUInt32 i = 0; i < 10000; ++i)
    {
      const plVec3T vDir = plVec3T(plMath::Sin(plAngle::Degree(i * (plMathTestType)1)), plMath::Cos(plAngle::Degree(i * (plMathTestType)3)),
        plMath::Cos(plAngle::Degree(i * (plMathTestType)1)))
                             .GetNormalized();
      const plVec3T vTarget = vDir * s.m_fRadius + s.m_vCenter - vDir;
      const plVec3T vSource = vTarget + vDir * (plMathTestType)5;

      plMathTestType fIntersection;
      plVec3T vIntersection;
      PLASMA_TEST_BOOL(s.GetLineSegmentIntersection(vSource, vTarget, &fIntersection, &vIntersection) == true);
      PLASMA_TEST_FLOAT(fIntersection, 4.0f / 5.0f, 0.0001f);
      PLASMA_TEST_BOOL(vIntersection.IsEqual(vTarget + vDir, 0.0001f));

      PLASMA_TEST_BOOL(s.GetLineSegmentIntersection(vTarget, vSource, &fIntersection, &vIntersection) == true);
      PLASMA_TEST_FLOAT(fIntersection, 1.0f / 5.0f, 0.0001f);
      PLASMA_TEST_BOOL(vIntersection.IsEqual(vTarget + vDir, 0.0001f));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TransformFromOrigin")
  {
    plBoundingSphereT s(plVec3T(1, 2, 3), 4);
    plMat4T mTransform;

    mTransform.SetTranslationMatrix(plVec3T(5, 6, 7));
    mTransform.SetScalingFactors(plVec3T(4, 3, 2)).IgnoreResult();

    s.TransformFromOrigin(mTransform);

    PLASMA_TEST_BOOL(s.m_vCenter == plVec3T(9, 12, 13));
    PLASMA_TEST_BOOL(s.m_fRadius == 16);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TransformFromCenter")
  {
    plBoundingSphereT s(plVec3T(1, 2, 3), 4);
    plMat4T mTransform;

    mTransform.SetTranslationMatrix(plVec3T(5, 6, 7));
    mTransform.SetScalingFactors(plVec3T(4, 3, 2)).IgnoreResult();

    s.TransformFromCenter(mTransform);

    PLASMA_TEST_BOOL(s.m_vCenter == plVec3T(6, 8, 10));
    PLASMA_TEST_BOOL(s.m_fRadius == 16);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNaN")
  {
    if (plMath::SupportsNaN<plMathTestType>())
    {
      plBoundingSphereT s;

      s.SetInvalid();
      PLASMA_TEST_BOOL(!s.IsNaN());

      s.SetInvalid();
      s.m_fRadius = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(s.IsNaN());

      s.SetInvalid();
      s.m_vCenter.x = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(s.IsNaN());

      s.SetInvalid();
      s.m_vCenter.y = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(s.IsNaN());

      s.SetInvalid();
      s.m_vCenter.z = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(s.IsNaN());
    }
  }
}

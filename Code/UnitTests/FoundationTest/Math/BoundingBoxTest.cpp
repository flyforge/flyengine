#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>

PLASMA_CREATE_SIMPLE_TEST(Math, BoundingBox)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor(SetElements)")
  {
    plBoundingBoxT b(plVec3T(-1, -2, -3), plVec3T(1, 2, 3));

    PLASMA_TEST_BOOL(b.m_vMin == plVec3T(-1, -2, -3));
    PLASMA_TEST_BOOL(b.m_vMax == plVec3T(1, 2, 3));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetElements")
  {
    plBoundingBoxT b;
    b.SetElements(plVec3T(-1, -2, -3), plVec3T(1, 2, 3));

    PLASMA_TEST_BOOL(b.m_vMin == plVec3T(-1, -2, -3));
    PLASMA_TEST_BOOL(b.m_vMax == plVec3T(1, 2, 3));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFromPoints")
  {
    plVec3T p[6] = {
      plVec3T(-4, 0, 0),
      plVec3T(5, 0, 0),
      plVec3T(0, -6, 0),
      plVec3T(0, 7, 0),
      plVec3T(0, 0, -8),
      plVec3T(0, 0, 9),
    };

    plBoundingBoxT b;
    b.SetFromPoints(p, 6);

    PLASMA_TEST_BOOL(b.m_vMin == plVec3T(-4, -6, -8));
    PLASMA_TEST_BOOL(b.m_vMax == plVec3T(5, 7, 9));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetInvalid")
  {
    plBoundingBoxT b;
    b.SetInvalid();

    PLASMA_TEST_BOOL(!b.IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetCenterAndHalfExtents")
  {
    plBoundingBoxT b;
    b.SetCenterAndHalfExtents(plVec3T(1, 2, 3), plVec3T(4, 5, 6));

    PLASMA_TEST_BOOL(b.m_vMin == plVec3T(-3, -3, -3));
    PLASMA_TEST_BOOL(b.m_vMax == plVec3T(5, 7, 9));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetCorners")
  {
    plBoundingBoxT b;
    b.SetElements(plVec3T(-1, -2, -3), plVec3T(1, 2, 3));

    plVec3T c[8];
    b.GetCorners(c);

    PLASMA_TEST_BOOL(c[0] == plVec3T(-1, -2, -3));
    PLASMA_TEST_BOOL(c[1] == plVec3T(-1, -2, 3));
    PLASMA_TEST_BOOL(c[2] == plVec3T(-1, 2, -3));
    PLASMA_TEST_BOOL(c[3] == plVec3T(-1, 2, 3));
    PLASMA_TEST_BOOL(c[4] == plVec3T(1, -2, -3));
    PLASMA_TEST_BOOL(c[5] == plVec3T(1, -2, 3));
    PLASMA_TEST_BOOL(c[6] == plVec3T(1, 2, -3));
    PLASMA_TEST_BOOL(c[7] == plVec3T(1, 2, 3));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclue (Point)")
  {
    plBoundingBoxT b;
    b.SetInvalid();
    b.ExpandToInclude(plVec3T(1, 2, 3));

    PLASMA_TEST_BOOL(b.m_vMin == plVec3T(1, 2, 3));
    PLASMA_TEST_BOOL(b.m_vMax == plVec3T(1, 2, 3));


    b.ExpandToInclude(plVec3T(2, 3, 4));

    PLASMA_TEST_BOOL(b.m_vMin == plVec3T(1, 2, 3));
    PLASMA_TEST_BOOL(b.m_vMax == plVec3T(2, 3, 4));

    b.ExpandToInclude(plVec3T(0, 1, 2));

    PLASMA_TEST_BOOL(b.m_vMin == plVec3T(0, 1, 2));
    PLASMA_TEST_BOOL(b.m_vMax == plVec3T(2, 3, 4));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclude (Box)")
  {
    plBoundingBoxT b1, b2;

    b1.SetElements(plVec3T(-1, -2, -3), plVec3T(1, 2, 3));
    b2.SetElements(plVec3T(0), plVec3T(4, 5, 6));

    b1.ExpandToInclude(b2);

    PLASMA_TEST_BOOL(b1.m_vMin == plVec3T(-1, -2, -3));
    PLASMA_TEST_BOOL(b1.m_vMax == plVec3T(4, 5, 6));

    b2.SetElements(plVec3T(-4, -5, -6), plVec3T(0));

    b1.ExpandToInclude(b2);

    PLASMA_TEST_BOOL(b1.m_vMin == plVec3T(-4, -5, -6));
    PLASMA_TEST_BOOL(b1.m_vMax == plVec3T(4, 5, 6));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclude (array)")
  {
    plVec3T v[4] = {plVec3T(1, 1, 1), plVec3T(-1, -1, -1), plVec3T(2, 2, 2), plVec3T(4, 4, 4)};

    plBoundingBoxT b;
    b.SetInvalid();
    b.ExpandToInclude(v, 2, sizeof(plVec3T) * 2);

    PLASMA_TEST_BOOL(b.m_vMin == plVec3T(1, 1, 1));
    PLASMA_TEST_BOOL(b.m_vMax == plVec3T(2, 2, 2));

    b.ExpandToInclude(v, 4, sizeof(plVec3T));

    PLASMA_TEST_BOOL(b.m_vMin == plVec3T(-1, -1, -1));
    PLASMA_TEST_BOOL(b.m_vMax == plVec3T(4, 4, 4));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToCube")
  {
    plBoundingBoxT b;
    b.SetCenterAndHalfExtents(plVec3T(1, 2, 3), plVec3T(4, 5, 6));

    b.ExpandToCube();

    PLASMA_TEST_VEC3(b.GetCenter(), plVec3T(1, 2, 3), plMath::DefaultEpsilon<plMathTestType>());
    PLASMA_TEST_VEC3(b.GetHalfExtents(), plVec3T(6, 6, 6), plMath::DefaultEpsilon<plMathTestType>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Grow")
  {
    plBoundingBoxT b(plVec3T(1, 2, 3), plVec3T(4, 5, 6));
    b.Grow(plVec3T(2, 4, 6));

    PLASMA_TEST_BOOL(b.m_vMin == plVec3T(-1, -2, -3));
    PLASMA_TEST_BOOL(b.m_vMax == plVec3T(6, 9, 12));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains (Point)")
  {
    plBoundingBoxT b(plVec3T(0), plVec3T(0));

    PLASMA_TEST_BOOL(b.Contains(plVec3T(0)));
    PLASMA_TEST_BOOL(!b.Contains(plVec3T(1, 0, 0)));
    PLASMA_TEST_BOOL(!b.Contains(plVec3T(-1, 0, 0)));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains (Box)")
  {
    plBoundingBoxT b1(plVec3T(-3), plVec3T(3));
    plBoundingBoxT b2(plVec3T(-1), plVec3T(1));
    plBoundingBoxT b3(plVec3T(-1), plVec3T(4));

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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains (Array)")
  {
    plBoundingBoxT b(plVec3T(1), plVec3T(5));

    plVec3T v[4] = {plVec3T(0), plVec3T(1), plVec3T(5), plVec3T(6)};

    PLASMA_TEST_BOOL(!b.Contains(&v[0], 4, sizeof(plVec3T)));
    PLASMA_TEST_BOOL(b.Contains(&v[1], 2, sizeof(plVec3T)));
    PLASMA_TEST_BOOL(b.Contains(&v[2], 1, sizeof(plVec3T)));

    PLASMA_TEST_BOOL(!b.Contains(&v[1], 2, sizeof(plVec3T) * 2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains (Sphere)")
  {
    plBoundingBoxT b(plVec3T(1), plVec3T(5));

    PLASMA_TEST_BOOL(b.Contains(plBoundingSphereT(plVec3T(3), 2)));
    PLASMA_TEST_BOOL(!b.Contains(plBoundingSphereT(plVec3T(3), 2.1f)));
    PLASMA_TEST_BOOL(!b.Contains(plBoundingSphereT(plVec3T(8), 2)));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Overlaps (box)")
  {
    plBoundingBoxT b1(plVec3T(-3), plVec3T(3));
    plBoundingBoxT b2(plVec3T(-1), plVec3T(1));
    plBoundingBoxT b3(plVec3T(1), plVec3T(4));
    plBoundingBoxT b4(plVec3T(-4, 1, 1), plVec3T(4, 2, 2));

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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Overlaps (Array)")
  {
    plBoundingBoxT b(plVec3T(1), plVec3T(5));

    plVec3T v[4] = {plVec3T(0), plVec3T(1), plVec3T(5), plVec3T(6)};

    PLASMA_TEST_BOOL(!b.Overlaps(&v[0], 1, sizeof(plVec3T)));
    PLASMA_TEST_BOOL(!b.Overlaps(&v[3], 1, sizeof(plVec3T)));

    PLASMA_TEST_BOOL(b.Overlaps(&v[0], 4, sizeof(plVec3T)));
    PLASMA_TEST_BOOL(b.Overlaps(&v[1], 2, sizeof(plVec3T)));
    PLASMA_TEST_BOOL(b.Overlaps(&v[2], 1, sizeof(plVec3T)));

    PLASMA_TEST_BOOL(b.Overlaps(&v[1], 2, sizeof(plVec3T) * 2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Overlaps (Sphere)")
  {
    plBoundingBoxT b(plVec3T(1), plVec3T(5));

    PLASMA_TEST_BOOL(b.Overlaps(plBoundingSphereT(plVec3T(3), 2)));
    PLASMA_TEST_BOOL(b.Overlaps(plBoundingSphereT(plVec3T(3), 2.1f)));
    PLASMA_TEST_BOOL(!b.Overlaps(plBoundingSphereT(plVec3T(8), 2)));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsIdentical, ==, !=")
  {
    plBoundingBoxT b1, b2, b3;

    b1.SetElements(plVec3T(1), plVec3T(2));
    b2.SetElements(plVec3T(1), plVec3T(2));
    b3.SetElements(plVec3T(1), plVec3T(2.01f));

    PLASMA_TEST_BOOL(b1.IsIdentical(b1));
    PLASMA_TEST_BOOL(b2.IsIdentical(b2));
    PLASMA_TEST_BOOL(b3.IsIdentical(b3));

    PLASMA_TEST_BOOL(b1 == b1);
    PLASMA_TEST_BOOL(b2 == b2);
    PLASMA_TEST_BOOL(b3 == b3);

    PLASMA_TEST_BOOL(b1.IsIdentical(b2));
    PLASMA_TEST_BOOL(b2.IsIdentical(b1));

    PLASMA_TEST_BOOL(!b1.IsIdentical(b3));
    PLASMA_TEST_BOOL(!b2.IsIdentical(b3));
    PLASMA_TEST_BOOL(!b3.IsIdentical(b1));
    PLASMA_TEST_BOOL(!b3.IsIdentical(b1));

    PLASMA_TEST_BOOL(b1 == b2);
    PLASMA_TEST_BOOL(b2 == b1);

    PLASMA_TEST_BOOL(b1 != b3);
    PLASMA_TEST_BOOL(b2 != b3);
    PLASMA_TEST_BOOL(b3 != b1);
    PLASMA_TEST_BOOL(b3 != b1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqual")
  {
    plBoundingBoxT b1, b2;
    b1.SetElements(plVec3T(-1), plVec3T(1));
    b2.SetElements(plVec3T(-1), plVec3T(2));

    PLASMA_TEST_BOOL(!b1.IsEqual(b2));
    PLASMA_TEST_BOOL(!b1.IsEqual(b2, 0.5f));
    PLASMA_TEST_BOOL(b1.IsEqual(b2, 1));
    PLASMA_TEST_BOOL(b1.IsEqual(b2, 2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetCenter")
  {
    plBoundingBoxT b(plVec3T(3), plVec3T(7));

    PLASMA_TEST_BOOL(b.GetCenter() == plVec3T(5));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetExtents")
  {
    plBoundingBoxT b(plVec3T(3), plVec3T(7));

    PLASMA_TEST_BOOL(b.GetExtents() == plVec3T(4));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetHalfExtents")
  {
    plBoundingBoxT b(plVec3T(3), plVec3T(7));

    PLASMA_TEST_BOOL(b.GetHalfExtents() == plVec3T(2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Translate")
  {
    plBoundingBoxT b(plVec3T(3), plVec3T(5));

    b.Translate(plVec3T(1, 2, 3));

    PLASMA_TEST_BOOL(b.m_vMin == plVec3T(4, 5, 6));
    PLASMA_TEST_BOOL(b.m_vMax == plVec3T(6, 7, 8));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ScaleFromCenter")
  {
    {
      plBoundingBoxT b(plVec3T(3), plVec3T(5));

      b.ScaleFromCenter(plVec3T(1, 2, 3));

      PLASMA_TEST_BOOL(b.m_vMin == plVec3T(3, 2, 1));
      PLASMA_TEST_BOOL(b.m_vMax == plVec3T(5, 6, 7));
    }
    {
      plBoundingBoxT b(plVec3T(3), plVec3T(5));

      b.ScaleFromCenter(plVec3T(-1, -2, -3));

      PLASMA_TEST_BOOL(b.m_vMin == plVec3T(3, 2, 1));
      PLASMA_TEST_BOOL(b.m_vMax == plVec3T(5, 6, 7));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ScaleFromOrigin")
  {
    {
      plBoundingBoxT b(plVec3T(3), plVec3T(5));

      b.ScaleFromOrigin(plVec3T(1, 2, 3));

      PLASMA_TEST_BOOL(b.m_vMin == plVec3T(3, 6, 9));
      PLASMA_TEST_BOOL(b.m_vMax == plVec3T(5, 10, 15));
    }
    {
      plBoundingBoxT b(plVec3T(3), plVec3T(5));

      b.ScaleFromOrigin(plVec3T(-1, -2, -3));

      PLASMA_TEST_BOOL(b.m_vMin == plVec3T(-5, -10, -15));
      PLASMA_TEST_BOOL(b.m_vMax == plVec3T(-3, -6, -9));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TransformFromOrigin")
  {
    plBoundingBoxT b(plVec3T(3), plVec3T(5));

    plMat4T m;
    m.SetScalingMatrix(plVec3T(2));

    b.TransformFromOrigin(m);

    PLASMA_TEST_BOOL(b.m_vMin == plVec3T(6, 6, 6));
    PLASMA_TEST_BOOL(b.m_vMax == plVec3T(10, 10, 10));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TransformFromCenter")
  {
    plBoundingBoxT b(plVec3T(3), plVec3T(5));

    plMat4T m;
    m.SetScalingMatrix(plVec3T(2));

    b.TransformFromCenter(m);

    PLASMA_TEST_BOOL(b.m_vMin == plVec3T(2, 2, 2));
    PLASMA_TEST_BOOL(b.m_vMax == plVec3T(6, 6, 6));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetClampedPoint")
  {
    plBoundingBoxT b(plVec3T(-1, -2, -3), plVec3T(1, 2, 3));

    PLASMA_TEST_BOOL(b.GetClampedPoint(plVec3T(-2, 0, 0)) == plVec3T(-1, 0, 0));
    PLASMA_TEST_BOOL(b.GetClampedPoint(plVec3T(2, 0, 0)) == plVec3T(1, 0, 0));

    PLASMA_TEST_BOOL(b.GetClampedPoint(plVec3T(0, -3, 0)) == plVec3T(0, -2, 0));
    PLASMA_TEST_BOOL(b.GetClampedPoint(plVec3T(0, 3, 0)) == plVec3T(0, 2, 0));

    PLASMA_TEST_BOOL(b.GetClampedPoint(plVec3T(0, 0, -4)) == plVec3T(0, 0, -3));
    PLASMA_TEST_BOOL(b.GetClampedPoint(plVec3T(0, 0, 4)) == plVec3T(0, 0, 3));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDistanceTo (point)")
  {
    plBoundingBoxT b(plVec3T(-1, -2, -3), plVec3T(1, 2, 3));

    PLASMA_TEST_BOOL(b.GetDistanceTo(plVec3T(-2, 0, 0)) == 1);
    PLASMA_TEST_BOOL(b.GetDistanceTo(plVec3T(2, 0, 0)) == 1);

    PLASMA_TEST_BOOL(b.GetDistanceTo(plVec3T(0, -4, 0)) == 2);
    PLASMA_TEST_BOOL(b.GetDistanceTo(plVec3T(0, 4, 0)) == 2);

    PLASMA_TEST_BOOL(b.GetDistanceTo(plVec3T(0, 0, -6)) == 3);
    PLASMA_TEST_BOOL(b.GetDistanceTo(plVec3T(0, 0, 6)) == 3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDistanceTo (Sphere)")
  {
    plBoundingBoxT b(plVec3T(1), plVec3T(5));

    PLASMA_TEST_BOOL(b.GetDistanceTo(plBoundingSphereT(plVec3T(3), 2)) < 0);
    PLASMA_TEST_BOOL(b.GetDistanceTo(plBoundingSphereT(plVec3T(5), 1)) < 0);
    PLASMA_TEST_FLOAT(b.GetDistanceTo(plBoundingSphereT(plVec3T(8, 2, 2), 2)), 1, 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDistanceTo (box)")
  {
    plBoundingBoxT b(plVec3T(1), plVec3T(5));

    plBoundingBoxT b1, b2, b3;
    b1.SetCenterAndHalfExtents(plVec3T(3), plVec3T(2));
    b2.SetCenterAndHalfExtents(plVec3T(5), plVec3T(1));
    b3.SetCenterAndHalfExtents(plVec3T(9, 2, 2), plVec3T(2));

    PLASMA_TEST_BOOL(b.GetDistanceTo(b1) <= 0);
    PLASMA_TEST_BOOL(b.GetDistanceTo(b2) <= 0);
    PLASMA_TEST_FLOAT(b.GetDistanceTo(b3), 2, 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDistanceSquaredTo (point)")
  {
    plBoundingBoxT b(plVec3T(-1, -2, -3), plVec3T(1, 2, 3));

    PLASMA_TEST_BOOL(b.GetDistanceSquaredTo(plVec3T(-2, 0, 0)) == 1);
    PLASMA_TEST_BOOL(b.GetDistanceSquaredTo(plVec3T(2, 0, 0)) == 1);

    PLASMA_TEST_BOOL(b.GetDistanceSquaredTo(plVec3T(0, -4, 0)) == 4);
    PLASMA_TEST_BOOL(b.GetDistanceSquaredTo(plVec3T(0, 4, 0)) == 4);

    PLASMA_TEST_BOOL(b.GetDistanceSquaredTo(plVec3T(0, 0, -6)) == 9);
    PLASMA_TEST_BOOL(b.GetDistanceSquaredTo(plVec3T(0, 0, 6)) == 9);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDistanceSquaredTo (box)")
  {
    plBoundingBoxT b(plVec3T(1), plVec3T(5));

    plBoundingBoxT b1, b2, b3;
    b1.SetCenterAndHalfExtents(plVec3T(3), plVec3T(2));
    b2.SetCenterAndHalfExtents(plVec3T(5), plVec3T(1));
    b3.SetCenterAndHalfExtents(plVec3T(9, 2, 2), plVec3T(2));

    PLASMA_TEST_BOOL(b.GetDistanceSquaredTo(b1) <= 0);
    PLASMA_TEST_BOOL(b.GetDistanceSquaredTo(b2) <= 0);
    PLASMA_TEST_FLOAT(b.GetDistanceSquaredTo(b3), 4, 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetBoundingSphere")
  {
    plBoundingBoxT b;
    b.SetCenterAndHalfExtents(plVec3T(5, 4, 2), plVec3T(3));

    plBoundingSphereT s = b.GetBoundingSphere();

    PLASMA_TEST_BOOL(s.m_vCenter == plVec3T(5, 4, 2));
    PLASMA_TEST_FLOAT(s.m_fRadius, plVec3T(3).GetLength(), 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetRayIntersection")
  {
    if (plMath::SupportsInfinity<plMathTestType>())
    {
      const plVec3T c = plVec3T(10);

      plBoundingBoxT b;
      b.SetCenterAndHalfExtents(c, plVec3T(2, 4, 8));

      for (plMathTestType x = b.m_vMin.x - (plMathTestType)1; x < b.m_vMax.x + (plMathTestType)1; x += (plMathTestType)0.2f)
      {
        for (plMathTestType y = b.m_vMin.y - (plMathTestType)1; y < b.m_vMax.y + (plMathTestType)1; y += (plMathTestType)0.2f)
        {
          for (plMathTestType z = b.m_vMin.z - (plMathTestType)1; z < b.m_vMax.z + (plMathTestType)1; z += (plMathTestType)0.2f)
          {
            const plVec3T v(x, y, z);

            if (b.Contains(v))
              continue;

            const plVec3T vTarget = b.GetClampedPoint(v);

            const plVec3T vDir = (vTarget - c).GetNormalized();

            const plVec3T vSource = vTarget + vDir * (plMathTestType)3;

            plMathTestType f;
            plVec3T vi;
            PLASMA_TEST_BOOL(b.GetRayIntersection(vSource, -vDir, &f, &vi) == true);
            PLASMA_TEST_FLOAT(f, 3, 0.001f);
            PLASMA_TEST_BOOL(vi.IsEqual(vTarget, 0.0001f));

            PLASMA_TEST_BOOL(b.GetRayIntersection(vSource, vDir, &f, &vi) == false);
            PLASMA_TEST_BOOL(b.GetRayIntersection(vTarget, vDir, &f, &vi) == false);
          }
        }
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLineSegmentIntersection")
  {
    if (plMath::SupportsInfinity<plMathTestType>())
    {
      const plVec3T c = plVec3T(10);

      plBoundingBoxT b;
      b.SetCenterAndHalfExtents(c, plVec3T(2, 4, 8));

      for (plMathTestType x = b.m_vMin.x - (plMathTestType)1; x < b.m_vMax.x + (plMathTestType)1; x += (plMathTestType)0.2f)
      {
        for (plMathTestType y = b.m_vMin.y - (plMathTestType)1; y < b.m_vMax.y + (plMathTestType)1; y += (plMathTestType)0.2f)
        {
          for (plMathTestType z = b.m_vMin.z - (plMathTestType)1; z < b.m_vMax.z + (plMathTestType)1; z += (plMathTestType)0.2f)
          {
            const plVec3T v(x, y, z);

            if (b.Contains(v))
              continue;

            const plVec3T vTarget0 = b.GetClampedPoint(v);

            const plVec3T vDir = (vTarget0 - c).GetNormalized();

            const plVec3T vTarget = vTarget0 - vDir * (plMathTestType)1;
            const plVec3T vSource = vTarget0 + vDir * (plMathTestType)3;

            plMathTestType f;
            plVec3T vi;
            PLASMA_TEST_BOOL(b.GetLineSegmentIntersection(vSource, vTarget, &f, &vi) == true);
            PLASMA_TEST_FLOAT(f, 0.75f, 0.001f);
            PLASMA_TEST_BOOL(vi.IsEqual(vTarget0, 0.0001f));
          }
        }
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNaN")
  {
    if (plMath::SupportsNaN<plMathTestType>())
    {
      plBoundingBoxT b;

      b.SetInvalid();
      PLASMA_TEST_BOOL(!b.IsNaN());

      b.SetInvalid();
      b.m_vMin.x = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vMin.y = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vMin.z = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vMax.x = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vMax.y = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vMax.z = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(b.IsNaN());
    }
  }
}

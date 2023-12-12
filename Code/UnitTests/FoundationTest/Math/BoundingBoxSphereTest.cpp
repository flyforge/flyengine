#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBoxSphere.h>

PLASMA_CREATE_SIMPLE_TEST(Math, BoundingBoxSphere)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plBoundingBoxSphereT b(plVec3T(-1, -2, -3), plVec3T(1, 2, 3), 2);

    PLASMA_TEST_BOOL(b.m_vCenter == plVec3T(-1, -2, -3));
    PLASMA_TEST_BOOL(b.m_vBoxHalfExtends == plVec3T(1, 2, 3));
    PLASMA_TEST_BOOL(b.m_fSphereRadius == 2);

    plBoundingBoxT box(plVec3T(1, 1, 1), plVec3T(3, 3, 3));
    plBoundingSphereT sphere(plVec3T(2, 2, 2), 1);

    b = plBoundingBoxSphereT(box, sphere);

    PLASMA_TEST_BOOL(b.m_vCenter == plVec3T(2, 2, 2));
    PLASMA_TEST_BOOL(b.m_vBoxHalfExtends == plVec3T(1, 1, 1));
    PLASMA_TEST_BOOL(b.m_fSphereRadius == 1);
    PLASMA_TEST_BOOL(b.GetBox() == box);
    PLASMA_TEST_BOOL(b.GetSphere() == sphere);

    b = plBoundingBoxSphereT(box);

    PLASMA_TEST_BOOL(b.m_vCenter == plVec3T(2, 2, 2));
    PLASMA_TEST_BOOL(b.m_vBoxHalfExtends == plVec3T(1, 1, 1));
    PLASMA_TEST_FLOAT(b.m_fSphereRadius, plMath::Sqrt(plMathTestType(3)), 0.00001f);
    PLASMA_TEST_BOOL(b.GetBox() == box);

    b = plBoundingBoxSphereT(sphere);

    PLASMA_TEST_BOOL(b.m_vCenter == plVec3T(2, 2, 2));
    PLASMA_TEST_BOOL(b.m_vBoxHalfExtends == plVec3T(1, 1, 1));
    PLASMA_TEST_BOOL(b.m_fSphereRadius == 1);
    PLASMA_TEST_BOOL(b.GetSphere() == sphere);
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

    plBoundingBoxSphereT b;
    b.SetFromPoints(p, 6);

    PLASMA_TEST_BOOL(b.m_vCenter == plVec3T(0.5, 0.5, 0.5));
    PLASMA_TEST_BOOL(b.m_vBoxHalfExtends == plVec3T(4.5, 6.5, 8.5));
    PLASMA_TEST_FLOAT(b.m_fSphereRadius, plVec3T(0.5, 0.5, 8.5).GetLength(), 0.00001f);
    PLASMA_TEST_BOOL(b.m_fSphereRadius <= b.m_vBoxHalfExtends.GetLength());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetInvalid")
  {
    plBoundingBoxSphereT b;
    b.SetInvalid();

    PLASMA_TEST_BOOL(!b.IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandToInclude")
  {
    plBoundingBoxSphereT b1;
    b1.SetInvalid();
    plBoundingBoxSphereT b2(plBoundingBoxT(plVec3T(2, 2, 2), plVec3T(4, 4, 4)));

    b1.ExpandToInclude(b2);
    PLASMA_TEST_BOOL(b1 == b2);

    plBoundingSphereT sphere(plVec3T(2, 2, 2), 2);
    b2 = plBoundingBoxSphereT(sphere);

    b1.ExpandToInclude(b2);
    PLASMA_TEST_BOOL(b1 != b2);

    PLASMA_TEST_BOOL(b1.m_vCenter == plVec3T(2, 2, 2));
    PLASMA_TEST_BOOL(b1.m_vBoxHalfExtends == plVec3T(2, 2, 2));
    PLASMA_TEST_FLOAT(b1.m_fSphereRadius, plMath::Sqrt(plMathTestType(3)) * 2, 0.00001f);
    PLASMA_TEST_BOOL(b1.m_fSphereRadius <= b1.m_vBoxHalfExtends.GetLength());

    b1.SetInvalid();
    b2 = plBoundingBoxT(plVec3T(0.25, 0.25, 0.25), plVec3T(0.5, 0.5, 0.5));

    b1.ExpandToInclude(b2);
    PLASMA_TEST_BOOL(b1 == b2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transform")
  {
    plBoundingBoxSphereT b(plVec3T(1), plVec3T(5), 5);

    plMat4T m;
    m.SetScalingMatrix(plVec3T(-2, -3, -2));
    m.SetTranslationVector(plVec3T(1, 1, 1));

    b.Transform(m);

    PLASMA_TEST_BOOL(b.m_vCenter == plVec3T(-1, -2, -1));
    PLASMA_TEST_BOOL(b.m_vBoxHalfExtends == plVec3T(10, 15, 10));
    PLASMA_TEST_BOOL(b.m_fSphereRadius == 15);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNaN")
  {
    if (plMath::SupportsNaN<plMathTestType>())
    {
      plBoundingBoxSphereT b;

      b.SetInvalid();
      PLASMA_TEST_BOOL(!b.IsNaN());

      b.SetInvalid();
      b.m_vCenter.x = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vCenter.y = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vCenter.z = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vBoxHalfExtends.x = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vBoxHalfExtends.y = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_vBoxHalfExtends.z = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(b.IsNaN());

      b.SetInvalid();
      b.m_fSphereRadius = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(b.IsNaN());
    }
  }
}

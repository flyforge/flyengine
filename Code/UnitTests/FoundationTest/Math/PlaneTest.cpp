#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/Math/Random.h>

PLASMA_CREATE_SIMPLE_TEST(Math, Plane)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Default Constructor")
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    if (plMath::SupportsNaN<plMat3T::ComponentType>())
    {
      // In debug the default constructor initializes everything with NaN.
      plPlaneT p;
      PLASMA_TEST_BOOL(plMath::IsNaN(p.m_vNormal.x) && plMath::IsNaN(p.m_vNormal.y) && plMath::IsNaN(p.m_vNormal.z) && plMath::IsNaN(p.m_fNegDistance));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    plPlaneT::ComponentType testBlock[4] = {(plPlaneT::ComponentType)1, (plPlaneT::ComponentType)2, (plPlaneT::ComponentType)3, (plPlaneT::ComponentType)4};
    plPlaneT* p = ::new ((void*)&testBlock[0]) plPlaneT;
    PLASMA_TEST_BOOL(p->m_vNormal.x == (plPlaneT::ComponentType)1 && p->m_vNormal.y == (plPlaneT::ComponentType)2 && p->m_vNormal.z == (plPlaneT::ComponentType)3 && p->m_fNegDistance == (plPlaneT::ComponentType)4);
#endif
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor(Normal, Point)")
  {
    plPlaneT p(plVec3T(1, 0, 0), plVec3T(5, 3, 1));

    PLASMA_TEST_BOOL(p.m_vNormal == plVec3T(1, 0, 0));
    PLASMA_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor(Point, Point, Point)")
  {
    plPlaneT p(plVec3T(-1, 5, 1), plVec3T(1, 5, 1), plVec3T(0, 5, -5));

    PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 1, 0), 0.0001f);
    PLASMA_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor(Points)")
  {
    plVec3T v[3] = {plVec3T(-1, 5, 1), plVec3T(1, 5, 1), plVec3T(0, 5, -5)};

    plPlaneT p(v);

    PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 1, 0), 0.0001f);
    PLASMA_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor(Points, numpoints)")
  {
    plVec3T v[6] = {plVec3T(-1, 5, 1), plVec3T(-1, 5, 1), plVec3T(1, 5, 1), plVec3T(1, 5, 1), plVec3T(0, 5, -5), plVec3T(0, 5, -5)};

    plPlaneT p(v, 6);

    PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 1, 0), 0.0001f);
    PLASMA_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFromNormalAndPoint")
  {
    plPlaneT p;
    p.SetFromNormalAndPoint(plVec3T(1, 0, 0), plVec3T(5, 3, 1));

    PLASMA_TEST_BOOL(p.m_vNormal == plVec3T(1, 0, 0));
    PLASMA_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFromPoints")
  {
    plPlaneT p;
    p.SetFromPoints(plVec3T(-1, 5, 1), plVec3T(1, 5, 1), plVec3T(0, 5, -5)).IgnoreResult();

    PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 1, 0), 0.0001f);
    PLASMA_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFromPoints")
  {
    plVec3T v[3] = {plVec3T(-1, 5, 1), plVec3T(1, 5, 1), plVec3T(0, 5, -5)};

    plPlaneT p;
    p.SetFromPoints(v).IgnoreResult();

    PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 1, 0), 0.0001f);
    PLASMA_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFromPoints")
  {
    plVec3T v[6] = {plVec3T(-1, 5, 1), plVec3T(-1, 5, 1), plVec3T(1, 5, 1), plVec3T(1, 5, 1), plVec3T(0, 5, -5), plVec3T(0, 5, -5)};

    plPlaneT p;
    p.SetFromPoints(v, 6).IgnoreResult();

    PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 1, 0), 0.0001f);
    PLASMA_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFromDirections")
  {
    plPlaneT p;
    p.SetFromDirections(plVec3T(1, 0, 0), plVec3T(1, 0, -1), plVec3T(3, 5, 9)).IgnoreResult();

    PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 1, 0), 0.0001f);
    PLASMA_TEST_FLOAT(p.m_fNegDistance, -5.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetInvalid")
  {
    plPlaneT p;
    p.SetFromDirections(plVec3T(1, 0, 0), plVec3T(1, 0, -1), plVec3T(3, 5, 9)).IgnoreResult();

    p.SetInvalid();

    PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 0, 0), 0.0001f);
    PLASMA_TEST_FLOAT(p.m_fNegDistance, 0.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDistanceTo")
  {
    plPlaneT p(plVec3T(1, 0, 0), plVec3T(5, 0, 0));

    PLASMA_TEST_FLOAT(p.GetDistanceTo(plVec3T(10, 3, 5)), 5.0f, 0.0001f);
    PLASMA_TEST_FLOAT(p.GetDistanceTo(plVec3T(0, 7, 123)), -5.0f, 0.0001f);
    PLASMA_TEST_FLOAT(p.GetDistanceTo(plVec3T(5, 12, 23)), 0.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetMinimumDistanceTo")
  {
    plVec3T v1[3] = {plVec3T(15, 3, 5), plVec3T(6, 7, 123), plVec3T(10, 12, 23)};
    plVec3T v2[3] = {plVec3T(3, 3, 5), plVec3T(5, 7, 123), plVec3T(10, 12, 23)};

    plPlaneT p(plVec3T(1, 0, 0), plVec3T(5, 0, 0));

    PLASMA_TEST_FLOAT(p.GetMinimumDistanceTo(v1, 3), 1.0f, 0.0001f);
    PLASMA_TEST_FLOAT(p.GetMinimumDistanceTo(v2, 3), -2.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetMinMaxDistanceTo")
  {
    plVec3T v1[3] = {plVec3T(15, 3, 5), plVec3T(5, 7, 123), plVec3T(0, 12, 23)};
    plVec3T v2[3] = {plVec3T(8, 3, 5), plVec3T(6, 7, 123), plVec3T(10, 12, 23)};

    plPlaneT p(plVec3T(1, 0, 0), plVec3T(5, 0, 0));

    plMathTestType fmin, fmax;

    p.GetMinMaxDistanceTo(fmin, fmax, v1, 3);
    PLASMA_TEST_FLOAT(fmin, -5.0f, 0.0001f);
    PLASMA_TEST_FLOAT(fmax, 10.0f, 0.0001f);

    p.GetMinMaxDistanceTo(fmin, fmax, v2, 3);
    PLASMA_TEST_FLOAT(fmin, 1, 0.0001f);
    PLASMA_TEST_FLOAT(fmax, 5, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetPointPosition")
  {
    plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

    PLASMA_TEST_BOOL(p.GetPointPosition(plVec3T(0, 15, 0)) == plPositionOnPlane::Front);
    PLASMA_TEST_BOOL(p.GetPointPosition(plVec3T(0, 5, 0)) == plPositionOnPlane::Back);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetPointPosition(planewidth)")
  {
    plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

    PLASMA_TEST_BOOL(p.GetPointPosition(plVec3T(0, 15, 0), 0.01f) == plPositionOnPlane::Front);
    PLASMA_TEST_BOOL(p.GetPointPosition(plVec3T(0, 5, 0), 0.01f) == plPositionOnPlane::Back);
    PLASMA_TEST_BOOL(p.GetPointPosition(plVec3T(0, 10, 0), 0.01f) == plPositionOnPlane::OnPlane);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetObjectPosition")
  {
    plPlaneT p(plVec3T(1, 0, 0), plVec3T(10, 0, 0));

    plVec3T v0[3] = {plVec3T(12, 0, 0), plVec3T(15, 0, 0), plVec3T(20, 0, 0)};
    plVec3T v1[3] = {plVec3T(8, 0, 0), plVec3T(6, 0, 0), plVec3T(4, 0, 0)};
    plVec3T v2[3] = {plVec3T(12, 0, 0), plVec3T(6, 0, 0), plVec3T(4, 0, 0)};

    PLASMA_TEST_BOOL(p.GetObjectPosition(v0, 3) == plPositionOnPlane::Front);
    PLASMA_TEST_BOOL(p.GetObjectPosition(v1, 3) == plPositionOnPlane::Back);
    PLASMA_TEST_BOOL(p.GetObjectPosition(v2, 3) == plPositionOnPlane::Spanning);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetObjectPosition(fPlaneHalfWidth)")
  {
    plPlaneT p(plVec3T(1, 0, 0), plVec3T(10, 0, 0));

    plVec3T v0[3] = {plVec3T(12, 0, 0), plVec3T(15, 0, 0), plVec3T(20, 0, 0)};
    plVec3T v1[3] = {plVec3T(8, 0, 0), plVec3T(6, 0, 0), plVec3T(4, 0, 0)};
    plVec3T v2[3] = {plVec3T(12, 0, 0), plVec3T(6, 0, 0), plVec3T(4, 0, 0)};
    plVec3T v3[3] = {plVec3T(10, 1, 0), plVec3T(10, 5, 7), plVec3T(10, 3, -5)};

    PLASMA_TEST_BOOL(p.GetObjectPosition(v0, 3, 0.001f) == plPositionOnPlane::Front);
    PLASMA_TEST_BOOL(p.GetObjectPosition(v1, 3, 0.001f) == plPositionOnPlane::Back);
    PLASMA_TEST_BOOL(p.GetObjectPosition(v2, 3, 0.001f) == plPositionOnPlane::Spanning);
    PLASMA_TEST_BOOL(p.GetObjectPosition(v3, 3, 0.001f) == plPositionOnPlane::OnPlane);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetObjectPosition(sphere)")
  {
    plPlaneT p(plVec3T(1, 0, 0), plVec3T(10, 0, 0));

    PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingSphereT(plVec3T(15, 2, 3), 3.0f)) == plPositionOnPlane::Front);
    PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingSphereT(plVec3T(5, 2, 3), 3.0f)) == plPositionOnPlane::Back);
    PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingSphereT(plVec3T(15, 2, 4.999f), 3.0f)) == plPositionOnPlane::Front);
    PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingSphereT(plVec3T(5, 2, 3), 4.999f)) == plPositionOnPlane::Back);
    PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingSphereT(plVec3T(8, 2, 3), 3.0f)) == plPositionOnPlane::Spanning);
    PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingSphereT(plVec3T(12, 2, 3), 3.0f)) == plPositionOnPlane::Spanning);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetObjectPosition(box)")
  {
    {
      plPlaneT p(plVec3T(1, 0, 0), plVec3T(10, 0, 0));
      PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingBoxT(plVec3T(10.1f), plVec3T(15))) == plPositionOnPlane::Front);
      PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingBoxT(plVec3T(7), plVec3T(9.9f))) == plPositionOnPlane::Back);
      PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingBoxT(plVec3T(7), plVec3T(15))) == plPositionOnPlane::Spanning);
    }
    {
      plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));
      PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingBoxT(plVec3T(10.1f), plVec3T(15))) == plPositionOnPlane::Front);
      PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingBoxT(plVec3T(7), plVec3T(9.9f))) == plPositionOnPlane::Back);
      PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingBoxT(plVec3T(7), plVec3T(15))) == plPositionOnPlane::Spanning);
    }
    {
      plPlaneT p(plVec3T(0, 0, 1), plVec3T(0, 0, 10));
      PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingBoxT(plVec3T(10.1f), plVec3T(15))) == plPositionOnPlane::Front);
      PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingBoxT(plVec3T(7), plVec3T(9.9f))) == plPositionOnPlane::Back);
      PLASMA_TEST_BOOL(p.GetObjectPosition(plBoundingBoxT(plVec3T(7), plVec3T(15))) == plPositionOnPlane::Spanning);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ProjectOntoPlane")
  {
    plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

    PLASMA_TEST_VEC3(p.ProjectOntoPlane(plVec3T(3, 15, 2)), plVec3T(3, 10, 2), 0.001f);
    PLASMA_TEST_VEC3(p.ProjectOntoPlane(plVec3T(-1, 5, -5)), plVec3T(-1, 10, -5), 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Mirror")
  {
    plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

    PLASMA_TEST_VEC3(p.Mirror(plVec3T(3, 15, 2)), plVec3T(3, 5, 2), 0.001f);
    PLASMA_TEST_VEC3(p.Mirror(plVec3T(-1, 5, -5)), plVec3T(-1, 15, -5), 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetCoplanarDirection")
  {
    plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

    PLASMA_TEST_VEC3(p.GetCoplanarDirection(plVec3T(0, 1, 0)), plVec3T(0, 0, 0), 0.001f);
    PLASMA_TEST_VEC3(p.GetCoplanarDirection(plVec3T(1, 1, 0)).GetNormalized(), plVec3T(1, 0, 0), 0.001f);
    PLASMA_TEST_VEC3(p.GetCoplanarDirection(plVec3T(-1, 1, 0)).GetNormalized(), plVec3T(-1, 0, 0), 0.001f);
    PLASMA_TEST_VEC3(p.GetCoplanarDirection(plVec3T(0, 1, 1)).GetNormalized(), plVec3T(0, 0, 1), 0.001f);
    PLASMA_TEST_VEC3(p.GetCoplanarDirection(plVec3T(0, 1, -1)).GetNormalized(), plVec3T(0, 0, -1), 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsIdentical / operator== / operator!=")
  {
    plPlaneT p1(plVec3T(0, 1, 0), plVec3T(0, 10, 0));
    plPlaneT p2(plVec3T(0, 1, 0), plVec3T(0, 10, 0));
    plPlaneT p3(plVec3T(0, 1, 0), plVec3T(0, 10.00001f, 0));

    PLASMA_TEST_BOOL(p1.IsIdentical(p1));
    PLASMA_TEST_BOOL(p2.IsIdentical(p2));
    PLASMA_TEST_BOOL(p3.IsIdentical(p3));

    PLASMA_TEST_BOOL(p1.IsIdentical(p2));
    PLASMA_TEST_BOOL(p2.IsIdentical(p1));

    PLASMA_TEST_BOOL(!p1.IsIdentical(p3));
    PLASMA_TEST_BOOL(!p2.IsIdentical(p3));


    PLASMA_TEST_BOOL(p1 == p2);
    PLASMA_TEST_BOOL(p2 == p1);

    PLASMA_TEST_BOOL(p1 != p3);
    PLASMA_TEST_BOOL(p2 != p3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqual")
  {
    plPlaneT p1(plVec3T(0, 1, 0), plVec3T(0, 10, 0));
    plPlaneT p2(plVec3T(0, 1, 0), plVec3T(0, 10, 0));
    plPlaneT p3(plVec3T(0, 1, 0), plVec3T(0, 10.00001f, 0));

    PLASMA_TEST_BOOL(p1.IsEqual(p1));
    PLASMA_TEST_BOOL(p2.IsEqual(p2));
    PLASMA_TEST_BOOL(p3.IsEqual(p3));

    PLASMA_TEST_BOOL(p1.IsEqual(p2));
    PLASMA_TEST_BOOL(p2.IsEqual(p1));

    PLASMA_TEST_BOOL(p1.IsEqual(p3));
    PLASMA_TEST_BOOL(p2.IsEqual(p3));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsValid")
  {
    plPlaneT p1(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

    PLASMA_TEST_BOOL(p1.IsValid());

    p1.SetInvalid();
    PLASMA_TEST_BOOL(!p1.IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transform(Mat3)")
  {
    plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

    plMat3T m;
    m.SetRotationMatrixX(plAngle::Degree(90));

    p.Transform(m);

    PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 0, 1), 0.0001f);
    PLASMA_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transform(Mat4)")
  {
    {
      plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

      plMat4T m;
      m.SetRotationMatrixX(plAngle::Degree(90));
      m.SetTranslationVector(plVec3T(0, 5, 0));

      p.Transform(m);

      PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 0, 1), 0.0001f);
      PLASMA_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);
    }

    {
      plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

      plMat4T m;
      m.SetRotationMatrixX(plAngle::Degree(90));
      m.SetTranslationVector(plVec3T(0, 0, 5));

      p.Transform(m);

      PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 0, 1), 0.0001f);
      PLASMA_TEST_FLOAT(p.m_fNegDistance, -15.0f, 0.0001f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Flip")
  {
    plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

    PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 1, 0), 0.0001f);
    PLASMA_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);

    p.Flip();

    PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, -1, 0), 0.0001f);
    PLASMA_TEST_FLOAT(p.m_fNegDistance, 10.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FlipIfNecessary")
  {
    {
      plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

      PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 1, 0), 0.0001f);
      PLASMA_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);

      PLASMA_TEST_BOOL(p.FlipIfNecessary(plVec3T(0, 11, 0), true) == false);

      PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 1, 0), 0.0001f);
      PLASMA_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);
    }

    {
      plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

      PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, 1, 0), 0.0001f);
      PLASMA_TEST_FLOAT(p.m_fNegDistance, -10.0f, 0.0001f);

      PLASMA_TEST_BOOL(p.FlipIfNecessary(plVec3T(0, 11, 0), false) == true);

      PLASMA_TEST_VEC3(p.m_vNormal, plVec3T(0, -1, 0), 0.0001f);
      PLASMA_TEST_FLOAT(p.m_fNegDistance, 10.0f, 0.0001f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetRayIntersection")
  {
    plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

    plMathTestType f;
    plVec3T v;

    PLASMA_TEST_BOOL(p.GetRayIntersection(plVec3T(3, 1, 7), plVec3T(0, 1, 0), &f, &v));
    PLASMA_TEST_FLOAT(f, 9, 0.0001f);
    PLASMA_TEST_VEC3(v, plVec3T(3, 10, 7), 0.0001f);

    PLASMA_TEST_BOOL(p.GetRayIntersection(plVec3T(3, 20, 7), plVec3T(0, -1, 0), &f, &v));
    PLASMA_TEST_FLOAT(f, 10, 0.0001f);
    PLASMA_TEST_VEC3(v, plVec3T(3, 10, 7), 0.0001f);

    PLASMA_TEST_BOOL(!p.GetRayIntersection(plVec3T(3, 1, 7), plVec3T(1, 0, 0), &f, &v));
    PLASMA_TEST_BOOL(!p.GetRayIntersection(plVec3T(3, 1, 7), plVec3T(0, -1, 0), &f, &v));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetRayIntersectionBiDirectional")
  {
    plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

    plMathTestType f;
    plVec3T v;

    PLASMA_TEST_BOOL(p.GetRayIntersectionBiDirectional(plVec3T(3, 1, 7), plVec3T(0, 1, 0), &f, &v));
    PLASMA_TEST_FLOAT(f, 9, 0.0001f);
    PLASMA_TEST_VEC3(v, plVec3T(3, 10, 7), 0.0001f);

    PLASMA_TEST_BOOL(!p.GetRayIntersectionBiDirectional(plVec3T(3, 1, 7), plVec3T(1, 0, 0), &f, &v));

    PLASMA_TEST_BOOL(p.GetRayIntersectionBiDirectional(plVec3T(3, 1, 7), plVec3T(0, -1, 0), &f, &v));
    PLASMA_TEST_FLOAT(f, -9, 0.0001f);
    PLASMA_TEST_VEC3(v, plVec3T(3, 10, 7), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLineSegmentIntersection")
  {
    plPlaneT p(plVec3T(0, 1, 0), plVec3T(0, 10, 0));

    plMathTestType f;
    plVec3T v;

    PLASMA_TEST_BOOL(p.GetLineSegmentIntersection(plVec3T(3, 5, 7), plVec3T(3, 15, 7), &f, &v));
    PLASMA_TEST_FLOAT(f, 0.5f, 0.0001f);
    PLASMA_TEST_VEC3(v, plVec3T(3, 10, 7), 0.0001f);

    PLASMA_TEST_BOOL(!p.GetLineSegmentIntersection(plVec3T(3, 5, 7), plVec3T(13, 5, 7), &f, &v));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetPlanesIntersectionPoint")
  {
    plPlaneT p1(plVec3T(1, 0, 0), plVec3T(0, 10, 0));
    plPlaneT p2(plVec3T(0, 1, 0), plVec3T(0, 10, 0));
    plPlaneT p3(plVec3T(0, 0, 1), plVec3T(0, 10, 0));

    plVec3T r;

    PLASMA_TEST_BOOL(plPlaneT::GetPlanesIntersectionPoint(p1, p2, p3, r) == PLASMA_SUCCESS);
    PLASMA_TEST_VEC3(r, plVec3T(0, 10, 0), 0.0001f);

    PLASMA_TEST_BOOL(plPlaneT::GetPlanesIntersectionPoint(p1, p1, p3, r) == PLASMA_FAILURE);
    PLASMA_TEST_BOOL(plPlaneT::GetPlanesIntersectionPoint(p1, p2, p2, r) == PLASMA_FAILURE);
    PLASMA_TEST_BOOL(plPlaneT::GetPlanesIntersectionPoint(p3, p2, p3, r) == PLASMA_FAILURE);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindSupportPoints")
  {
    plVec3T v[6] = {plVec3T(-1, 5, 1), plVec3T(-1, 5, 1), plVec3T(1, 5, 1), plVec3T(1, 5, 1), plVec3T(0, 5, -5), plVec3T(0, 5, -5)};

    plInt32 i1, i2, i3;

    plPlaneT::FindSupportPoints(v, 6, i1, i2, i3).IgnoreResult();

    PLASMA_TEST_INT(i1, 0);
    PLASMA_TEST_INT(i2, 2);
    PLASMA_TEST_INT(i3, 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNaN")
  {
    if (plMath::SupportsNaN<plMathTestType>())
    {
      plPlaneT p;

      p.SetInvalid();
      PLASMA_TEST_BOOL(!p.IsNaN());

      p.SetInvalid();
      p.m_fNegDistance = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(p.IsNaN());

      p.SetInvalid();
      p.m_vNormal.x = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(p.IsNaN());

      p.SetInvalid();
      p.m_vNormal.y = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(p.IsNaN());

      p.SetInvalid();
      p.m_vNormal.z = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(p.IsNaN());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsFinite")
  {
    if (plMath::SupportsInfinity<plMathTestType>())
    {
      plPlaneT p;

      p.m_vNormal = plVec3(1, 2, 3).GetNormalized();
      p.m_fNegDistance = 42;
      PLASMA_TEST_BOOL(p.IsValid());
      PLASMA_TEST_BOOL(p.IsFinite());

      p.SetInvalid();
      p.m_vNormal = plVec3(1, 2, 3).GetNormalized();
      p.m_fNegDistance = plMath::Infinity<plMathTestType>();
      PLASMA_TEST_BOOL(p.IsValid());
      PLASMA_TEST_BOOL(!p.IsFinite());

      p.SetInvalid();
      p.m_vNormal.x = plMath::NaN<plMathTestType>();
      p.m_fNegDistance = plMath::Infinity<plMathTestType>();
      PLASMA_TEST_BOOL(!p.IsValid());
      PLASMA_TEST_BOOL(!p.IsFinite());

      p.SetInvalid();
      p.m_vNormal = plVec3(1, 2, 3);
      p.m_fNegDistance = 42;
      PLASMA_TEST_BOOL(!p.IsValid());
      PLASMA_TEST_BOOL(p.IsFinite());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetMinimumDistanceTo/GetMaximumDistanceTo")
  {
    const plUInt32 numTestLoops = 1000 * 1000;

    plRandom randomGenerator;
    randomGenerator.Initialize(0x83482343);

    const auto randomNonZeroVec3T = [&randomGenerator]() -> plVec3T
    {
      const float extent = 1000.f;
      const plVec3T v (randomGenerator.FloatMinMax(-extent, extent), randomGenerator.FloatMinMax(-extent, extent), randomGenerator.FloatMinMax(-extent, extent));
      return v.GetLength() > 0.001f ? v : plVec3T::UnitXAxis();
    };

    for (plUInt32 loopIndex = 0; loopIndex < numTestLoops; ++loopIndex)
    {
      const plPlaneT plane ( randomNonZeroVec3T().GetNormalized(), randomNonZeroVec3T() );

      plVec3T boxCorners[8];
      plBoundingBoxT box;
      {
        const plVec3T boxPoint0 = randomNonZeroVec3T();
        const plVec3T boxPoint1 = randomNonZeroVec3T();
        const plVec3T boxMins (plMath::Min(boxPoint0.x, boxPoint1.x), plMath::Min(boxPoint0.y, boxPoint1.y), plMath::Min(boxPoint0.z, boxPoint1.z));
        const plVec3T boxMaxs (plMath::Max(boxPoint0.x, boxPoint1.x), plMath::Max(boxPoint0.y, boxPoint1.y), plMath::Max(boxPoint0.z, boxPoint1.z));
        box = plBoundingBoxT(boxMins, boxMaxs);
        box.GetCorners(boxCorners);
      }

      float distanceMin;
      float distanceMax;
      {
        distanceMin = plane.GetMinimumDistanceTo(box);
        distanceMax = plane.GetMaximumDistanceTo(box);
      }

      float referenceDistanceMin = FLT_MAX;
      float referenceDistanceMax = -FLT_MAX;
      {
        for (plUInt32 cornerIndex=0; cornerIndex<PLASMA_ARRAY_SIZE(boxCorners); ++cornerIndex)
        {
          const float cornerDist = plane.GetDistanceTo(boxCorners[cornerIndex]);
          referenceDistanceMin = plMath::Min(referenceDistanceMin, cornerDist);
          referenceDistanceMax = plMath::Max(referenceDistanceMax, cornerDist);
        }
      }

      // Break at first error to not spam the log with other potential error (the loop here is very long)
      {
        bool currIterSucceeded = true;
        currIterSucceeded = currIterSucceeded && PLASMA_TEST_FLOAT(distanceMin, referenceDistanceMin, 0.0001f);
        currIterSucceeded = currIterSucceeded && PLASMA_TEST_FLOAT(distanceMax, referenceDistanceMax, 0.0001f);
        if (!currIterSucceeded)
        {
          break;
        }
      }
    }
  }
}

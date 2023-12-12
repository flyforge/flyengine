#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Intersection.h>
#include <Foundation/Math/Mat4.h>

PLASMA_CREATE_SIMPLE_TEST(Math, Intersection)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RayPolygonIntersection")
  {
    for (plUInt32 i = 0; i < 100; ++i)
    {
      plMat4 m;
      m.SetRotationMatrix(plVec3(i + 1.0f, i * 3.0f, i * 7.0f).GetNormalized(), plAngle::Degree((float)i));
      m.SetTranslationVector(plVec3((float)i, i * 2.0f, i * 3.0f));

      plVec3 Vertices[8] = {m.TransformPosition(plVec3(-10, -10, 0)), plVec3(-10, -10, 0), m.TransformPosition(plVec3(10, -10, 0)),
        plVec3(10, -10, 0), m.TransformPosition(plVec3(10, 10, 0)), plVec3(10, 10, 0), m.TransformPosition(plVec3(-10, 10, 0)), plVec3(-10, 10, 0)};

      for (float y = -14.5; y <= 14.5f; y += 2.0f)
      {
        for (float x = -14.5; x <= 14.5f; x += 2.0f)
        {
          const plVec3 vRayDir = m.TransformDirection(plVec3(x, y, -10.0f));
          const plVec3 vRayStart = m.TransformPosition(plVec3(x, y, 0.0f)) - vRayDir * 3.0f;

          const bool bIntersects = (x >= -10.0f && x <= 10.0f && y >= -10.0f && y <= 10.0f);

          float fIntersection;
          plVec3 vIntersection;
          PLASMA_TEST_BOOL(plIntersectionUtils::RayPolygonIntersection(
                         vRayStart, vRayDir, Vertices, 4, &fIntersection, &vIntersection, sizeof(plVec3) * 2) == bIntersects);

          if (bIntersects)
          {
            PLASMA_TEST_FLOAT(fIntersection, 3.0f, 0.0001f);
            PLASMA_TEST_VEC3(vIntersection, m.TransformPosition(plVec3(x, y, 0.0f)), 0.0001f);
          }
        }
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ClosestPoint_PointLineSegment")
  {
    for (plUInt32 i = 0; i < 100; ++i)
    {
      plMat4 m;
      m.SetRotationMatrix(plVec3(i + 1.0f, i * 3.0f, i * 7.0f).GetNormalized(), plAngle::Degree((float)i));
      m.SetTranslationVector(plVec3((float)i, i * 2.0f, i * 3.0f));

      plVec3 vSegment0 = m.TransformPosition(plVec3(-10, 1, 2));
      plVec3 vSegment1 = m.TransformPosition(plVec3(10, 1, 2));

      for (float f = -20; f <= -10; f += 0.5f)
      {
        const plVec3 vPos = m.TransformPosition(plVec3(f, 10.0f, 20.0f));

        float fFraction = -1.0f;
        const plVec3 vClosest = plIntersectionUtils::ClosestPoint_PointLineSegment(vPos, vSegment0, vSegment1, &fFraction);

        PLASMA_TEST_FLOAT(fFraction, 0.0f, 0.0001f);
        PLASMA_TEST_VEC3(vClosest, vSegment0, 0.0001f);
      }

      for (float f = -10; f <= 10; f += 0.5f)
      {
        const plVec3 vPos = m.TransformPosition(plVec3(f, 10.0f, 20.0f));

        float fFraction = -1.0f;
        const plVec3 vClosest = plIntersectionUtils::ClosestPoint_PointLineSegment(vPos, vSegment0, vSegment1, &fFraction);

        PLASMA_TEST_FLOAT(fFraction, (f + 10.0f) / 20.0f, 0.0001f);
        PLASMA_TEST_VEC3(vClosest, m.TransformPosition(plVec3(f, 1, 2)), 0.0001f);
      }

      for (float f = 10; f <= 20; f += 0.5f)
      {
        const plVec3 vPos = m.TransformPosition(plVec3(f, 10.0f, 20.0f));

        float fFraction = -1.0f;
        const plVec3 vClosest = plIntersectionUtils::ClosestPoint_PointLineSegment(vPos, vSegment0, vSegment1, &fFraction);

        PLASMA_TEST_FLOAT(fFraction, 1.0f, 0.0001f);
        PLASMA_TEST_VEC3(vClosest, vSegment1, 0.0001f);
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Ray2DLine2D")
  {
    for (plUInt32 i = 0; i < 100; ++i)
    {
      plMat4 m;
      m.SetRotationMatrixZ(plAngle::Degree((float)i));
      m.SetTranslationVector(plVec3((float)i, i * 2.0f, i * 3.0f));

      const plVec2 vSegment0 = m.TransformPosition(plVec3(23, 42, 0)).GetAsVec2();
      const plVec2 vSegmentDir = m.TransformDirection(plVec3(13, 15, 0)).GetAsVec2();

      const plVec2 vSegment1 = vSegment0 + vSegmentDir;

      for (float f = -1.1f; f < 2.0f; f += 0.2f)
      {
        const bool bIntersection = (f >= 0.0f && f <= 1.0f);
        const plVec2 vSegmentPos = vSegment0 + f * vSegmentDir;

        const plVec2 vRayDir = plVec2(2.0f, f);
        const plVec2 vRayStart = vSegmentPos - vRayDir * 5.0f;

        float fIntersection;
        plVec2 vIntersection;
        PLASMA_TEST_BOOL(plIntersectionUtils::Ray2DLine2D(vRayStart, vRayDir, vSegment0, vSegment1, &fIntersection, &vIntersection) == bIntersection);

        if (bIntersection)
        {
          PLASMA_TEST_FLOAT(fIntersection, 5.0f, 0.0001f);
          PLASMA_TEST_VEC2(vIntersection, vSegmentPos, 0.0001f);
        }
      };
    }
  }
}

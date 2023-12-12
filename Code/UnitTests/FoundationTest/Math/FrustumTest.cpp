#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Frustum.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Utilities/GraphicsUtils.h>

PLASMA_CREATE_SIMPLE_TEST(Math, Frustum)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFrustum (planes)")
  {
    plFrustum f;

    plPlane p[6];
    p[0].SetFromNormalAndPoint(plVec3(1, 0, 0), plVec3(1, 2, 3));
    p[1].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));
    p[2].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));
    p[3].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));
    p[4].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));
    p[5].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));

    f.SetFrustum(p);

    PLASMA_TEST_BOOL(f.GetPlane(0) == p[0]);
    PLASMA_TEST_BOOL(f.GetPlane(1) == p[1]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TransformFrustum")
  {
    plFrustum f;

    plPlane p[6];
    p[0].SetFromNormalAndPoint(plVec3(1, 0, 0), plVec3(1, 2, 3));
    p[1].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));
    p[2].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));
    p[3].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));
    p[4].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));
    p[5].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));

    f.SetFrustum(p);

    plMat4 mTransform;
    mTransform.SetRotationMatrixY(plAngle::Degree(90.0f));
    mTransform.SetTranslationVector(plVec3(2, 3, 4));

    f.TransformFrustum(mTransform);

    p[0].Transform(mTransform);
    p[1].Transform(mTransform);

    PLASMA_TEST_BOOL(f.GetPlane(0).IsEqual(p[0], 0.001f));
    PLASMA_TEST_BOOL(f.GetPlane(1).IsEqual(p[1], 0.001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "InvertFrustum")
  {
    plFrustum f;

    plPlane p[6];
    p[0].SetFromNormalAndPoint(plVec3(1, 0, 0), plVec3(1, 2, 3));
    p[1].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));
    p[2].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));
    p[3].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));
    p[4].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));
    p[5].SetFromNormalAndPoint(plVec3(0, 1, 0), plVec3(2, 3, 4));

    f.SetFrustum(p);

    f.InvertFrustum();

    p[0].Flip();
    p[1].Flip();

    PLASMA_TEST_BOOL(f.GetPlane(0) == p[0]);
    PLASMA_TEST_BOOL(f.GetPlane(1) == p[1]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFrustum")
  {
    // check that the extracted frustum planes are always the same, no matter the handedness or depth-range

    // test the different depth ranges
    for (int r = 0; r < 2; ++r)
    {
      const plClipSpaceDepthRange::Enum range = (r == 0) ? plClipSpaceDepthRange::MinusOneToOne : plClipSpaceDepthRange::ZeroToOne;

      // test rotated model-view matrices
      for (int rot = 0; rot < 360; rot += 45)
      {
        plVec3 vLookDir;
        vLookDir.Set(plMath::Sin(plAngle::Degree((float)rot)), 0, -plMath::Cos(plAngle::Degree((float)rot)));

        plVec3 vRightDir;
        vRightDir.Set(plMath::Sin(plAngle::Degree(rot + 90.0f)), 0, -plMath::Cos(plAngle::Degree(rot + 90.0f)));

        const plVec3 vCamPos(rot * 1.0f, rot * 0.5f, rot * -0.3f);

        // const plMat4 mViewLH = plGraphicsUtils::CreateViewMatrix(vCamPos, vLookDir, -vRightDir, plVec3(0, 1, 0), plHandedness::LeftHanded);
        // const plMat4 mViewRH = plGraphicsUtils::CreateViewMatrix(vCamPos, vLookDir, vRightDir, plVec3(0, 1, 0), plHandedness::RightHanded);
        const plMat4 mViewLH = plGraphicsUtils::CreateLookAtViewMatrix(vCamPos, vCamPos + vLookDir, plVec3(0, 1, 0), plHandedness::LeftHanded);
        const plMat4 mViewRH = plGraphicsUtils::CreateLookAtViewMatrix(vCamPos, vCamPos + vLookDir, plVec3(0, 1, 0), plHandedness::RightHanded);

        const plMat4 mProjLH = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
          plAngle::Degree(90), 1.0f, 1.0f, 100.0f, range, plClipSpaceYMode::Regular, plHandedness::LeftHanded);
        const plMat4 mProjRH = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
          plAngle::Degree(90), 1.0f, 1.0f, 100.0f, range, plClipSpaceYMode::Regular, plHandedness::RightHanded);

        const plMat4 mViewProjLH = mProjLH * mViewLH;
        const plMat4 mViewProjRH = mProjRH * mViewRH;

        plFrustum fLH, fRH, fB;
        fLH.SetFrustum(mViewProjLH, range, plHandedness::LeftHanded);
        fRH.SetFrustum(mViewProjRH, range, plHandedness::RightHanded);

        fB.SetFrustum(vCamPos, vLookDir, plVec3(0, 1, 0), plAngle::Degree(90), plAngle::Degree(90), 1.0f, 100.0f);

        PLASMA_TEST_BOOL(fRH.GetPlane(plFrustum::NearPlane).IsEqual(fB.GetPlane(plFrustum::NearPlane), 0.1f));
        PLASMA_TEST_BOOL(fRH.GetPlane(plFrustum::LeftPlane).IsEqual(fB.GetPlane(plFrustum::LeftPlane), 0.1f));
        PLASMA_TEST_BOOL(fRH.GetPlane(plFrustum::RightPlane).IsEqual(fB.GetPlane(plFrustum::RightPlane), 0.1f));
        PLASMA_TEST_BOOL(fRH.GetPlane(plFrustum::FarPlane).IsEqual(fB.GetPlane(plFrustum::FarPlane), 0.1f));
        PLASMA_TEST_BOOL(fRH.GetPlane(plFrustum::BottomPlane).IsEqual(fB.GetPlane(plFrustum::BottomPlane), 0.1f));
        PLASMA_TEST_BOOL(fRH.GetPlane(plFrustum::TopPlane).IsEqual(fB.GetPlane(plFrustum::TopPlane), 0.1f));

        PLASMA_TEST_BOOL(fLH.GetPlane(plFrustum::NearPlane).IsEqual(fB.GetPlane(plFrustum::NearPlane), 0.1f));
        PLASMA_TEST_BOOL(fLH.GetPlane(plFrustum::LeftPlane).IsEqual(fB.GetPlane(plFrustum::LeftPlane), 0.1f));
        PLASMA_TEST_BOOL(fLH.GetPlane(plFrustum::RightPlane).IsEqual(fB.GetPlane(plFrustum::RightPlane), 0.1f));
        PLASMA_TEST_BOOL(fLH.GetPlane(plFrustum::FarPlane).IsEqual(fB.GetPlane(plFrustum::FarPlane), 0.1f));
        PLASMA_TEST_BOOL(fLH.GetPlane(plFrustum::BottomPlane).IsEqual(fB.GetPlane(plFrustum::BottomPlane), 0.1f));
        PLASMA_TEST_BOOL(fLH.GetPlane(plFrustum::TopPlane).IsEqual(fB.GetPlane(plFrustum::TopPlane), 0.1f));
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Culling")
  {
    const plVec3 offsetPos(23, 17, -9);
    const plVec3 camDir[6] = {plVec3(-1, 0, 0), plVec3(1, 0, 0), plVec3(0, -1, 0), plVec3(0, 1, 0), plVec3(0, 0, -1), plVec3(0, 0, 1)};
    const plVec3 objPos[6] = {plVec3(-9, 0, 0), plVec3(9, 0, 0), plVec3(0, -9, 0), plVec3(0, 9, 0), plVec3(0, 0, -9), plVec3(0, 0, 9)};

    for (plUInt32 dir = 0; dir < 6; ++dir)
    {
      plFrustum fDir;
      fDir.SetFrustum(
        offsetPos, camDir[dir], camDir[dir].GetOrthogonalVector() /*arbitrary*/, plAngle::Degree(90), plAngle::Degree(90), 1.0f, 100.0f);

      for (plUInt32 obj = 0; obj < 6; ++obj)
      {
        // box
        {
          plBoundingBox boundingObj;
          boundingObj.SetCenterAndHalfExtents(offsetPos + objPos[obj], plVec3(1.0f));

          const plVolumePosition::Enum res = fDir.GetObjectPosition(boundingObj);

          if (obj == dir)
            PLASMA_TEST_BOOL(res == plVolumePosition::Inside);
          else
            PLASMA_TEST_BOOL(res == plVolumePosition::Outside);
        }

        // sphere
        {
          plBoundingSphere boundingObj;
          boundingObj.SetElements(offsetPos + objPos[obj], 0.93f);

          const plVolumePosition::Enum res = fDir.GetObjectPosition(boundingObj);

          if (obj == dir)
            PLASMA_TEST_BOOL(res == plVolumePosition::Inside);
          else
            PLASMA_TEST_BOOL(res == plVolumePosition::Outside);
        }

        // vertices
        {
          plBoundingBox boundingObj;
          boundingObj.SetCenterAndHalfExtents(offsetPos + objPos[obj], plVec3(1.0f));

          plVec3 vertices[8];
          boundingObj.GetCorners(vertices);

          const plVolumePosition::Enum res = fDir.GetObjectPosition(vertices, 8);

          if (obj == dir)
            PLASMA_TEST_BOOL(res == plVolumePosition::Inside);
          else
            PLASMA_TEST_BOOL(res == plVolumePosition::Outside);
        }

        // vertices + transform
        {
          plBoundingBox boundingObj;
          boundingObj.SetCenterAndHalfExtents(objPos[obj], plVec3(1.0f));

          plVec3 vertices[8];
          boundingObj.GetCorners(vertices);

          plMat4 transform;
          transform.SetTranslationMatrix(offsetPos);

          const plVolumePosition::Enum res = fDir.GetObjectPosition(vertices, 8, transform);

          if (obj == dir)
            PLASMA_TEST_BOOL(res == plVolumePosition::Inside);
          else
            PLASMA_TEST_BOOL(res == plVolumePosition::Outside);
        }

        // SIMD box
        {
          plBoundingBox boundingObj;
          boundingObj.SetCenterAndHalfExtents(offsetPos + objPos[obj], plVec3(1.0f));

          const bool res = fDir.Overlaps(plSimdConversion::ToBBox(boundingObj));

          if (obj == dir)
            PLASMA_TEST_BOOL(res == true);
          else
            PLASMA_TEST_BOOL(res == false);
        }

        // SIMD sphere
        {
          plBoundingSphere boundingObj;
          boundingObj.SetElements(offsetPos + objPos[obj], 0.93f);

          const bool res = fDir.Overlaps(plSimdConversion::ToBSphere(boundingObj));

          if (obj == dir)
            PLASMA_TEST_BOOL(res == true);
          else
            PLASMA_TEST_BOOL(res == false);
        }
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ComputeCornerPoints")
  {
    const plMat4 mProj = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
      plAngle::Degree(90), 1.0f, 1.0f, 10.0f, plClipSpaceDepthRange::MinusOneToOne, plClipSpaceYMode::Regular, plHandedness::RightHanded);

    plFrustum frustum[2];
    frustum[0].SetFrustum(mProj, plClipSpaceDepthRange::MinusOneToOne, plHandedness::RightHanded);
    frustum[1].SetFrustum(plVec3::ZeroVector(), plVec3(0, 0, -1), plVec3(0, 1, 0), plAngle::Degree(90), plAngle::Degree(90), 1.0f, 10.0f);

    for (int f = 0; f < 2; ++f)
    {
      plVec3 corner[8];
      frustum[f].ComputeCornerPoints(corner);

      plPositionOnPlane::Enum results[8][6];

      for (int c = 0; c < 8; ++c)
      {
        for (int p = 0; p < 6; ++p)
        {
          results[c][p] = plPositionOnPlane::Back;
        }
      }

      results[plFrustum::FrustumCorner::NearTopLeft][plFrustum::PlaneType::NearPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::NearTopLeft][plFrustum::PlaneType::TopPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::NearTopLeft][plFrustum::PlaneType::LeftPlane] = plPositionOnPlane::OnPlane;

      results[plFrustum::FrustumCorner::NearTopRight][plFrustum::PlaneType::NearPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::NearTopRight][plFrustum::PlaneType::TopPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::NearTopRight][plFrustum::PlaneType::RightPlane] = plPositionOnPlane::OnPlane;

      results[plFrustum::FrustumCorner::NearBottomLeft][plFrustum::PlaneType::NearPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::NearBottomLeft][plFrustum::PlaneType::BottomPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::NearBottomLeft][plFrustum::PlaneType::LeftPlane] = plPositionOnPlane::OnPlane;

      results[plFrustum::FrustumCorner::NearBottomRight][plFrustum::PlaneType::NearPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::NearBottomRight][plFrustum::PlaneType::BottomPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::NearBottomRight][plFrustum::PlaneType::RightPlane] = plPositionOnPlane::OnPlane;

      results[plFrustum::FrustumCorner::FarTopLeft][plFrustum::PlaneType::FarPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::FarTopLeft][plFrustum::PlaneType::TopPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::FarTopLeft][plFrustum::PlaneType::LeftPlane] = plPositionOnPlane::OnPlane;

      results[plFrustum::FrustumCorner::FarTopRight][plFrustum::PlaneType::FarPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::FarTopRight][plFrustum::PlaneType::TopPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::FarTopRight][plFrustum::PlaneType::RightPlane] = plPositionOnPlane::OnPlane;

      results[plFrustum::FrustumCorner::FarBottomLeft][plFrustum::PlaneType::FarPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::FarBottomLeft][plFrustum::PlaneType::BottomPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::FarBottomLeft][plFrustum::PlaneType::LeftPlane] = plPositionOnPlane::OnPlane;

      results[plFrustum::FrustumCorner::FarBottomRight][plFrustum::PlaneType::FarPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::FarBottomRight][plFrustum::PlaneType::BottomPlane] = plPositionOnPlane::OnPlane;
      results[plFrustum::FrustumCorner::FarBottomRight][plFrustum::PlaneType::RightPlane] = plPositionOnPlane::OnPlane;

      for (int c = 0; c < 8; ++c)
      {
        plFrustum::FrustumCorner cornerName = (plFrustum::FrustumCorner)c;

        for (int p = 0; p < 6; ++p)
        {
          plFrustum::PlaneType planeName = (plFrustum::PlaneType)p;

          plPlane plane = frustum[f].GetPlane(planeName);
          plPositionOnPlane::Enum expected = results[cornerName][planeName];
          plPositionOnPlane::Enum result = plane.GetPointPosition(corner[cornerName], 0.1f);
          // float fDistToPlane = plane.GetDistanceTo(corner[cornerName]);
          PLASMA_TEST_BOOL(result == expected);
        }
      }
    }
  }
}

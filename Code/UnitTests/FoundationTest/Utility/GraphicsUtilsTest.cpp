#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>

PLASMA_CREATE_SIMPLE_TEST(Utility, GraphicsUtils)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Perspective (-1/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    plMat4 mProj, mProjInv;

    mProj = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(
      plAngle::Degree(85.0f), 2.0f, 1.0f, 1000.0f, plClipSpaceDepthRange::MinusOneToOne, plClipSpaceYMode::Regular, plHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (plUInt32 y = 0; y < 25; ++y)
    {
      for (plUInt32 x = 0; x < 50; ++x)
      {
        plVec3 vPoint, vDir;
        PLASMA_TEST_BOOL(plGraphicsUtils::ConvertScreenPosToWorldPos(
          mProjInv, 0, 0, 50, 25, plVec3((float)x, (float)y, 0.5f), vPoint, &vDir, plClipSpaceDepthRange::MinusOneToOne)
                       .Succeeded());

        PLASMA_TEST_VEC3(vDir, vPoint.GetNormalized(), 0.01f);

        plVec3 vScreen;
        PLASMA_TEST_BOOL(
          plGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, plClipSpaceDepthRange::MinusOneToOne).Succeeded());

        PLASMA_TEST_VEC3(vScreen, plVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Perspective (0/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    plMat4 mProj, mProjInv;
    mProj = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(
      plAngle::Degree(85.0f), 2.0f, 1.0f, 1000.0f, plClipSpaceDepthRange::ZeroToOne, plClipSpaceYMode::Regular, plHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (plUInt32 y = 0; y < 25; ++y)
    {
      for (plUInt32 x = 0; x < 50; ++x)
      {
        plVec3 vPoint, vDir;
        PLASMA_TEST_BOOL(plGraphicsUtils::ConvertScreenPosToWorldPos(
          mProjInv, 0, 0, 50, 25, plVec3((float)x, (float)y, 0.5f), vPoint, &vDir, plClipSpaceDepthRange::ZeroToOne)
                       .Succeeded());

        PLASMA_TEST_VEC3(vDir, vPoint.GetNormalized(), 0.01f);

        plVec3 vScreen;
        PLASMA_TEST_BOOL(plGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, plClipSpaceDepthRange::ZeroToOne).Succeeded());

        PLASMA_TEST_VEC3(vScreen, plVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Ortho (-1/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    plMat4 mProj, mProjInv;
    mProj = plGraphicsUtils::CreateOrthographicProjectionMatrix(
      50, 25, 1.0f, 1000.0f, plClipSpaceDepthRange::MinusOneToOne, plClipSpaceYMode::Regular, plHandedness::LeftHanded);

    mProjInv = mProj.GetInverse();

    for (plUInt32 y = 0; y < 25; ++y)
    {
      for (plUInt32 x = 0; x < 50; ++x)
      {
        plVec3 vPoint, vDir;
        PLASMA_TEST_BOOL(plGraphicsUtils::ConvertScreenPosToWorldPos(
          mProjInv, 0, 0, 50, 25, plVec3((float)x, (float)y, 0.5f), vPoint, &vDir, plClipSpaceDepthRange::MinusOneToOne)
                       .Succeeded());

        PLASMA_TEST_VEC3(vDir, plVec3(0, 0, 1.0f), 0.01f);

        plVec3 vScreen;
        PLASMA_TEST_BOOL(
          plGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, plClipSpaceDepthRange::MinusOneToOne).Succeeded());

        PLASMA_TEST_VEC3(vScreen, plVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Ortho (0/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    plMat4 mProj, mProjInv;
    mProj = plGraphicsUtils::CreateOrthographicProjectionMatrix(
      50, 25, 1.0f, 1000.0f, plClipSpaceDepthRange::ZeroToOne, plClipSpaceYMode::Regular, plHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (plUInt32 y = 0; y < 25; ++y)
    {
      for (plUInt32 x = 0; x < 50; ++x)
      {
        plVec3 vPoint, vDir;
        PLASMA_TEST_BOOL(plGraphicsUtils::ConvertScreenPosToWorldPos(
          mProjInv, 0, 0, 50, 25, plVec3((float)x, (float)y, 0.5f), vPoint, &vDir, plClipSpaceDepthRange::ZeroToOne)
                       .Succeeded());

        PLASMA_TEST_VEC3(vDir, plVec3(0, 0, 1.0f), 0.01f);

        plVec3 vScreen;
        PLASMA_TEST_BOOL(plGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, plClipSpaceDepthRange::ZeroToOne).Succeeded());

        PLASMA_TEST_VEC3(vScreen, plVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ConvertProjectionMatrixDepthRange")
  {
    plMat4 mProj1, mProj2;
    mProj1 = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(
      plAngle::Degree(85.0f), 2.0f, 1.0f, 1000.0f, plClipSpaceDepthRange::ZeroToOne, plClipSpaceYMode::Regular, plHandedness::LeftHanded);
    mProj2 = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(
      plAngle::Degree(85.0f), 2.0f, 1.0f, 1000.0f, plClipSpaceDepthRange::MinusOneToOne, plClipSpaceYMode::Regular, plHandedness::LeftHanded);

    plMat4 mProj1b = mProj1;
    plMat4 mProj2b = mProj2;
    plGraphicsUtils::ConvertProjectionMatrixDepthRange(mProj1b, plClipSpaceDepthRange::ZeroToOne, plClipSpaceDepthRange::MinusOneToOne);
    plGraphicsUtils::ConvertProjectionMatrixDepthRange(mProj2b, plClipSpaceDepthRange::MinusOneToOne, plClipSpaceDepthRange::ZeroToOne);

    PLASMA_TEST_BOOL(mProj1.IsEqual(mProj2b, 0.001f));
    PLASMA_TEST_BOOL(mProj2.IsEqual(mProj1b, 0.001f));
  }

  struct DepthRange
  {
    float fNear = 0.0f;
    float fFar = 0.0f;
  };

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExtractPerspectiveMatrixFieldOfView")
  {
    DepthRange depthRanges[] = {{1.0f, 1000.0f}, {1000.0f, 1.0f}, {0.5f, 20.0f}, {20.0f, 0.5f}};
    plClipSpaceDepthRange::Enum clipRanges[] = {plClipSpaceDepthRange::ZeroToOne, plClipSpaceDepthRange::MinusOneToOne};
    plHandedness::Enum handednesses[] = {plHandedness::LeftHanded, plHandedness::RightHanded};
    plClipSpaceYMode::Enum clipSpaceYModes[] = {plClipSpaceYMode::Regular, plClipSpaceYMode::Flipped};

    for (auto clipSpaceYMode : clipSpaceYModes)
    {
      for (auto handedness : handednesses)
      {
        for (auto depthRange : depthRanges)
        {
          for (auto clipRange : clipRanges)
          {
            for (plUInt32 angle = 10; angle < 180; angle += 10)
            {
              {
                plMat4 mProj;
                mProj = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(
                  plAngle::Degree((float)angle), 2.0f, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                plAngle fovx, fovy;
                plGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fovx, fovy);

                PLASMA_TEST_FLOAT(fovx.GetDegree(), (float)angle, 0.5f);
              }

              {
                plMat4 mProj;
                mProj = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
                  plAngle::Degree((float)angle), 1.0f / 3.0f, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                plAngle fovx, fovy;
                plGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fovx, fovy);

                PLASMA_TEST_FLOAT(fovy.GetDegree(), (float)angle, 0.5f);
              }

              {
                const float fMinDepth = plMath::Min(depthRange.fNear, depthRange.fFar);
                const plAngle right = plAngle::Degree((float)angle) / 2;
                const plAngle top = plAngle::Degree((float)angle) / 2;
                const float fLeft = plMath::Tan(-right) * fMinDepth;
                const float fRight = plMath::Tan(right) * fMinDepth * 0.8f;
                const float fBottom = plMath::Tan(-top) * fMinDepth;
                const float fTop = plMath::Tan(top) * fMinDepth * 0.7f;

                plMat4 mProj;
                mProj = plGraphicsUtils::CreatePerspectiveProjectionMatrix(fLeft, fRight, fBottom, fTop, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                float fNearOut, fFarOut;
                PLASMA_TEST_BOOL(plGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, clipRange).Succeeded());
                PLASMA_TEST_FLOAT(depthRange.fNear, fNearOut, 0.1f);
                PLASMA_TEST_FLOAT(depthRange.fFar, fFarOut, 0.1f);

                float fLeftOut, fRightOut, fBottomOut, fTopOut;
                PLASMA_TEST_BOOL(plGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fLeftOut, fRightOut, fBottomOut, fTopOut, clipRange, clipSpaceYMode).Succeeded());
                PLASMA_TEST_FLOAT(fLeft, fLeftOut, plMath::LargeEpsilon<float>());
                PLASMA_TEST_FLOAT(fRight, fRightOut, plMath::LargeEpsilon<float>());
                PLASMA_TEST_FLOAT(fBottom, fBottomOut, plMath::LargeEpsilon<float>());
                PLASMA_TEST_FLOAT(fTop, fTopOut, plMath::LargeEpsilon<float>());

                plAngle fFovLeft;
                plAngle fFovRight;
                plAngle fFovBottom;
                plAngle fFovTop;
                plGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop, clipSpaceYMode);

                PLASMA_TEST_FLOAT(fLeft, plMath::Tan(fFovLeft) * fMinDepth, plMath::LargeEpsilon<float>());
                PLASMA_TEST_FLOAT(fRight, plMath::Tan(fFovRight) * fMinDepth, plMath::LargeEpsilon<float>());
                PLASMA_TEST_FLOAT(fBottom, plMath::Tan(fFovBottom) * fMinDepth, plMath::LargeEpsilon<float>());
                PLASMA_TEST_FLOAT(fTop, plMath::Tan(fFovTop) * fMinDepth, plMath::LargeEpsilon<float>());
              }
            }
          }
        }
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExtractNearAndFarClipPlaneDistances")
  {
    DepthRange depthRanges[] = {{0.001f, 100.0f}, {0.01f, 10.0f}, {10.0f, 0.01f}, {1.01f, 110.0f}, {110.0f, 1.01f}};
    plClipSpaceDepthRange::Enum clipRanges[] = {plClipSpaceDepthRange::ZeroToOne, plClipSpaceDepthRange::MinusOneToOne};
    plHandedness::Enum handednesses[] = {plHandedness::LeftHanded, plHandedness::RightHanded};
    plClipSpaceYMode::Enum clipSpaceYModes[] = {plClipSpaceYMode::Regular, plClipSpaceYMode::Flipped};
    plAngle fovs[] = {plAngle::Degree(10.0f), plAngle::Degree(70.0f)};

    for (auto clipSpaceYMode : clipSpaceYModes)
    {
      for (auto handedness : handednesses)
      {
        for (auto depthRange : depthRanges)
        {
          for (auto clipRange : clipRanges)
          {
            for (auto fov : fovs)
            {
              plMat4 mProj;
              mProj = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
                fov, 0.7f, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

              float fNearOut, fFarOut;
              PLASMA_TEST_BOOL(plGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, clipRange).Succeeded());

              PLASMA_TEST_FLOAT(depthRange.fNear, fNearOut, 0.1f);
              PLASMA_TEST_FLOAT(depthRange.fFar, fFarOut, 0.2f);
            }
          }
        }
      }
    }

    { // Test failure on broken projection matrix
      // This matrix has a 0 in the w-component of the third column (invalid perspective divide)
      float vals[] = {0.770734549f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, 1.73205078f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, -1.00000000f, 0.00000000f, 0.000000000, 0.000000000f, -0.100000001f, 0.000000000f};
      plMat4 mProj;
      memcpy(mProj.m_fElementsCM, vals, 16 * sizeof(float));
      float fNearOut = 0.f, fFarOut = 0.f;
      PLASMA_TEST_BOOL(plGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, plClipSpaceDepthRange::MinusOneToOne).Failed());
      PLASMA_TEST_BOOL(fNearOut == 0.0f);
      PLASMA_TEST_BOOL(fFarOut == 0.0f);
    }

    { // Test failure on broken projection matrix
      // This matrix has a 0 in the z-component of the fourth column (one or both projection planes are zero)
      float vals[] = {0.770734549f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, 1.73205078f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, -1.00000000f, -1.00000000f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f};
      plMat4 mProj;
      memcpy(mProj.m_fElementsCM, vals, 16 * sizeof(float));
      float fNearOut = 0.f, fFarOut = 0.f;
      PLASMA_TEST_BOOL(plGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, plClipSpaceDepthRange::MinusOneToOne).Failed());
      PLASMA_TEST_BOOL(fNearOut == 0.0f);
      PLASMA_TEST_BOOL(fFarOut == 0.0f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ComputeInterpolatedFrustumPlane")
  {
    for (plUInt32 i = 0; i <= 10; ++i)
    {
      float nearPlane = 1.0f;
      float farPlane = 1000.0f;

      plMat4 mProj;
      mProj = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
        plAngle::Degree(90.0f), 1.0f, nearPlane, farPlane, plClipSpaceDepthRange::ZeroToOne, plClipSpaceYMode::Regular, plHandedness::LeftHanded);

      const plPlane horz = plGraphicsUtils::ComputeInterpolatedFrustumPlane(
        plGraphicsUtils::FrustumPlaneInterpolation::LeftToRight, i * 0.1f, mProj, plClipSpaceDepthRange::ZeroToOne);
      const plPlane vert = plGraphicsUtils::ComputeInterpolatedFrustumPlane(
        plGraphicsUtils::FrustumPlaneInterpolation::BottomToTop, i * 0.1f, mProj, plClipSpaceDepthRange::ZeroToOne);
      const plPlane forw = plGraphicsUtils::ComputeInterpolatedFrustumPlane(
        plGraphicsUtils::FrustumPlaneInterpolation::NearToFar, i * 0.1f, mProj, plClipSpaceDepthRange::ZeroToOne);

      // Generate clip space point at intersection of the 3 planes and project to worldspace
      plVec4 clipSpacePoint = plVec4(0.1f * i * 2 - 1, 0.1f * i * 2 - 1, 0.1f * i, 1);

      plVec4 worldSpacePoint = mProj.GetInverse() * clipSpacePoint;
      worldSpacePoint /= worldSpacePoint.w;

      PLASMA_TEST_FLOAT(horz.GetDistanceTo(plVec3::ZeroVector()), 0.0f, 0.01f);
      PLASMA_TEST_FLOAT(vert.GetDistanceTo(plVec3::ZeroVector()), 0.0f, 0.01f);

      if (i == 0)
      {
        PLASMA_TEST_FLOAT(forw.GetDistanceTo(plVec3::ZeroVector()), -nearPlane, 0.01f);
      }
      else if (i == 10)
      {
        PLASMA_TEST_FLOAT(forw.GetDistanceTo(plVec3::ZeroVector()), -farPlane, 0.01f);
      }

      PLASMA_TEST_FLOAT(horz.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0f, 0.02f);
      PLASMA_TEST_FLOAT(vert.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0f, 0.02f);
      PLASMA_TEST_FLOAT(forw.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0f, 0.02f);

      // this isn't interpolated linearly across the angle (rotated), so the epsilon has to be very large (just an approx test)
      PLASMA_TEST_FLOAT(horz.m_vNormal.GetAngleBetween(plVec3(1, 0, 0)).GetDegree(), plMath::Abs(-45.0f + 90.0f * i * 0.1f), 4.0f);
      PLASMA_TEST_FLOAT(vert.m_vNormal.GetAngleBetween(plVec3(0, 1, 0)).GetDegree(), plMath::Abs(-45.0f + 90.0f * i * 0.1f), 4.0f);
      PLASMA_TEST_VEC3(forw.m_vNormal, plVec3(0, 0, 1), 0.01f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CreateLookAtViewMatrix / CreateInverseLookAtViewMatrix")
  {
    for (int h = 0; h < 2; ++h)
    {
      const plHandedness::Enum handedness = (h == 0) ? plHandedness::LeftHanded : plHandedness::RightHanded;

      {
        plMat3 mLook3 = plGraphicsUtils::CreateLookAtViewMatrix(plVec3(1, 0, 0), plVec3(0, 0, 1), handedness);
        plMat3 mLookInv3 = plGraphicsUtils::CreateInverseLookAtViewMatrix(plVec3(1, 0, 0), plVec3(0, 0, 1), handedness);

        PLASMA_TEST_BOOL((mLook3 * mLookInv3).IsIdentity(0.01f));

        plMat4 mLook4 = plGraphicsUtils::CreateLookAtViewMatrix(plVec3(0), plVec3(1, 0, 0), plVec3(0, 0, 1), handedness);
        plMat4 mLookInv4 = plGraphicsUtils::CreateInverseLookAtViewMatrix(plVec3(0), plVec3(1, 0, 0), plVec3(0, 0, 1), handedness);

        PLASMA_TEST_BOOL((mLook4 * mLookInv4).IsIdentity(0.01f));

        PLASMA_TEST_BOOL(mLook3.IsEqual(mLook4.GetRotationalPart(), 0.01f));
        PLASMA_TEST_BOOL(mLookInv3.IsEqual(mLookInv4.GetRotationalPart(), 0.01f));
      }

      {
        plMat4 mLook4 = plGraphicsUtils::CreateLookAtViewMatrix(plVec3(1, 2, 0), plVec3(4, 5, 0), plVec3(0, 0, 1), handedness);
        plMat4 mLookInv4 = plGraphicsUtils::CreateInverseLookAtViewMatrix(plVec3(1, 2, 0), plVec3(4, 5, 0), plVec3(0, 0, 1), handedness);

        PLASMA_TEST_BOOL((mLook4 * mLookInv4).IsIdentity(0.01f));
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CreateViewMatrix / DecomposeViewMatrix / CreateInverseViewMatrix")
  {
    for (int h = 0; h < 2; ++h)
    {
      const plHandedness::Enum handedness = (h == 0) ? plHandedness::LeftHanded : plHandedness::RightHanded;

      const plVec3 vEye(0);
      const plVec3 vTarget(0, 0, 1);
      const plVec3 vUp0(0, 1, 0);
      const plVec3 vFwd = (vTarget - vEye).GetNormalized();
      plVec3 vRight = vUp0.CrossRH(vFwd).GetNormalized();
      const plVec3 vUp = vFwd.CrossRH(vRight).GetNormalized();

      if (handedness == plHandedness::RightHanded)
        vRight = -vRight;

      const plMat4 mLookAt = plGraphicsUtils::CreateLookAtViewMatrix(vEye, vTarget, vUp0, handedness);

      plVec3 decFwd, decRight, decUp, decPos;
      plGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, mLookAt, handedness);

      PLASMA_TEST_VEC3(decPos, vEye, 0.01f);
      PLASMA_TEST_VEC3(decFwd, vFwd, 0.01f);
      PLASMA_TEST_VEC3(decUp, vUp, 0.01f);
      PLASMA_TEST_VEC3(decRight, vRight, 0.01f);

      const plMat4 mView = plGraphicsUtils::CreateViewMatrix(decPos, decFwd, decRight, decUp, handedness);
      const plMat4 mViewInv = plGraphicsUtils::CreateInverseViewMatrix(decPos, decFwd, decRight, decUp, handedness);

      PLASMA_TEST_BOOL(mLookAt.IsEqual(mView, 0.01f));

      PLASMA_TEST_BOOL((mLookAt * mViewInv).IsIdentity());
    }
  }
}

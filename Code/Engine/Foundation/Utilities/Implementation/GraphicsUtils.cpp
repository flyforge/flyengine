#include <Foundation/FoundationPCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>

plResult plGraphicsUtils::ConvertWorldPosToScreenPos(const plMat4& mModelViewProjection, const plUInt32 uiViewportX, const plUInt32 uiViewportY, const plUInt32 uiViewportWidth, const plUInt32 uiViewportHeight, const plVec3& vPoint, plVec3& out_vScreenPos, plClipSpaceDepthRange::Enum depthRange)
{
  const plVec4 vToProject = vPoint.GetAsVec4(1.0f);

  plVec4 vClipSpace = mModelViewProjection * vToProject;

  if (vClipSpace.w == 0.0f)
    return PLASMA_FAILURE;

  plVec3 vProjected = vClipSpace.GetAsVec3() / vClipSpace.w;
  if (vClipSpace.w < 0.0f)
    vProjected.z = -vProjected.z;

  out_vScreenPos.x = uiViewportX + uiViewportWidth * ((vProjected.x * 0.5f) + 0.5f);
  out_vScreenPos.y = uiViewportY + uiViewportHeight * ((vProjected.y * 0.5f) + 0.5f);

  // normalize the output z value to always be in [0; 1] range
  // That means when the projection matrix spits out values between -1 and +1, rescale those values
  if (depthRange == plClipSpaceDepthRange::MinusOneToOne)
    out_vScreenPos.z = vProjected.z * 0.5f + 0.5f;
  else
    out_vScreenPos.z = vProjected.z;

  return PLASMA_SUCCESS;
}

plResult plGraphicsUtils::ConvertScreenPosToWorldPos(
  const plMat4& mInverseModelViewProjection, const plUInt32 uiViewportX, const plUInt32 uiViewportY, const plUInt32 uiViewportWidth, const plUInt32 uiViewportHeight, const plVec3& vScreenPos, plVec3& out_vPoint, plVec3* out_pDirection, plClipSpaceDepthRange::Enum depthRange)
{
  plVec3 vClipSpace = vScreenPos;

  // From window coordinates to [0; 1] range
  vClipSpace.x = (vClipSpace.x - uiViewportX) / uiViewportWidth;
  vClipSpace.y = (vClipSpace.y - uiViewportY) / uiViewportHeight;

  // Map to range [-1; 1]
  vClipSpace.x = vClipSpace.x * 2.0f - 1.0f;
  vClipSpace.y = vClipSpace.y * 2.0f - 1.0f;

  // The OpenGL matrix expects the z values to be between -1 and +1, so rescale the incoming value to that range
  if (depthRange == plClipSpaceDepthRange::MinusOneToOne)
    vClipSpace.z = vClipSpace.z * 2.0f - 1.0f;

  plVec4 vToUnProject = vClipSpace.GetAsVec4(1.0f);

  plVec4 vWorldSpacePoint = mInverseModelViewProjection * vToUnProject;

  if (vWorldSpacePoint.w == 0.0f)
    return PLASMA_FAILURE;

  out_vPoint = vWorldSpacePoint.GetAsVec3() / vWorldSpacePoint.w;

  if (out_pDirection != nullptr)
  {
    vToUnProject.z += 0.1f; // a point that is a bit further away

    const plVec4 vWorldSpacePoint2 = mInverseModelViewProjection * vToUnProject;

    PLASMA_ASSERT_DEV(vWorldSpacePoint2.w != 0.0f, "It should not be possible that the first projected point has a w other than zero, but the second one has!");

    const plVec3 vPoint2 = vWorldSpacePoint2.GetAsVec3() / vWorldSpacePoint2.w;

    *out_pDirection = (vPoint2 - out_vPoint).GetNormalized();
  }

  return PLASMA_SUCCESS;
}

plResult plGraphicsUtils::ConvertScreenPosToWorldPos(const plMat4d& mInverseModelViewProjection, const plUInt32 uiViewportX, const plUInt32 uiViewportY, const plUInt32 uiViewportWidth, const plUInt32 uiViewportHeight, const plVec3& vScreenPos, plVec3& out_vPoint, plVec3* out_pDirection /*= nullptr*/,
  plClipSpaceDepthRange::Enum depthRange /*= plClipSpaceDepthRange::Default*/)
{
  plVec3 vClipSpace = vScreenPos;

  // From window coordinates to [0; 1] range
  vClipSpace.x = (vClipSpace.x - uiViewportX) / uiViewportWidth;
  vClipSpace.y = (vClipSpace.y - uiViewportY) / uiViewportHeight;

  // Map to range [-1; 1]
  vClipSpace.x = vClipSpace.x * 2.0f - 1.0f;
  vClipSpace.y = vClipSpace.y * 2.0f - 1.0f;

  // The OpenGL matrix expects the z values to be between -1 and +1, so rescale the incoming value to that range
  if (depthRange == plClipSpaceDepthRange::MinusOneToOne)
    vClipSpace.z = vClipSpace.z * 2.0f - 1.0f;

  plVec4d vToUnProject = plVec4d(vClipSpace.x, vClipSpace.y, vClipSpace.z, 1.0);

  plVec4d vWorldSpacePoint = mInverseModelViewProjection * vToUnProject;

  if (vWorldSpacePoint.w == 0.0)
    return PLASMA_FAILURE;

  plVec3d outTemp = vWorldSpacePoint.GetAsVec3() / vWorldSpacePoint.w;
  out_vPoint.Set((float)outTemp.x, (float)outTemp.y, (float)outTemp.z);

  if (out_pDirection != nullptr)
  {
    vToUnProject.z += 0.1f; // a point that is a bit further away

    const plVec4d vWorldSpacePoint2 = mInverseModelViewProjection * vToUnProject;

    PLASMA_ASSERT_DEV(vWorldSpacePoint2.w != 0.0, "It should not be possible that the first projected point has a w other than zero, but the second one has!");

    const plVec3d vPoint2 = vWorldSpacePoint2.GetAsVec3() / vWorldSpacePoint2.w;

    plVec3d outDir = (vPoint2 - outTemp).GetNormalized();
    out_pDirection->Set((float)outDir.x, (float)outDir.y, (float)outDir.z);
  }

  return PLASMA_SUCCESS;
}

bool plGraphicsUtils::IsTriangleFlipRequired(const plMat3& mTransformation)
{
  return (mTransformation.GetColumn(0).CrossRH(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);
}

void plGraphicsUtils::ConvertProjectionMatrixDepthRange(plMat4& inout_mMatrix, plClipSpaceDepthRange::Enum srcDepthRange, plClipSpaceDepthRange::Enum dstDepthRange)
{
  // exclude identity transformations
  if (srcDepthRange == dstDepthRange)
    return;

  plVec4 row2 = inout_mMatrix.GetRow(2);
  plVec4 row3 = inout_mMatrix.GetRow(3);

  // only need to check SrcDepthRange, the rest is the logical conclusion from being not equal
  if (srcDepthRange == plClipSpaceDepthRange::MinusOneToOne /*&& DstDepthRange == plClipSpaceDepthRange::ZeroToOne*/)
  {
    // map z => (z + w)/2
    row2 += row3;
    row2 *= 0.5f;
  }
  else // if (SrcDepthRange == plClipSpaceDepthRange::ZeroToOne && DstDepthRange == plClipSpaceDepthRange::MinusOneToOne)
  {
    // map z => 2z - w
    row2 += row2;
    row2 -= row3;
  }


  inout_mMatrix.SetRow(2, row2);
  inout_mMatrix.SetRow(3, row3);
}

void plGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const plMat4& mProjectionMatrix, plAngle& out_fovX, plAngle& out_fovY)
{

  const plVec3 row0 = mProjectionMatrix.GetRow(0).GetAsVec3();
  const plVec3 row1 = mProjectionMatrix.GetRow(1).GetAsVec3();
  const plVec3 row3 = mProjectionMatrix.GetRow(3).GetAsVec3();

  const plVec3 leftPlane = (row3 + row0).GetNormalized();
  const plVec3 rightPlane = (row3 - row0).GetNormalized();
  const plVec3 bottomPlane = (row3 + row1).GetNormalized();
  const plVec3 topPlane = (row3 - row1).GetNormalized();

  out_fovX = plAngle::Radian(plMath::Pi<float>()) - plMath::ACos(leftPlane.Dot(rightPlane));
  out_fovY = plAngle::Radian(plMath::Pi<float>()) - plMath::ACos(topPlane.Dot(bottomPlane));
}

void plGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const plMat4& mProjectionMatrix, plAngle& out_fovLeft, plAngle& out_fovRight, plAngle& out_fovBottom, plAngle& out_fovTop, plClipSpaceYMode::Enum range)
{
  const plVec3 row0 = mProjectionMatrix.GetRow(0).GetAsVec3();
  const plVec3 row1 = mProjectionMatrix.GetRow(1).GetAsVec3();
  const plVec3 row3 = mProjectionMatrix.GetRow(3).GetAsVec3();

  const plVec3 leftPlane = (row3 + row0).GetNormalized();
  const plVec3 rightPlane = (row3 - row0).GetNormalized();
  const plVec3 bottomPlane = (row3 + row1).GetNormalized();
  const plVec3 topPlane = (row3 - row1).GetNormalized();

  out_fovLeft = -plMath::ACos(leftPlane.Dot(plVec3(1.0f, 0, 0)));
  out_fovRight = plAngle::Radian(plMath::Pi<float>()) - plMath::ACos(rightPlane.Dot(plVec3(1.0f, 0, 0)));
  out_fovBottom = -plMath::ACos(bottomPlane.Dot(plVec3(0, 1.0f, 0)));
  out_fovTop = plAngle::Radian(plMath::Pi<float>()) - plMath::ACos(topPlane.Dot(plVec3(0, 1.0f, 0)));

  if (range == plClipSpaceYMode::Flipped)
    plMath::Swap(out_fovBottom, out_fovTop);
}

plResult plGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const plMat4& mProjectionMatrix, float& out_fLeft, float& out_fRight, float& out_fBottom, float& out_fTop, plClipSpaceDepthRange::Enum depthRange, plClipSpaceYMode::Enum range)
{
  float fNear, fFar;
  PLASMA_SUCCEED_OR_RETURN(ExtractNearAndFarClipPlaneDistances(fNear, fFar, mProjectionMatrix, depthRange));
  // Compensate for inverse-Z.
  const float fMinDepth = plMath::Min(fNear, fFar);

  plAngle fFovLeft;
  plAngle fFovRight;
  plAngle fFovBottom;
  plAngle fFovTop;
  ExtractPerspectiveMatrixFieldOfView(mProjectionMatrix, fFovLeft, fFovRight, fFovBottom, fFovTop, range);

  out_fLeft = plMath::Tan(fFovLeft) * fMinDepth;
  out_fRight = plMath::Tan(fFovRight) * fMinDepth;
  out_fBottom = plMath::Tan(fFovBottom) * fMinDepth;
  out_fTop = plMath::Tan(fFovTop) * fMinDepth;
  return PLASMA_SUCCESS;
}

plResult plGraphicsUtils::ExtractNearAndFarClipPlaneDistances(float& out_fNear, float& out_fFar, const plMat4& mProjectionMatrix, plClipSpaceDepthRange::Enum depthRange)
{
  const plVec4 row2 = mProjectionMatrix.GetRow(2);
  const plVec4 row3 = mProjectionMatrix.GetRow(3);

  plVec4 nearPlane = row2;

  if (depthRange == plClipSpaceDepthRange::MinusOneToOne)
  {
    nearPlane += row3;
  }

  const plVec4 farPlane = row3 - row2;

  const float nearLength = nearPlane.GetAsVec3().GetLength();
  const float farLength = farPlane.GetAsVec3().GetLength();

  const float nearW = plMath::Abs(nearPlane.w);
  const float farW = plMath::Abs(farPlane.w);

  if ((nearLength < plMath::SmallEpsilon<float>() && farLength < plMath::SmallEpsilon<float>()) ||
      nearW < plMath::SmallEpsilon<float>() || farW < plMath::SmallEpsilon<float>())
  {
    return PLASMA_FAILURE;
  }

  const float fNear = nearW / nearLength;
  const float fFar = farW / farLength;

  if (plMath::IsEqual(fNear, fFar, plMath::SmallEpsilon<float>()))
  {
    return PLASMA_FAILURE;
  }

  out_fNear = fNear;
  out_fFar = fFar;

  return PLASMA_SUCCESS;
}

plPlane plGraphicsUtils::ComputeInterpolatedFrustumPlane(FrustumPlaneInterpolation direction, float fLerpFactor, const plMat4& mProjectionMatrix, plClipSpaceDepthRange::Enum depthRange)
{
  plVec4 rowA;
  plVec4 rowB = mProjectionMatrix.GetRow(3);
  const float factorMinus1to1 = (fLerpFactor - 0.5f) * 2.0f; // bring into [-1; +1] range

  switch (direction)
  {
    case FrustumPlaneInterpolation::LeftToRight:
    {
      rowA = mProjectionMatrix.GetRow(0);
      rowB *= factorMinus1to1;
      break;
    }

    case FrustumPlaneInterpolation::BottomToTop:
    {
      rowA = mProjectionMatrix.GetRow(1);
      rowB *= factorMinus1to1;
      break;
    }

    case FrustumPlaneInterpolation::NearToFar:
      rowA = mProjectionMatrix.GetRow(2);

      if (depthRange == plClipSpaceDepthRange::ZeroToOne)
        rowB *= fLerpFactor; // [0; 1] range
      else
        rowB *= factorMinus1to1;
      break;
  }

  plPlane res;
  res.m_vNormal = rowA.GetAsVec3() - rowB.GetAsVec3();
  res.m_fNegDistance = (rowA.w - rowB.w) / res.m_vNormal.GetLengthAndNormalize();

  return res;
}

plMat4 plGraphicsUtils::CreatePerspectiveProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, plClipSpaceDepthRange::Enum depthRange, plClipSpaceYMode::Enum range, plHandedness::Enum handedness)
{
  const float vw = fViewWidth * 0.5f;
  const float vh = fViewHeight * 0.5f;

  return CreatePerspectiveProjectionMatrix(-vw, vw, -vh, vh, fNearZ, fFarZ, depthRange, range, handedness);
}

plMat4 plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(plAngle fieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, plClipSpaceDepthRange::Enum depthRange, plClipSpaceYMode::Enum range, plHandedness::Enum handedness)
{
  // Taking the minimum allows the function to be used to create
  // inverse z matrices (fNearZ > fFarZ) as well.
  const float xm = plMath::Min(fNearZ, fFarZ) * plMath::Tan(fieldOfViewX * 0.5f);
  const float ym = xm / fAspectRatioWidthDivHeight;

  return CreatePerspectiveProjectionMatrix(-xm, xm, -ym, ym, fNearZ, fFarZ, depthRange, range, handedness);
}

plMat4 plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(plAngle fieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, plClipSpaceDepthRange::Enum depthRange, plClipSpaceYMode::Enum range, plHandedness::Enum handedness)
{
  // Taking the minimum allows the function to be used to create
  // inverse z matrices (fNearZ > fFarZ) as well.
  const float ym = plMath::Min(fNearZ, fFarZ) * plMath::Tan(fieldOfViewY * 0.5);
  const float xm = ym * fAspectRatioWidthDivHeight;

  return CreatePerspectiveProjectionMatrix(-xm, xm, -ym, ym, fNearZ, fFarZ, depthRange, range, handedness);
}

plMat4 plGraphicsUtils::CreateOrthographicProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, plClipSpaceDepthRange::Enum depthRange, plClipSpaceYMode::Enum range, plHandedness::Enum handedness)
{
  return CreateOrthographicProjectionMatrix(-fViewWidth * 0.5f, fViewWidth * 0.5f, -fViewHeight * 0.5f, fViewHeight * 0.5f, fNearZ, fFarZ, depthRange, range, handedness);
}

plMat4 plGraphicsUtils::CreateOrthographicProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, plClipSpaceDepthRange::Enum depthRange, plClipSpaceYMode::Enum range, plHandedness::Enum handedness)
{
  PLASMA_ASSERT_DEBUG(plMath::IsFinite(fNearZ) && plMath::IsFinite(fFarZ), "Infinite plane values are not supported for orthographic projections!");

  plMat4 res;
  res.SetIdentity();

  if (range == plClipSpaceYMode::Flipped)
  {
    plMath::Swap(fBottom, fTop);
  }

  const float fOneDivFarMinusNear = 1.0f / (fFarZ - fNearZ);
  const float fOneDivRightMinusLeft = 1.0f / (fRight - fLeft);
  const float fOneDivTopMinusBottom = 1.0f / (fTop - fBottom);

  res.Element(0, 0) = 2.0f / (fRight - fLeft);

  res.Element(1, 1) = 2.0f / (fTop - fBottom);

  res.Element(3, 0) = -(fLeft + fRight) * fOneDivRightMinusLeft;
  res.Element(3, 1) = -(fTop + fBottom) * fOneDivTopMinusBottom;


  if (depthRange == plClipSpaceDepthRange::MinusOneToOne)
  {
    // The OpenGL Way: http://wiki.delphigl.com/index.php/glFrustum
    res.Element(2, 2) = -2.0f * fOneDivFarMinusNear;
    res.Element(3, 2) = -(fFarZ + fNearZ) * fOneDivFarMinusNear;
  }
  else
  {
    // The Left-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixorthooffcenterlh
    // The Right-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixorthooffcenterrh

    res.Element(2, 2) = -1.0f * fOneDivFarMinusNear;
    res.Element(3, 2) = -fNearZ * fOneDivFarMinusNear;
  }

  if (handedness == plHandedness::LeftHanded)
  {
    res.SetColumn(2, -res.GetColumn(2));
  }

  return res;
}

plMat4 plGraphicsUtils::CreatePerspectiveProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, plClipSpaceDepthRange::Enum depthRange, plClipSpaceYMode::Enum range, plHandedness::Enum handedness)
{
  PLASMA_ASSERT_DEBUG(plMath::IsFinite(fNearZ) || plMath::IsFinite(fFarZ), "fNearZ and fFarZ cannot both be infinite at the same time!");

  plMat4 res;
  res.SetZero();

  if (range == plClipSpaceYMode::Flipped)
  {
    plMath::Swap(fBottom, fTop);
  }

  // Taking the minimum of the two plane values allows
  // this function to also be used to create inverse-z
  // matrices by specifying values of fNearZ > fFarZ.
  // Otherwise the x and y scaling values will be wrong
  // in the final matrix.
  const float fMinPlane = plMath::Min(fNearZ, fFarZ);
  const float fTwoNearZ = fMinPlane + fMinPlane;
  const float fOneDivRightMinusLeft = 1.0f / (fRight - fLeft);
  const float fOneDivTopMinusBottom = 1.0f / (fTop - fBottom);

  res.Element(0, 0) = fTwoNearZ * fOneDivRightMinusLeft;

  res.Element(1, 1) = fTwoNearZ * fOneDivTopMinusBottom;

  res.Element(2, 0) = (fLeft + fRight) * fOneDivRightMinusLeft;
  res.Element(2, 1) = (fTop + fBottom) * fOneDivTopMinusBottom;
  res.Element(2, 3) = -1.0f;

  // If either fNearZ or fFarZ is infinite, one can derive the resulting z-transformation by using limit math
  // and letting the respective variable approach infinity in the original expressions for P(2, 2) and P(3, 2).
  // The result is that a couple of terms from the original fraction get reduced to 0 by being divided by infinity,
  // which fortunately yields 1) finite and 2) much simpler expressions for P(2, 2) and P(3, 2).
  if (depthRange == plClipSpaceDepthRange::MinusOneToOne)
  {
    // The OpenGL Way: http://wiki.delphigl.com/index.php/glFrustum
    // Algebraically reordering the z-row fractions from the above source in a way so infinite fNearZ or fFarZ will zero out
    // instead of producing NaNs due to inf/inf divisions will yield these generalized formulas which could be used instead
    // of the branching below. Insert infinity for either fNearZ or fFarZ to see that these will yield exactly these simplifications:
    //res.Element(2, 2) = 1.f / (fNearZ / fFarZ - 1.f) + 1.f / (1.f - fFarZ / fNearZ);
    //res.Element(3, 2) = 2.f / (1.f / fFarZ - 1.f / fNearZ);
    if (!plMath::IsFinite(fNearZ))
    {
      res.Element(2, 2) = 1.f;
      res.Element(3, 2) = 2.f * fFarZ;
    }
    else if (!plMath::IsFinite(fFarZ))
    {
      res.Element(2, 2) = -1.f;
      res.Element(3, 2) = -2.f * fNearZ;
    }
    else
    {
      const float fOneDivNearMinusFar = 1.0f / (fNearZ - fFarZ);
      res.Element(2, 2) = (fFarZ + fNearZ) * fOneDivNearMinusFar;
      res.Element(3, 2) = 2 * fFarZ * fNearZ * fOneDivNearMinusFar;
    }
  }
  else
  {
    // The Left-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixperspectiveoffcenterlh
    // The Right-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixperspectiveoffcenterrh
    // Algebraically reordering the z-row fractions from the above source in a way so infinite fNearZ or fFarZ will zero out
    // instead of producing NaNs due to inf/inf divisions will yield these generalized formulas which could be used instead
    // of the branching below. Insert infinity for either fNearZ or fFarZ to see that these will yield exactly these simplifications:
    //res.Element(2, 2) = 1.f / (fNearZ / fFarZ - 1.f);
    //res.Element(3, 2) = 1.f / (1.f / fFarZ - 1.f / fNearZ);
    if (!plMath::IsFinite(fNearZ))
    {
      res.Element(2, 2) = 0.f;
      res.Element(3, 2) = fFarZ;
    }
    else if (!plMath::IsFinite(fFarZ))
    {
      res.Element(2, 2) = -1.f;
      res.Element(3, 2) = -fNearZ;
    }
    else
    {
      const float fOneDivNearMinusFar = 1.0f / (fNearZ - fFarZ);
      res.Element(2, 2) = fFarZ * fOneDivNearMinusFar;
      res.Element(3, 2) = fFarZ * fNearZ * fOneDivNearMinusFar;
    }
  }

  if (handedness == plHandedness::LeftHanded)
  {
    res.SetColumn(2, -res.GetColumn(2));
  }

  return res;
}

plMat3 plGraphicsUtils::CreateLookAtViewMatrix(const plVec3& vTarget, const plVec3& vUpDir, plHandedness::Enum handedness)
{
  PLASMA_ASSERT_DEBUG(!vTarget.IsZero(), "The target must not be at the origin.");

  plVec3 vLookDir = vTarget;
  vLookDir.NormalizeIfNotZero(plVec3::UnitXAxis()).IgnoreResult();

  plVec3 vNormalizedUpDir = vUpDir.GetNormalized();

  if (plMath::Abs(vLookDir.Dot(vNormalizedUpDir)) > 0.9999f) // less than 1 degree difference -> problem
  {
    // use some arbitrary other orthogonal vector as UP
    vNormalizedUpDir = vLookDir.GetOrthogonalVector();
  }

  plMat3 res;

  const plVec3 zaxis = (handedness == plHandedness::RightHanded) ? -vLookDir : vLookDir;
  const plVec3 xaxis = vNormalizedUpDir.CrossRH(zaxis).GetNormalized();
  const plVec3 yaxis = zaxis.CrossRH(xaxis);

  res.SetRow(0, xaxis);
  res.SetRow(1, yaxis);
  res.SetRow(2, zaxis);

  return res;
}

plMat3 plGraphicsUtils::CreateInverseLookAtViewMatrix(const plVec3& vTarget, const plVec3& vUpDir, plHandedness::Enum handedness)
{
  PLASMA_ASSERT_DEBUG(!vTarget.IsZero(), "The target must not be at the origin.");

  plVec3 vLookDir = vTarget;
  vLookDir.NormalizeIfNotZero(plVec3::UnitXAxis()).IgnoreResult();

  plVec3 vNormalizedUpDir = vUpDir.GetNormalized();

  if (plMath::Abs(vLookDir.Dot(vNormalizedUpDir)) > 0.9999f) // less than 1 degree difference -> problem
  {
    // use some arbitrary other orthogonal vector as UP
    vNormalizedUpDir = vLookDir.GetOrthogonalVector();
  }

  plMat3 res;

  const plVec3 zaxis = (handedness == plHandedness::RightHanded) ? -vLookDir : vLookDir;
  const plVec3 xaxis = vNormalizedUpDir.CrossRH(zaxis).GetNormalized();
  const plVec3 yaxis = zaxis.CrossRH(xaxis);

  res.SetColumn(0, xaxis);
  res.SetColumn(1, yaxis);
  res.SetColumn(2, zaxis);

  return res;
}

plMat4 plGraphicsUtils::CreateLookAtViewMatrix(const plVec3& vEyePos, const plVec3& vLookAtPos, const plVec3& vUpDir, plHandedness::Enum handedness)
{
  const plMat3 rotation = CreateLookAtViewMatrix(vLookAtPos - vEyePos, vUpDir, handedness);

  plMat4 res;
  res.SetRotationalPart(rotation);
  res.SetTranslationVector(rotation * -vEyePos);
  res.SetRow(3, plVec4(0, 0, 0, 1));
  return res;
}

plMat4 plGraphicsUtils::CreateInverseLookAtViewMatrix(const plVec3& vEyePos, const plVec3& vLookAtPos, const plVec3& vUpDir, plHandedness::Enum handedness)
{
  const plMat3 rotation = CreateInverseLookAtViewMatrix(vLookAtPos - vEyePos, vUpDir, handedness);

  plMat4 res;
  res.SetRotationalPart(rotation);
  res.SetTranslationVector(vEyePos);
  res.SetRow(3, plVec4(0, 0, 0, 1));
  return res;
}

plMat4 plGraphicsUtils::CreateViewMatrix(const plVec3& vPosition, const plVec3& vForwardDir, const plVec3& vRightDir, const plVec3& vUpDir, plHandedness::Enum handedness)
{
  plMat4 res;
  res.SetIdentity();

  plVec3 xaxis, yaxis, zaxis;

  if (handedness == plHandedness::LeftHanded)
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = vForwardDir;
  }
  else
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = -vForwardDir;
  }

  res.SetRow(0, xaxis.GetAsVec4(0));
  res.SetRow(1, yaxis.GetAsVec4(0));
  res.SetRow(2, zaxis.GetAsVec4(0));
  res.SetTranslationVector(plVec3(-xaxis.Dot(vPosition), -yaxis.Dot(vPosition), -zaxis.Dot(vPosition)));

  return res;
}

plMat4 plGraphicsUtils::CreateInverseViewMatrix(const plVec3& vPosition, const plVec3& vForwardDir, const plVec3& vRightDir, const plVec3& vUpDir, plHandedness::Enum handedness)
{
  plMat4 res;
  res.SetIdentity();

  plVec3 xaxis, yaxis, zaxis;

  if (handedness == plHandedness::LeftHanded)
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = vForwardDir;
  }
  else
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = -vForwardDir;
  }

  res.SetColumn(0, xaxis.GetAsVec4(0));
  res.SetColumn(1, yaxis.GetAsVec4(0));
  res.SetColumn(2, zaxis.GetAsVec4(0));
  res.SetTranslationVector(vPosition);

  return res;
}

void plGraphicsUtils::DecomposeViewMatrix(plVec3& ref_vPosition, plVec3& ref_vForwardDir, plVec3& ref_vRightDir, plVec3& ref_vUpDir, const plMat4& mViewMatrix, plHandedness::Enum handedness)
{
  const plMat3 rotation = mViewMatrix.GetRotationalPart();

  if (handedness == plHandedness::LeftHanded)
  {
    ref_vRightDir = rotation.GetRow(0);
    ref_vUpDir = rotation.GetRow(1);
    ref_vForwardDir = rotation.GetRow(2);
  }
  else
  {
    ref_vRightDir = rotation.GetRow(0);
    ref_vUpDir = rotation.GetRow(1);
    ref_vForwardDir = -rotation.GetRow(2);
  }

  ref_vPosition = rotation.GetTranspose() * -mViewMatrix.GetTranslationVector();
}

plResult plGraphicsUtils::ComputeBarycentricCoordinates(plVec3& out_vCoordinates, const plVec3& a, const plVec3& b, const plVec3& c, const plVec3& p)
{
  // implementation copied from https://gamedev.stackexchange.com/a/49370

  const plVec3 v0 = b - a;
  const plVec3 v1 = c - a;
  const plVec3 v2 = p - a;

  const float d00 = v0.Dot(v0);
  const float d01 = v0.Dot(v1);
  const float d11 = v1.Dot(v1);
  const float d20 = v2.Dot(v0);
  const float d21 = v2.Dot(v1);
  const float denom = d00 * d11 - d01 * d01;

  if (plMath::IsZero(denom, plMath::SmallEpsilon<float>()))
    return PLASMA_FAILURE;

  const float invDenom = 1.0f / denom;

  const float v = (d11 * d20 - d01 * d21) * invDenom;
  const float w = (d00 * d21 - d01 * d20) * invDenom;
  const float u = 1.0f - v - w;

  out_vCoordinates.Set(u, v, w);

  return PLASMA_SUCCESS;
}

plResult plGraphicsUtils::ComputeBarycentricCoordinates(plVec3& out_vCoordinates, const plVec2& a, const plVec2& b, const plVec2& c, const plVec2& p)
{
  // implementation copied from https://gamedev.stackexchange.com/a/63203

  const plVec2 v0 = b - a;
  const plVec2 v1 = c - a;
  const plVec2 v2 = p - a;

  const float denom = v0.x * v1.y - v1.x * v0.y;

  if (plMath::IsZero(denom, plMath::SmallEpsilon<float>()))
    return PLASMA_FAILURE;

  const float invDenom = 1.0f / denom;
  const float v = (v2.x * v1.y - v1.x * v2.y) * invDenom;
  const float w = (v0.x * v2.y - v2.x * v0.y) * invDenom;
  const float u = 1.0f - v - w;

  out_vCoordinates.Set(u, v, w);

  return PLASMA_SUCCESS;
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_GraphicsUtils);

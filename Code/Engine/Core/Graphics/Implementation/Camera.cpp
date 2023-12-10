#include <Core/CorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/CoordinateSystem.h>
#include <Foundation/Utilities/GraphicsUtils.h>

class RemapCoordinateSystemProvider : public plCoordinateSystemProvider
{
public:
  RemapCoordinateSystemProvider()
    : plCoordinateSystemProvider(nullptr)
  {
  }

  virtual void GetCoordinateSystem(const plVec3& vGlobalPosition, plCoordinateSystem& out_CoordinateSystem) const override
  {
    out_CoordinateSystem.m_vForwardDir = plBasisAxis::GetBasisVector(m_ForwardAxis);
    out_CoordinateSystem.m_vRightDir = plBasisAxis::GetBasisVector(m_RightAxis);
    out_CoordinateSystem.m_vUpDir = plBasisAxis::GetBasisVector(m_UpAxis);
  }

  plBasisAxis::Enum m_ForwardAxis = plBasisAxis::PositiveX;
  plBasisAxis::Enum m_RightAxis = plBasisAxis::PositiveY;
  plBasisAxis::Enum m_UpAxis = plBasisAxis::PositiveZ;
};

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plCameraMode, 1)
  PLASMA_ENUM_CONSTANT(plCameraMode::PerspectiveFixedFovX),
  PLASMA_ENUM_CONSTANT(plCameraMode::PerspectiveFixedFovY),
  PLASMA_ENUM_CONSTANT(plCameraMode::OrthoFixedWidth),
  PLASMA_ENUM_CONSTANT(plCameraMode::OrthoFixedHeight),
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

plCamera::plCamera()
{
  m_vCameraPosition[0].SetZero();
  m_vCameraPosition[1].SetZero();
  m_mViewMatrix[0].SetIdentity();
  m_mViewMatrix[1].SetIdentity();
  m_mStereoProjectionMatrix[0].SetIdentity();
  m_mStereoProjectionMatrix[1].SetIdentity();

  SetCoordinateSystem(plBasisAxis::PositiveX, plBasisAxis::PositiveY, plBasisAxis::PositiveZ);
}

void plCamera::SetCoordinateSystem(plBasisAxis::Enum forwardAxis, plBasisAxis::Enum rightAxis, plBasisAxis::Enum upAxis)
{
  auto provider = PLASMA_DEFAULT_NEW(RemapCoordinateSystemProvider);
  provider->m_ForwardAxis = forwardAxis;
  provider->m_RightAxis = rightAxis;
  provider->m_UpAxis = upAxis;

  m_pCoordinateSystem = provider;
}

void plCamera::SetCoordinateSystem(const plSharedPtr<plCoordinateSystemProvider>& provider)
{
  m_pCoordinateSystem = provider;
}

plVec3 plCamera::GetPosition(plCameraEye eye) const
{
  return MapInternalToExternal(m_vCameraPosition[static_cast<int>(eye)]);
}

plVec3 plCamera::GetDirForwards(plCameraEye eye) const
{
  plVec3 decFwd, decRight, decUp, decPos;
  plGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], plHandedness::LeftHanded);

  return MapInternalToExternal(decFwd);
}

plVec3 plCamera::GetDirUp(plCameraEye eye) const
{
  plVec3 decFwd, decRight, decUp, decPos;
  plGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], plHandedness::LeftHanded);

  return MapInternalToExternal(decUp);
}

plVec3 plCamera::GetDirRight(plCameraEye eye) const
{
  plVec3 decFwd, decRight, decUp, decPos;
  plGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], plHandedness::LeftHanded);

  return MapInternalToExternal(decRight);
}

plVec3 plCamera::InternalGetPosition(plCameraEye eye) const
{
  return m_vCameraPosition[static_cast<int>(eye)];
}

plVec3 plCamera::InternalGetDirForwards(plCameraEye eye) const
{
  plVec3 decFwd, decRight, decUp, decPos;
  plGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], plHandedness::LeftHanded);

  return decFwd;
}

plVec3 plCamera::InternalGetDirUp(plCameraEye eye) const
{
  plVec3 decFwd, decRight, decUp, decPos;
  plGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], plHandedness::LeftHanded);

  return decUp;
}

plVec3 plCamera::InternalGetDirRight(plCameraEye eye) const
{
  plVec3 decFwd, decRight, decUp, decPos;
  plGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], plHandedness::LeftHanded);

  return -decRight;
}

plVec3 plCamera::MapExternalToInternal(const plVec3& v) const
{
  if (m_pCoordinateSystem)
  {
    plCoordinateSystem system;
    m_pCoordinateSystem->GetCoordinateSystem(m_vCameraPosition[0], system);

    plMat3 m;
    m.SetRow(0, system.m_vForwardDir);
    m.SetRow(1, system.m_vRightDir);
    m.SetRow(2, system.m_vUpDir);

    return m * v;
  }

  return v;
}

plVec3 plCamera::MapInternalToExternal(const plVec3& v) const
{
  if (m_pCoordinateSystem)
  {
    plCoordinateSystem system;
    m_pCoordinateSystem->GetCoordinateSystem(m_vCameraPosition[0], system);

    plMat3 m;
    m.SetColumn(0, system.m_vForwardDir);
    m.SetColumn(1, system.m_vRightDir);
    m.SetColumn(2, system.m_vUpDir);

    return m * v;
  }

  return v;
}

plAngle plCamera::GetFovX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == plCameraMode::PerspectiveFixedFovX)
    return plAngle::MakeFromDegree(m_fFovOrDim);

  if (m_Mode == plCameraMode::PerspectiveFixedFovY)
    return plMath::ATan(plMath::Tan(plAngle::MakeFromDegree(m_fFovOrDim) * 0.5f) * fAspectRatioWidthDivHeight) * 2.0f;

  // TODO: HACK
  if (m_Mode == plCameraMode::Stereo)
    return plAngle::MakeFromDegree(90);

  PLASMA_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return plAngle();
}

plAngle plCamera::GetFovY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == plCameraMode::PerspectiveFixedFovX)
    return plMath::ATan(plMath::Tan(plAngle::MakeFromDegree(m_fFovOrDim) * 0.5f) / fAspectRatioWidthDivHeight) * 2.0f;

  if (m_Mode == plCameraMode::PerspectiveFixedFovY)
    return plAngle::MakeFromDegree(m_fFovOrDim);

  // TODO: HACK
  if (m_Mode == plCameraMode::Stereo)
    return plAngle::MakeFromDegree(90);

  PLASMA_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return plAngle();
}


float plCamera::GetDimensionX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == plCameraMode::OrthoFixedWidth)
    return m_fFovOrDim;

  if (m_Mode == plCameraMode::OrthoFixedHeight)
    return m_fFovOrDim * fAspectRatioWidthDivHeight;

  PLASMA_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
  return 0;
}


float plCamera::GetDimensionY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == plCameraMode::OrthoFixedWidth)
    return m_fFovOrDim / fAspectRatioWidthDivHeight;

  if (m_Mode == plCameraMode::OrthoFixedHeight)
    return m_fFovOrDim;

  PLASMA_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
  return 0;
}

void plCamera::SetCameraMode(plCameraMode::Enum Mode, float fFovOrDim, float fNearPlane, float fFarPlane)
{
  // early out if no change
  if (m_Mode == Mode && m_fFovOrDim == fFovOrDim && m_fNearPlane == fNearPlane && m_fFarPlane == fFarPlane)
  {
    return;
  }

  m_Mode = Mode;
  m_fFovOrDim = fFovOrDim;
  m_fNearPlane = fNearPlane;
  m_fFarPlane = fFarPlane;

  m_fAspectOfPrecomputedStereoProjection = -1.0f;

  CameraSettingsChanged();
}

void plCamera::SetStereoProjection(const plMat4& mProjectionLeftEye, const plMat4& mProjectionRightEye, float fAspectRatioWidthDivHeight)
{
  m_mStereoProjectionMatrix[static_cast<int>(plCameraEye::Left)] = mProjectionLeftEye;
  m_mStereoProjectionMatrix[static_cast<int>(plCameraEye::Right)] = mProjectionRightEye;
  m_fAspectOfPrecomputedStereoProjection = fAspectRatioWidthDivHeight;

  CameraSettingsChanged();
}

void plCamera::LookAt(const plVec3& vCameraPos0, const plVec3& vTargetPos0, const plVec3& vUp0)
{
  const plVec3 vCameraPos = MapExternalToInternal(vCameraPos0);
  const plVec3 vTargetPos = MapExternalToInternal(vTargetPos0);
  const plVec3 vUp = MapExternalToInternal(vUp0);

  if (m_Mode == plCameraMode::Stereo)
  {
    PLASMA_REPORT_FAILURE("plCamera::LookAt is not possible for stereo cameras.");
    return;
  }

  m_mViewMatrix[0] = plGraphicsUtils::CreateLookAtViewMatrix(vCameraPos, vTargetPos, vUp, plHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];
  m_vCameraPosition[1] = m_vCameraPosition[0] = vCameraPos;

  CameraOrientationChanged(true, true);
}

void plCamera::SetViewMatrix(const plMat4& mLookAtMatrix, plCameraEye eye)
{
  const int iEyeIdx = static_cast<int>(eye);

  m_mViewMatrix[iEyeIdx] = mLookAtMatrix;

  plVec3 decFwd, decRight, decUp;
  plGraphicsUtils::DecomposeViewMatrix(
    m_vCameraPosition[iEyeIdx], decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], plHandedness::LeftHanded);

  if (m_Mode != plCameraMode::Stereo)
  {
    m_mViewMatrix[1 - iEyeIdx] = m_mViewMatrix[iEyeIdx];
    m_vCameraPosition[1 - iEyeIdx] = m_vCameraPosition[iEyeIdx];
  }

  CameraOrientationChanged(true, true);
}

void plCamera::GetProjectionMatrix(float fAspectRatioWidthDivHeight, plMat4& out_projectionMatrix, plCameraEye eye, plClipSpaceDepthRange::Enum depthRange) const
{
  switch (m_Mode)
  {
    case plCameraMode::PerspectiveFixedFovX:
      out_projectionMatrix = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(plAngle::MakeFromDegree(m_fFovOrDim), fAspectRatioWidthDivHeight,
        m_fNearPlane, m_fFarPlane, depthRange, plClipSpaceYMode::Regular, plHandedness::LeftHanded);
      break;

    case plCameraMode::PerspectiveFixedFovY:
      out_projectionMatrix = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(plAngle::MakeFromDegree(m_fFovOrDim), fAspectRatioWidthDivHeight,
        m_fNearPlane, m_fFarPlane, depthRange, plClipSpaceYMode::Regular, plHandedness::LeftHanded);
      break;

    case plCameraMode::OrthoFixedWidth:
      out_projectionMatrix = plGraphicsUtils::CreateOrthographicProjectionMatrix(m_fFovOrDim, m_fFovOrDim / fAspectRatioWidthDivHeight, m_fNearPlane,
        m_fFarPlane, depthRange, plClipSpaceYMode::Regular, plHandedness::LeftHanded);
      break;

    case plCameraMode::OrthoFixedHeight:
      out_projectionMatrix = plGraphicsUtils::CreateOrthographicProjectionMatrix(m_fFovOrDim * fAspectRatioWidthDivHeight, m_fFovOrDim, m_fNearPlane,
        m_fFarPlane, depthRange, plClipSpaceYMode::Regular, plHandedness::LeftHanded);
      break;

    case plCameraMode::Stereo:
      if (plMath::IsEqual(m_fAspectOfPrecomputedStereoProjection, fAspectRatioWidthDivHeight, plMath::LargeEpsilon<float>()))
        out_projectionMatrix = m_mStereoProjectionMatrix[static_cast<int>(eye)];
      else
      {
        // Evade to FixedFovY
        out_projectionMatrix = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(plAngle::MakeFromDegree(m_fFovOrDim), fAspectRatioWidthDivHeight,
          m_fNearPlane, m_fFarPlane, depthRange, plClipSpaceYMode::Regular, plHandedness::LeftHanded);
      }
      break;

    default:
      PLASMA_REPORT_FAILURE("Invalid Camera Mode {0}", (int)m_Mode);
  }
}

void plCamera::CameraSettingsChanged()
{
  PLASMA_ASSERT_DEV(m_Mode != plCameraMode::None, "Invalid Camera Mode.");
  PLASMA_ASSERT_DEV(m_fNearPlane < m_fFarPlane, "Near and Far Plane are invalid.");
  PLASMA_ASSERT_DEV(m_fFovOrDim > 0.0f, "FOV or Camera Dimension is invalid.");

  ++m_uiSettingsModificationCounter;
}

void plCamera::MoveLocally(float fForward, float fRight, float fUp)
{
  m_mViewMatrix[0].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector() - plVec3(fRight, fUp, fForward));
  m_mViewMatrix[1].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector());

  plVec3 decFwd, decRight, decUp, decPos;
  plGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[0], plHandedness::LeftHanded);

  m_vCameraPosition[0] = m_vCameraPosition[1] = decPos;

  CameraOrientationChanged(true, false);
}

void plCamera::MoveGlobally(float fForward, float fRight, float fUp)
{
  plVec3 vMove(fForward, fRight, fUp);

  plVec3 decFwd, decRight, decUp, decPos;
  plGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[0], plHandedness::LeftHanded);

  m_vCameraPosition[0] += vMove;
  m_vCameraPosition[1] = m_vCameraPosition[0];

  m_mViewMatrix[0] = plGraphicsUtils::CreateViewMatrix(m_vCameraPosition[0], decFwd, decRight, decUp, plHandedness::LeftHanded);

  m_mViewMatrix[1].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector());

  CameraOrientationChanged(true, false);
}

void plCamera::ClampRotationAngles(bool bLocalSpace, plAngle& forwardAxis, plAngle& rightAxis, plAngle& upAxis)
{
  if (bLocalSpace)
  {
    if (rightAxis.GetRadian() != 0.0f)
    {
      // Limit how much the camera can look up and down, to prevent it from overturning

      const float fDot = InternalGetDirForwards().Dot(plVec3(0, 0, -1));
      const plAngle fCurAngle = plMath::ACos(fDot) - plAngle::MakeFromDegree(90.0f);
      const plAngle fNewAngle = fCurAngle + rightAxis;

      const plAngle fAllowedAngle = plMath::Clamp(fNewAngle, plAngle::MakeFromDegree(-85.0f), plAngle::MakeFromDegree(85.0f));

      rightAxis = fAllowedAngle - fCurAngle;
    }
  }
}

void plCamera::RotateLocally(plAngle forwardAxis, plAngle rightAxis, plAngle upAxis)
{
  ClampRotationAngles(true, forwardAxis, rightAxis, upAxis);

  plVec3 vDirForwards = InternalGetDirForwards();
  plVec3 vDirUp = InternalGetDirUp();
  plVec3 vDirRight = InternalGetDirRight();

  if (forwardAxis.GetRadian() != 0.0f)
  {
    plMat3 m = plMat3::MakeAxisRotation(vDirForwards, forwardAxis);

    vDirUp = m * vDirUp;
    vDirRight = m * vDirRight;
  }

  if (rightAxis.GetRadian() != 0.0f)
  {
    plMat3 m = plMat3::MakeAxisRotation(vDirRight, rightAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (upAxis.GetRadian() != 0.0f)
  {
    plMat3 m = plMat3::MakeAxisRotation(vDirUp, upAxis);

    vDirRight = m * vDirRight;
    vDirForwards = m * vDirForwards;
  }

  // Using plGraphicsUtils::CreateLookAtViewMatrix is not only easier, it also has the advantage that we end up always with orthonormal
  // vectors.
  auto vPos = InternalGetPosition();
  m_mViewMatrix[0] = plGraphicsUtils::CreateLookAtViewMatrix(vPos, vPos + vDirForwards, vDirUp, plHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];

  CameraOrientationChanged(false, true);
}

void plCamera::RotateGlobally(plAngle forwardAxis, plAngle rightAxis, plAngle upAxis)
{
  ClampRotationAngles(false, forwardAxis, rightAxis, upAxis);

  plVec3 vDirForwards = InternalGetDirForwards();
  plVec3 vDirUp = InternalGetDirUp();

  if (forwardAxis.GetRadian() != 0.0f)
  {
    plMat3 m = plMat3 ::MakeRotationX(forwardAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (rightAxis.GetRadian() != 0.0f)
  {
    plMat3 m = plMat3 ::MakeRotationY(rightAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (upAxis.GetRadian() != 0.0f)
  {
    plMat3 m = plMat3 ::MakeRotationZ(upAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  // Using plGraphicsUtils::CreateLookAtViewMatrix is not only easier, it also has the advantage that we end up always with orthonormal
  // vectors.
  auto vPos = InternalGetPosition();
  m_mViewMatrix[0] = plGraphicsUtils::CreateLookAtViewMatrix(vPos, vPos + vDirForwards, vDirUp, plHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];

  CameraOrientationChanged(false, true);
}



PLASMA_STATICLINK_FILE(Core, Core_Graphics_Implementation_Camera);

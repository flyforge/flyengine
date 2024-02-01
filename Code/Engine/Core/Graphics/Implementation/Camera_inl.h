#pragma once

inline plVec3 plCamera::GetCenterPosition() const
{
  if (m_Mode == plCameraMode::Stereo)
    return (GetPosition(plCameraEye::Left) + GetPosition(plCameraEye::Right)) * 0.5f;
  else
    return GetPosition();
}

inline plVec3 plCamera::GetCenterDirForwards() const
{
  if (m_Mode == plCameraMode::Stereo)
    return (GetDirForwards(plCameraEye::Left) + GetDirForwards(plCameraEye::Right)).GetNormalized();
  else
    return GetDirForwards();
}

inline plVec3 plCamera::GetCenterDirUp() const
{
  if (m_Mode == plCameraMode::Stereo)
    return (GetDirUp(plCameraEye::Left) + GetDirUp(plCameraEye::Right)).GetNormalized();
  else
    return GetDirUp();
}

inline plVec3 plCamera::GetCenterDirRight() const
{
  if (m_Mode == plCameraMode::Stereo)
    return (GetDirRight(plCameraEye::Left) + GetDirRight(plCameraEye::Right)).GetNormalized();
  else
    return GetDirRight();
}

PL_ALWAYS_INLINE float plCamera::GetNearPlane() const
{
  return m_fNearPlane;
}

PL_ALWAYS_INLINE float plCamera::GetFarPlane() const
{
  return m_fFarPlane;
}

PL_ALWAYS_INLINE float plCamera::GetFovOrDim() const
{
  return m_fFovOrDim;
}

PL_ALWAYS_INLINE plCameraMode::Enum plCamera::GetCameraMode() const
{
  return m_Mode;
}

PL_ALWAYS_INLINE bool plCamera::IsPerspective() const
{
  return m_Mode == plCameraMode::PerspectiveFixedFovX || m_Mode == plCameraMode::PerspectiveFixedFovY ||
         m_Mode == plCameraMode::Stereo; // All HMD stereo cameras are perspective!
}

PL_ALWAYS_INLINE bool plCamera::IsOrthographic() const
{
  return m_Mode == plCameraMode::OrthoFixedWidth || m_Mode == plCameraMode::OrthoFixedHeight;
}

PL_ALWAYS_INLINE bool plCamera::IsStereoscopic() const
{
  return m_Mode == plCameraMode::Stereo;
}

PL_ALWAYS_INLINE float plCamera::GetExposure() const
{
  return m_fExposure;
}

PL_ALWAYS_INLINE void plCamera::SetExposure(float fExposure)
{
  m_fExposure = fExposure;
}

PL_ALWAYS_INLINE const plMat4& plCamera::GetViewMatrix(plCameraEye eye) const
{
  return m_mViewMatrix[static_cast<int>(eye)];
}

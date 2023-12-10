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

PLASMA_ALWAYS_INLINE float plCamera::GetNearPlane() const
{
  return m_fNearPlane;
}

PLASMA_ALWAYS_INLINE float plCamera::GetFarPlane() const
{
  return m_fFarPlane;
}

PLASMA_ALWAYS_INLINE float plCamera::GetFovOrDim() const
{
  return m_fFovOrDim;
}

PLASMA_ALWAYS_INLINE plCameraMode::Enum plCamera::GetCameraMode() const
{
  return m_Mode;
}

PLASMA_ALWAYS_INLINE bool plCamera::IsPerspective() const
{
  return m_Mode == plCameraMode::PerspectiveFixedFovX || m_Mode == plCameraMode::PerspectiveFixedFovY ||
         m_Mode == plCameraMode::Stereo; // All HMD stereo cameras are perspective!
}

PLASMA_ALWAYS_INLINE bool plCamera::IsOrthographic() const
{
  return m_Mode == plCameraMode::OrthoFixedWidth || m_Mode == plCameraMode::OrthoFixedHeight;
}

PLASMA_ALWAYS_INLINE bool plCamera::IsStereoscopic() const
{
  return m_Mode == plCameraMode::Stereo;
}

PLASMA_ALWAYS_INLINE float plCamera::GetShutterSpeed() const
{
  return m_fShutterSpeed;
}

PLASMA_ALWAYS_INLINE void plCamera::SetShutterSpeed(float fShutterSpeed)
{
  m_fShutterSpeed = fShutterSpeed;
}

PLASMA_ALWAYS_INLINE float plCamera::GetExposure() const
{
  return m_fExposure;
}

PLASMA_ALWAYS_INLINE void plCamera::SetExposure(float fExposure)
{
  m_fExposure = fExposure;
}

PLASMA_ALWAYS_INLINE float plCamera::GetAperture() const
{
    return m_fAperture;
}

PLASMA_ALWAYS_INLINE void plCamera::SetAperture(float fAperture)
{
    m_fAperture = fAperture;
}

PLASMA_ALWAYS_INLINE float plCamera::GetISO() const
{
    return m_fISO;
}

PLASMA_ALWAYS_INLINE void plCamera::SetISO(float fISO)
{
    m_fISO = fISO;
}


PLASMA_ALWAYS_INLINE float plCamera::GetFocusDistance() const
{
  return m_fFocusDistance;
}

PLASMA_ALWAYS_INLINE void plCamera::SetFocusDistance(float fFocusDistance)
{
  m_fFocusDistance = fFocusDistance;
}


PLASMA_ALWAYS_INLINE const plMat4& plCamera::GetViewMatrix(plCameraEye eye) const
{
  return m_mViewMatrix[static_cast<int>(eye)];
}

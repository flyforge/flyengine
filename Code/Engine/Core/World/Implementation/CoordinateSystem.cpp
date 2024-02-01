#include <Core/CorePCH.h>

#include <Core/World/CoordinateSystem.h>


plCoordinateSystemConversion::plCoordinateSystemConversion()
{
  m_mSourceToTarget.SetIdentity();
  m_mTargetToSource.SetIdentity();
}

void plCoordinateSystemConversion::SetConversion(const plCoordinateSystem& source, const plCoordinateSystem& target)
{
  float fSourceScale = source.m_vForwardDir.GetLengthSquared();
  PL_ASSERT_DEV(plMath::IsEqual(fSourceScale, source.m_vRightDir.GetLengthSquared(), plMath::DefaultEpsilon<float>()),
    "Only uniformly scaled coordinate systems are supported");
  PL_ASSERT_DEV(plMath::IsEqual(fSourceScale, source.m_vUpDir.GetLengthSquared(), plMath::DefaultEpsilon<float>()),
    "Only uniformly scaled coordinate systems are supported");
  plMat3 mSourceFromId;
  mSourceFromId.SetColumn(0, source.m_vRightDir);
  mSourceFromId.SetColumn(1, source.m_vUpDir);
  mSourceFromId.SetColumn(2, source.m_vForwardDir);

  float fTargetScale = target.m_vForwardDir.GetLengthSquared();
  PL_ASSERT_DEV(plMath::IsEqual(fTargetScale, target.m_vRightDir.GetLengthSquared(), plMath::DefaultEpsilon<float>()),
    "Only uniformly scaled coordinate systems are supported");
  PL_ASSERT_DEV(plMath::IsEqual(fTargetScale, target.m_vUpDir.GetLengthSquared(), plMath::DefaultEpsilon<float>()),
    "Only uniformly scaled coordinate systems are supported");
  plMat3 mTargetFromId;
  mTargetFromId.SetColumn(0, target.m_vRightDir);
  mTargetFromId.SetColumn(1, target.m_vUpDir);
  mTargetFromId.SetColumn(2, target.m_vForwardDir);

  m_mSourceToTarget = mTargetFromId * mSourceFromId.GetInverse();
  m_mSourceToTarget.SetColumn(0, m_mSourceToTarget.GetColumn(0).GetNormalized());
  m_mSourceToTarget.SetColumn(1, m_mSourceToTarget.GetColumn(1).GetNormalized());
  m_mSourceToTarget.SetColumn(2, m_mSourceToTarget.GetColumn(2).GetNormalized());

  m_fWindingSwap = m_mSourceToTarget.GetDeterminant() < 0 ? -1.0f : 1.0f;
  m_fSourceToTargetScale = 1.0f / plMath::Sqrt(fSourceScale) * plMath::Sqrt(fTargetScale);
  m_mTargetToSource = m_mSourceToTarget.GetInverse();
  m_fTargetToSourceScale = 1.0f / m_fSourceToTargetScale;
}

plVec3 plCoordinateSystemConversion::ConvertSourcePosition(const plVec3& vPos) const
{
  return m_mSourceToTarget * vPos * m_fSourceToTargetScale;
}

plQuat plCoordinateSystemConversion::ConvertSourceRotation(const plQuat& qOrientation) const
{
  plVec3 axis = m_mSourceToTarget * qOrientation.GetVectorPart();
  plQuat rr(axis.x, axis.y, axis.z, qOrientation.w * m_fWindingSwap);
  return rr;
}

float plCoordinateSystemConversion::ConvertSourceLength(float fLength) const
{
  return fLength * m_fSourceToTargetScale;
}

plVec3 plCoordinateSystemConversion::ConvertTargetPosition(const plVec3& vPos) const
{
  return m_mTargetToSource * vPos * m_fTargetToSourceScale;
}

plQuat plCoordinateSystemConversion::ConvertTargetRotation(const plQuat& qOrientation) const
{
  plVec3 axis = m_mTargetToSource * qOrientation.GetVectorPart();
  plQuat rr(axis.x, axis.y, axis.z, qOrientation.w * m_fWindingSwap);
  return rr;
}

float plCoordinateSystemConversion::ConvertTargetLength(float fLength) const
{
  return fLength * m_fTargetToSourceScale;
}



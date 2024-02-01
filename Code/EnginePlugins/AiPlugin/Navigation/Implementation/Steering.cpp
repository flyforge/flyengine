#include <AiPlugin/Navigation/Steering.h>

void plAiSteering::Calculate(float fTimeDiff, plDebugRendererContext ctxt)
{
  const float fRunSpeed = m_fMaxSpeed;
  const float fJogSpeed = m_fMaxSpeed * 0.75f;
  const float fWalkSpeed = m_fMaxSpeed * 0.5f;

  const float fBrakingDistanceRun = 1.2f * (plMath::Square(fRunSpeed) / (2.0f * m_fDecceleration));
  const float fBrakingDistanceJog = 1.2f * (plMath::Square(fJogSpeed) / (2.0f * m_fDecceleration));
  const float fBrakingDistanceWalk = 1.2f * (plMath::Square(fWalkSpeed) / (2.0f * m_fDecceleration));

  if (m_Info.m_fArrivalDistance <= fBrakingDistanceWalk)
    m_fMaxSpeed = 0.0f;
  else if (m_Info.m_fArrivalDistance < fBrakingDistanceJog)
    m_fMaxSpeed = plMath::Min(m_fMaxSpeed, fWalkSpeed);
  else if (m_Info.m_fArrivalDistance < fBrakingDistanceRun)
    m_fMaxSpeed = plMath::Min(m_fMaxSpeed, fJogSpeed);

  if (m_Info.m_AbsRotationTowardsWaypoint > plAngle::MakeFromDegree(80))
    m_fMaxSpeed = 0.0f;
  else if (m_Info.m_AbsRotationTowardsWaypoint > plAngle::MakeFromDegree(55))
    m_fMaxSpeed = plMath::Min(m_fMaxSpeed, fWalkSpeed);
  else if (m_Info.m_AbsRotationTowardsWaypoint > plAngle::MakeFromDegree(30))
    m_fMaxSpeed = plMath::Min(m_fMaxSpeed, fJogSpeed);

  plAngle maxRotation = m_Info.m_AbsRotationTowardsWaypoint;

  if (m_Info.m_fDistanceToWaypoint <= fBrakingDistanceRun)
  {
    if (m_Info.m_MaxAbsRotationAfterWaypoint > plAngle::MakeFromDegree(40))
      m_fMaxSpeed = plMath::Min(m_fMaxSpeed, fWalkSpeed);
    else if (m_Info.m_MaxAbsRotationAfterWaypoint > plAngle::MakeFromDegree(20))
      m_fMaxSpeed = plMath::Min(m_fMaxSpeed, fJogSpeed);

    maxRotation = plMath::Max(maxRotation, m_Info.m_MaxAbsRotationAfterWaypoint);
  }

  plVec3 vLookDir = m_qRotation * plVec3::MakeAxisX();
  vLookDir.z = 0;

  float fCurSpeed = m_vVelocity.GetAsVec2().GetLength();

  plAngle turnSpeed = m_MinTurnSpeed;

  {
    const float fTurnRadius = 1.0f; // plMath::Clamp(m_Info.m_fWaypointCorridorWidth, 0.5f, 5.0f);
    const float fCircumference = 2.0f * plMath::Pi<float>() * fTurnRadius;
    const float fCircleFraction = maxRotation / plAngle::MakeFromDegree(360);
    const float fTurnDistance = fCircumference * fCircleFraction;
    const float fTurnDuration = fTurnDistance / fCurSpeed;

    turnSpeed = plMath::Max(m_MinTurnSpeed, maxRotation / fTurnDuration);
  }

  // plDebugRenderer::DrawInfoText(ctxt, plDebugRenderer::ScreenPlacement::BottomLeft, "Steering", plFmt("Turn Speed: {}", turnSpeed));
  // plDebugRenderer::DrawInfoText(ctxt, plDebugRenderer::ScreenPlacement::BottomLeft, "Steering", plFmt("Corridor Width: {}", m_Info.m_fWaypointCorridorWidth));

  if (!m_Info.m_vDirectionTowardsWaypoint.IsZero())
  {
    const plVec3 vTargetDir = m_Info.m_vDirectionTowardsWaypoint.GetAsVec3(0);
    plVec3 vRotAxis = vLookDir.CrossRH(vTargetDir);
    vRotAxis.NormalizeIfNotZero(plVec3::MakeAxisZ()).IgnoreResult();
    const plAngle toRotate = plMath::Min(vLookDir.GetAngleBetween(vTargetDir), fTimeDiff * turnSpeed);
    const plQuat qRot = plQuat::MakeFromAxisAndAngle(vRotAxis, toRotate);

    vLookDir = qRot * vLookDir;

    m_qRotation = plQuat::MakeShortestRotation(plVec3::MakeAxisX(), vLookDir);
  }


  if (fCurSpeed < m_fMaxSpeed)
  {
    fCurSpeed += fTimeDiff * m_fAcceleration;
    fCurSpeed = plMath::Min(fCurSpeed, m_fMaxSpeed);
  }
  else if (fCurSpeed > m_fMaxSpeed)
  {
    fCurSpeed -= fTimeDiff * m_fDecceleration;
    fCurSpeed = plMath::Max(fCurSpeed, m_fMaxSpeed);
  }

  plVec3 vDir = vLookDir;
  vDir *= fCurSpeed;
  vDir *= fTimeDiff;
  m_vPosition += vDir;
}

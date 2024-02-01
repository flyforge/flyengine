#pragma once

#include <AiPlugin/Navigation/Navigation.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>

/// Work in progress, do not use.
///
/// Attempt to implement a steering behavior.
struct PL_AIPLUGIN_DLL plAiSteering
{
  plVec3 m_vPosition = plVec3::MakeZero();
  plQuat m_qRotation = plQuat::MakeIdentity();
  plVec3 m_vVelocity = plVec3::MakeZero();
  float m_fMaxSpeed = 6.0f;
  float m_fAcceleration = 5.0f;
  float m_fDecceleration = 10.0f;
  plAngle m_MinTurnSpeed = plAngle::MakeFromDegree(180);

  plAiSteeringInfo m_Info;

  void Calculate(float fTimeDiff, plDebugRendererContext ctxt);
};

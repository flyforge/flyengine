#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <Foundation/Math/Vec3.h>

/// \brief Stores the transformation data for an audio entity.
struct PL_AUDIOSYSTEMPLUGIN_DLL plAudioEntityTransform
{
  /// \brief The position of the entity in world space.
  plVec3 m_vPosition;

  /// \brief The velocity of the entity.
  plVec3 m_vVelocity;

  /// \brief The forward direction of the entity in world space.
  plVec3 m_vForward;

  /// \brief The up direction of the entity in world space.
  plVec3 m_vUp;
};

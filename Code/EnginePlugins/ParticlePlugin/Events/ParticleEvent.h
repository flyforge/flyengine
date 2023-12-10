#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/ArrayPtr.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

struct PLASMA_PARTICLEPLUGIN_DLL plParticleEvent
{
  PLASMA_DECLARE_POD_TYPE();

  plTempHashedString m_EventType;
  plVec3 m_vPosition;
  plVec3 m_vDirection;
  plVec3 m_vNormal;
};

typedef plArrayPtr<plParticleEvent> plParticleEventQueue;

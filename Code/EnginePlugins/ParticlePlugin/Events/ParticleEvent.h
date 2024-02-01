#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/ArrayPtr.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

struct PL_PARTICLEPLUGIN_DLL plParticleEvent
{
  PL_DECLARE_POD_TYPE();

  plTempHashedString m_EventType;
  plVec3 m_vPosition;
  plVec3 m_vDirection;
  plVec3 m_vNormal;
};

using plParticleEventQueue = plArrayPtr<plParticleEvent>;

#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Math/Vec3.h>

/// \brief Helper class to convert between Recast's convention (float[3] and Y is up) and plVec3 (Z up)
///
/// Will automatically swap Y and Z when assigning between the different types.
struct PLASMA_AIPLUGIN_DLL plRcPos
{
  float m_Pos[3];

  plRcPos();
  plRcPos(const float* pPos);
  plRcPos(const plVec3& v);

  void operator=(const plVec3& v);
  void operator=(const float* pPos);

  operator const float*() const;
  operator float*();
  operator plVec3() const;
};

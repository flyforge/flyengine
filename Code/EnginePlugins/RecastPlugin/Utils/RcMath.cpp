#include <RecastPlugin/RecastPluginPCH.h>

#include <Foundation/Math/Vec3.h>
#include <RecastPlugin/Utils/RcMath.h>

plRcPos::plRcPos()
{
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  m_Pos[0] = plMath::NaN<float>();
  m_Pos[1] = plMath::NaN<float>();
  m_Pos[2] = plMath::NaN<float>();
#endif
}

plRcPos::plRcPos(const plVec3& v)
{
  *this = v;
}

plRcPos::plRcPos(const float* pPos)
{
  *this = pPos;
}

plRcPos::operator const float *() const
{
  return &m_Pos[0];
}

plRcPos::operator float*()
{
  return &m_Pos[0];
}

plRcPos::operator plVec3() const
{
  return plVec3(m_Pos[0], m_Pos[2], m_Pos[1]);
}

void plRcPos::operator=(const float* pPos)
{
  m_Pos[0] = pPos[0];
  m_Pos[1] = pPos[1];
  m_Pos[2] = pPos[2];
}

void plRcPos::operator=(const plVec3& v)
{
  m_Pos[0] = v.x;
  m_Pos[1] = v.z;
  m_Pos[2] = v.y;
}

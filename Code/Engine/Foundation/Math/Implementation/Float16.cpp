#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

plFloat16::plFloat16(float f)
{
  operator=(f);
}

void plFloat16::operator=(float f)
{
  // source: http://www.ogre3d.org/docs/api/html/OgreBitwise_8h_source.html

  const plUInt32 i = *reinterpret_cast<plUInt32*>(&f);

  const plUInt32 s = (i >> 16) & 0x00008000;
  const plInt32 e = ((i >> 23) & 0x000000ff) - (127 - 15);
  plUInt32 m = i & 0x007fffff;

  if (e <= 0)
  {
    if (e < -10)
    {
      m_uiData = 0;
      return;
    }
    m = (m | 0x00800000) >> (1 - e);

    m_uiData = static_cast<plUInt16>(s | (m >> 13));
  }
  else if (e == 0xff - (127 - 15))
  {
    if (m == 0) // Inf
    {
      m_uiData = static_cast<plUInt16>(s | 0x7c00);
    }
    else // NAN
    {
      m >>= 13;
      m_uiData = static_cast<plUInt16>(s | 0x7c00 | m | (m == 0));
    }
  }
  else
  {
    if (e > 30) // Overflow
    {
      m_uiData = static_cast<plUInt16>(s | 0x7c00);
      return;
    }

    m_uiData = static_cast<plUInt16>(s | (e << 10) | (m >> 13));
  }
}

plFloat16::operator float() const
{
  const plUInt32 s = (m_uiData >> 15) & 0x00000001;
  plUInt32 e = (m_uiData >> 10) & 0x0000001f;
  plUInt32 m = m_uiData & 0x000003ff;

  plUInt32 uiResult;

  if (e == 0)
  {
    if (m == 0) // Plus or minus zero
    {
      uiResult = s << 31;
      return *reinterpret_cast<float*>(&uiResult);
    }
    else // Denormalized number -- renormalize it
    {
      while (!(m & 0x00000400))
      {
        m <<= 1;
        e -= 1;
      }

      e += 1;
      m &= ~0x00000400;
    }
  }
  else if (e == 31)
  {
    if (m == 0) // Inf
    {
      uiResult = (s << 31) | 0x7f800000;
      return *reinterpret_cast<float*>(&uiResult);
    }
    else // NaN
    {
      uiResult = (s << 31) | 0x7f800000 | (m << 13);
      return *reinterpret_cast<float*>(&uiResult);
    }
  }

  e = e + (127 - 15);
  m = m << 13;

  uiResult = (s << 31) | (e << 23) | m;

  return *reinterpret_cast<float*>(&uiResult);
}

//////////////////////////////////////////////////////////////////////////

plFloat16Vec2::plFloat16Vec2(const plVec2& vVec)
{
  operator=(vVec);
}

void plFloat16Vec2::operator=(const plVec2& vVec)
{
  x = vVec.x;
  y = vVec.y;
}

plFloat16Vec2::operator plVec2() const
{
  return plVec2(x, y);
}

//////////////////////////////////////////////////////////////////////////

plFloat16Vec3::plFloat16Vec3(const plVec3& vVec)
{
  operator=(vVec);
}

void plFloat16Vec3::operator=(const plVec3& vVec)
{
  x = vVec.x;
  y = vVec.y;
  z = vVec.z;
}

plFloat16Vec3::operator plVec3() const
{
  return plVec3(x, y, z);
}

//////////////////////////////////////////////////////////////////////////

plFloat16Vec4::plFloat16Vec4(const plVec4& vVec)
{
  operator=(vVec);
}

void plFloat16Vec4::operator=(const plVec4& vVec)
{
  x = vVec.x;
  y = vVec.y;
  z = vVec.z;
  w = vVec.w;
}

plFloat16Vec4::operator plVec4() const
{
  return plVec4(x, y, z, w);
}



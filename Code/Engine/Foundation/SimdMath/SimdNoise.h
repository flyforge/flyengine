#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

class PL_FOUNDATION_DLL plSimdPerlinNoise
{
public:
  plSimdPerlinNoise(plUInt32 uiSeed);

  plSimdVec4f NoiseZeroToOne(const plSimdVec4f& x, const plSimdVec4f& y, const plSimdVec4f& z, plUInt32 uiNumOctaves = 1);

private:
  plSimdVec4f Noise(const plSimdVec4f& x, const plSimdVec4f& y, const plSimdVec4f& z);

  PL_FORCE_INLINE plSimdVec4i Permute(const plSimdVec4i& v)
  {
#if 0
    plArrayPtr<plUInt8> p = plMakeArrayPtr(m_Permutations);
#else
    plUInt8* p = m_Permutations;
#endif

    plSimdVec4i i = v & plSimdVec4i(PL_ARRAY_SIZE(m_Permutations) - 1);
    return plSimdVec4i(p[i.x()], p[i.y()], p[i.z()], p[i.w()]);
  }

  plUInt8 m_Permutations[256];
};

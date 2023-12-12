#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/SimdMath/SimdNoise.h>

plSimdPerlinNoise::plSimdPerlinNoise(plUInt32 uiSeed)
{
  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(m_Permutations); ++i)
  {
    m_Permutations[i] = static_cast<plUInt8>(i);
  }

  plRandom rnd;
  rnd.Initialize(uiSeed);

  for (plUInt32 i = PLASMA_ARRAY_SIZE(m_Permutations) - 1; i > 0; --i)
  {
    plUInt32 uiRandomIndex = rnd.UIntInRange(PLASMA_ARRAY_SIZE(m_Permutations));
    plMath::Swap(m_Permutations[i], m_Permutations[uiRandomIndex]);
  }
}

plSimdVec4f plSimdPerlinNoise::NoiseZeroToOne(const plSimdVec4f& vX, const plSimdVec4f& vY, const plSimdVec4f& vZ, plUInt32 uiNumOctaves /*= 1*/)
{
  plSimdVec4f result = plSimdVec4f::ZeroVector();
  plSimdFloat amplitude = 1.0f;
  plUInt32 uiOffset = 0;

  uiNumOctaves = plMath::Max(uiNumOctaves, 1u);
  for (plUInt32 i = 0; i < uiNumOctaves; ++i)
  {
    plSimdFloat scale = static_cast<float>(PLASMA_BIT(i));
    plSimdVec4f offset = Permute(plSimdVec4i(uiOffset) + plSimdVec4i(0, 1, 2, 3)).ToFloat();
    plSimdVec4f x = vX * scale + offset.Get<plSwizzle::XXXX>();
    plSimdVec4f y = vY * scale + offset.Get<plSwizzle::YYYY>();
    plSimdVec4f z = vZ * scale + offset.Get<plSwizzle::ZZZZ>();

    result += Noise(x, y, z) * amplitude;

    amplitude *= 0.5f;
    uiOffset += 23;
  }

  return result * 0.5f + plSimdVec4f(0.5f);
}

namespace
{
  PLASMA_FORCE_INLINE plSimdVec4f Fade(const plSimdVec4f& t)
  {
    return t.CompMul(t).CompMul(t).CompMul(t.CompMul(t * 6.0f - plSimdVec4f(15.0f)) + plSimdVec4f(10.0f));
  }

  PLASMA_FORCE_INLINE plSimdVec4f Grad(plSimdVec4i vHash, const plSimdVec4f& x, const plSimdVec4f& y, const plSimdVec4f& z)
  {
    // convert low 4 bits of hash code into 12 gradient directions.
    const plSimdVec4i h = vHash & plSimdVec4i(15);
    const plSimdVec4f u = plSimdVec4f::Select(h < plSimdVec4i(8), x, y);
    const plSimdVec4f v = plSimdVec4f::Select(h < plSimdVec4i(4), y, plSimdVec4f::Select(h == plSimdVec4i(12) || h == plSimdVec4i(14), x, z));
    return plSimdVec4f::Select((h & plSimdVec4i(1)) == plSimdVec4i::ZeroVector(), u, -u) +
           plSimdVec4f::Select((h & plSimdVec4i(2)) == plSimdVec4i::ZeroVector(), v, -v);
  }

  PLASMA_ALWAYS_INLINE plSimdVec4f Lerp(const plSimdVec4f& t, const plSimdVec4f& a, const plSimdVec4f& b) { return plSimdVec4f::Lerp(a, b, t); }

} // namespace

// reference: https://mrl.nyu.edu/~perlin/noise/
plSimdVec4f plSimdPerlinNoise::Noise(const plSimdVec4f& inX, const plSimdVec4f& inY, const plSimdVec4f& inZ)
{
  plSimdVec4f x = inX;
  plSimdVec4f y = inY;
  plSimdVec4f z = inZ;

  // find unit cube that contains point.
  const plSimdVec4f xFloored = x.Floor();
  const plSimdVec4f yFloored = y.Floor();
  const plSimdVec4f zFloored = z.Floor();

  const plSimdVec4i maxIndex = plSimdVec4i(255);
  const plSimdVec4i X = plSimdVec4i::Truncate(xFloored) & maxIndex;
  const plSimdVec4i Y = plSimdVec4i::Truncate(yFloored) & maxIndex;
  const plSimdVec4i Z = plSimdVec4i::Truncate(zFloored) & maxIndex;

  // find relative x,y,z of point in cube.
  x -= xFloored;
  y -= yFloored;
  z -= zFloored;

  // compute fade curves for each of x,y,z.
  const plSimdVec4f u = Fade(x);
  const plSimdVec4f v = Fade(y);
  const plSimdVec4f w = Fade(z);

  // hash coordinates of the 8 cube corners
  const plSimdVec4i i1 = plSimdVec4i(1);
  const plSimdVec4i A = Permute(X) + Y;
  const plSimdVec4i AA = Permute(A) + Z;
  const plSimdVec4i AB = Permute(A + i1) + Z;
  const plSimdVec4i B = Permute(X + i1) + Y;
  const plSimdVec4i BA = Permute(B) + Z;
  const plSimdVec4i BB = Permute(B + i1) + Z;

  const plSimdVec4f f1 = plSimdVec4f(1.0f);

  // and add blended results from 8 corners of cube.
  const plSimdVec4f c000 = Grad(Permute(AA), x, y, z);
  const plSimdVec4f c100 = Grad(Permute(BA), x - f1, y, z);
  const plSimdVec4f c010 = Grad(Permute(AB), x, y - f1, z);
  const plSimdVec4f c110 = Grad(Permute(BB), x - f1, y - f1, z);
  const plSimdVec4f c001 = Grad(Permute(AA + i1), x, y, z - f1);
  const plSimdVec4f c101 = Grad(Permute(BA + i1), x - f1, y, z - f1);
  const plSimdVec4f c011 = Grad(Permute(AB + i1), x, y - f1, z - f1);
  const plSimdVec4f c111 = Grad(Permute(BB + i1), x - f1, y - f1, z - f1);

  const plSimdVec4f c000_c100 = Lerp(u, c000, c100);
  const plSimdVec4f c010_c110 = Lerp(u, c010, c110);
  const plSimdVec4f c001_c101 = Lerp(u, c001, c101);
  const plSimdVec4f c011_c111 = Lerp(u, c011, c111);

  return Lerp(w, Lerp(v, c000_c100, c010_c110), Lerp(v, c001_c101, c011_c111));
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_SimdMath_Implementation_SimdNoise);

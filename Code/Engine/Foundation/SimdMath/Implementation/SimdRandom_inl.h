#pragma once

// static
PLASMA_FORCE_INLINE plSimdVec4u plSimdRandom::UInt(const plSimdVec4i& vPosition, const plSimdVec4u& vSeed /*= plSimdVec4u::ZeroVector()*/)
{
  // Based on Squirrel3 which was introduced by Squirrel Eiserloh at 'Math for Game Programmers: Noise-Based RNG', GDC17.
  const plSimdVec4u BIT_NOISE1 = plSimdVec4u(0xb5297a4d);
  const plSimdVec4u BIT_NOISE2 = plSimdVec4u(0x68e31da4);
  const plSimdVec4u BIT_NOISE3 = plSimdVec4u(0x1b56c4e9);

  plSimdVec4u mangled = plSimdVec4u(vPosition);
  mangled = mangled.CompMul(BIT_NOISE1);
  mangled += vSeed;
  mangled ^= (mangled >> 8);
  mangled += BIT_NOISE2;
  mangled ^= (mangled << 8);
  mangled = mangled.CompMul(BIT_NOISE3);
  mangled ^= (mangled >> 8);

  return mangled;
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdRandom::FloatZeroToOne(const plSimdVec4i& vPosition, const plSimdVec4u& vSeed /*= plSimdVec4u::ZeroVector()*/)
{
  return UInt(vPosition, vSeed).ToFloat() * (1.0f / 4294967296.0f);
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdRandom::FloatMinMax(const plSimdVec4i& vPosition, const plSimdVec4f& vMinValue, const plSimdVec4f& vMaxValue, const plSimdVec4u& vSeed /*= plSimdVec4u::ZeroVector()*/)
{
  return plSimdVec4f::Lerp(vMinValue, vMaxValue, FloatZeroToOne(vPosition, vSeed));
}

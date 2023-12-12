#pragma once

#include <Foundation/SimdMath/SimdVec4u.h>

/// \brief Noise based random number generator that generates 4 pseudo random values at once.
///
/// Does not keep any internal state but relies on the user to provide different positions for each call.
/// The seed parameter can be used to further alter the noise function.
struct plSimdRandom
{
  /// \brief Returns 4 random uint32 values at position, ie. ranging from 0 to (2 ^ 32) - 1
  static plSimdVec4u UInt(const plSimdVec4i& vPosition, const plSimdVec4u& vSeed = plSimdVec4u::ZeroVector());

  /// \brief Returns 4 random float values in range [0.0 ; 1.0], ie. including zero and one
  static plSimdVec4f FloatZeroToOne(const plSimdVec4i& vPosition, const plSimdVec4u& vSeed = plSimdVec4u::ZeroVector());

  /// \brief Returns 4 random float values in range [fMinValue ; fMaxValue]
  static plSimdVec4f FloatMinMax(const plSimdVec4i& vPosition, const plSimdVec4f& vMinValue, const plSimdVec4f& vMaxValue, const plSimdVec4u& vSeed = plSimdVec4u::ZeroVector());
};

#include <Foundation/SimdMath/Implementation/SimdRandom_inl.h>

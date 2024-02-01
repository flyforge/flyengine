#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Math/Vec3.h>

template <typename Type>
PL_IMPLEMENT_IF_FLOAT_TYPE plVec3Template<Type> plVec3Template<Type>::MakeRandomPointInSphere(plRandom& inout_rng)
{
  double px, py, pz;
  double len = 0.0;

  do
  {
    px = inout_rng.DoubleMinMax(-1, 1);
    py = inout_rng.DoubleMinMax(-1, 1);
    pz = inout_rng.DoubleMinMax(-1, 1);

    len = (px * px) + (py * py) + (pz * pz);
  } while (len > 1.0 || len <= 0.000001); // prevent the exact center

  return plVec3Template<Type>((Type)px, (Type)py, (Type)pz);
}

template <typename Type>
PL_IMPLEMENT_IF_FLOAT_TYPE plVec3Template<Type> plVec3Template<Type>::MakeRandomDirection(plRandom& inout_rng)
{
  plVec3Template<Type> vec = MakeRandomPointInSphere(inout_rng);
  vec.Normalize();
  return vec;
}

template <typename Type>
PL_IMPLEMENT_IF_FLOAT_TYPE plVec3Template<Type> plVec3Template<Type>::MakeRandomDeviationX(plRandom& inout_rng, const plAngle& maxDeviation)
{
  const double twoPi = 2.0 * plMath::Pi<double>();

  const double cosAngle = plMath::Cos(maxDeviation);

  const double x = inout_rng.DoubleZeroToOneInclusive() * (1 - cosAngle) + cosAngle;
  const plAngle phi = plAngle::MakeFromRadian((float)(inout_rng.DoubleZeroToOneInclusive() * twoPi));
  const double invSqrt = plMath::Sqrt(1 - (x * x));
  const double y = invSqrt * plMath::Cos(phi);
  const double z = invSqrt * plMath::Sin(phi);

  return plVec3Template<Type>((Type)x, (Type)y, (Type)z);
}

template <typename Type>
PL_IMPLEMENT_IF_FLOAT_TYPE plVec3Template<Type> plVec3Template<Type>::MakeRandomDeviationY(plRandom& inout_rng, const plAngle& maxDeviation)
{
  plVec3Template<Type> vec = MakeRandomDeviationX(inout_rng, maxDeviation);
  plMath::Swap(vec.x, vec.y);
  return vec;
}

template <typename Type>
PL_IMPLEMENT_IF_FLOAT_TYPE plVec3Template<Type> plVec3Template<Type>::MakeRandomDeviationZ(plRandom& inout_rng, const plAngle& maxDeviation)
{
  plVec3Template<Type> vec = MakeRandomDeviationX(inout_rng, maxDeviation);
  plMath::Swap(vec.x, vec.z);
  return vec;
}

template <typename Type>
PL_IMPLEMENT_IF_FLOAT_TYPE plVec3Template<Type> plVec3Template<Type>::MakeRandomDeviation(plRandom& inout_rng, const plAngle& maxDeviation, const plVec3Template<Type>& vNormal)
{
  // If you need to do this very often:
  // *** Pre-compute this once: ***

  // how to get from the X axis to our desired basis
  plQuatTemplate<Type> qRotXtoDir = plQuat::MakeShortestRotation(plVec3Template<Type>(1, 0, 0), vNormal);

  // *** Then call this with the precomputed value as often as needed: ***

  // create a random vector along X
  plVec3Template<Type> vec = MakeRandomDeviationX(inout_rng, maxDeviation);
  // rotate from X to our basis
  return qRotXtoDir * vec;
}

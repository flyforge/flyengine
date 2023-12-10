#include <Foundation/FoundationPCH.h>

#include <Foundation/SimdMath/SimdQuat.h>

plSimdQuat plSimdQuat::MakeShortestRotation(const plSimdVec4f& vDirFrom, const plSimdVec4f& vDirTo)
{
  const plSimdVec4f v0 = vDirFrom.GetNormalized<3>();
  const plSimdVec4f v1 = vDirTo.GetNormalized<3>();

  const plSimdFloat fDot = v0.Dot<3>(v1);

  // if both vectors are identical -> no rotation needed
  if (fDot.IsEqual(1.0f, 0.0001f))
  {
    return plSimdQuat::MakeIdentity();
  }
  else if (fDot.IsEqual(-1.0f, 0.0001f)) // if both vectors are opposing
  {
    return plSimdQuat::MakeFromAxisAndAngle(v0.GetOrthogonalVector().GetNormalized<3>(), plAngle::MakeFromRadian(plMath::Pi<float>()));
  }

  const plSimdVec4f c = v0.CrossRH(v1);
  const plSimdFloat s = ((fDot + plSimdFloat(1.0f)) * plSimdFloat(2.0f)).GetSqrt();

  plSimdQuat res;
  res.m_v = c / s;
  res.m_v.SetW(s * plSimdFloat(0.5f));
  res.Normalize();
  return res;
}

plSimdQuat plSimdQuat::MakeSlerp(const plSimdQuat& qFrom, const plSimdQuat& qTo, const plSimdFloat& t)
{
  PLASMA_ASSERT_DEBUG((t >= 0.0f) && (t <= 1.0f), "Invalid lerp factor.");

  const plSimdFloat one = 1.0f;
  const plSimdFloat qdelta = 1.0f - 0.001f;

  const plSimdFloat fDot = qFrom.m_v.Dot<4>(qTo.m_v);

  plSimdFloat cosTheta = fDot;

  bool bFlipSign = false;
  if (cosTheta < 0.0f)
  {
    bFlipSign = true;
    cosTheta = -cosTheta;
  }

  plSimdFloat t0, t1;

  if (cosTheta < qdelta)
  {
    plAngle theta = plMath::ACos(cosTheta);

    // use sqrtInv(1+c^2) instead of 1.0/sin(theta)
    const plSimdFloat iSinTheta = (one - (cosTheta * cosTheta)).GetInvSqrt();
    const plAngle tTheta = (float)t * theta;

    plSimdFloat s0 = plMath::Sin(theta - tTheta);
    plSimdFloat s1 = plMath::Sin(tTheta);

    t0 = s0 * iSinTheta;
    t1 = s1 * iSinTheta;
  }
  else
  {
    // If q0 is nearly the same as q1 we just linearly interpolate
    t0 = one - t;
    t1 = t;
  }

  if (bFlipSign)
    t1 = -t1;

  plSimdQuat res;
  res.m_v = qFrom.m_v * t0 + qTo.m_v * t1;
  res.Normalize();
  return res;
}

bool plSimdQuat::IsEqualRotation(const plSimdQuat& qOther, const plSimdFloat& fEpsilon) const
{
  plSimdVec4f vA1, vA2;
  plSimdFloat fA1, fA2;

  if (GetRotationAxisAndAngle(vA1, fA1) == PLASMA_FAILURE)
    return false;
  if (qOther.GetRotationAxisAndAngle(vA2, fA2) == PLASMA_FAILURE)
    return false;

  plAngle A1 = plAngle::MakeFromRadian(fA1);
  plAngle A2 = plAngle::MakeFromRadian(fA2);

  if ((A1.IsEqualSimple(A2, plAngle::MakeFromDegree(fEpsilon))) && (vA1.IsEqual(vA2, fEpsilon).AllSet<3>()))
    return true;

  if ((A1.IsEqualSimple(-A2, plAngle::MakeFromDegree(fEpsilon))) && (vA1.IsEqual(-vA2, fEpsilon).AllSet<3>()))
    return true;

  return false;
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_SimdMath_Implementation_SimdQuat);

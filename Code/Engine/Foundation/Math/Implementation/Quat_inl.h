#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec3.h>

template <typename Type>
PLASMA_ALWAYS_INLINE plQuatTemplate<Type>::plQuatTemplate()
{
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = plMath::NaN<Type>();
  x = TypeNaN;
  y = TypeNaN;
  z = TypeNaN;
  w = TypeNaN;
#endif
}

template <typename Type>
PLASMA_ALWAYS_INLINE plQuatTemplate<Type>::plQuatTemplate(Type inX, Type inY, Type inZ, Type inW)
  : x(inX)
  , y(inY)
  , z(inZ)
  , w(inW)
{
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plQuatTemplate<Type> plQuatTemplate<Type>::MakeIdentity()
{
  return plQuatTemplate(0, 0, 0, 1);
}

template <typename Type>
PLASMA_ALWAYS_INLINE plQuatTemplate<Type> plQuatTemplate<Type>::MakeFromElements(Type inX, Type inY, Type inZ, Type inW)
{
  return plQuatTemplate<Type>(inX, inY, inZ, inW);
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plQuatTemplate<Type>::SetIdentity()
{
  x = (Type)0;
  y = (Type)0;
  z = (Type)0;
  w = (Type)1;
}

template <typename Type>
plQuatTemplate<Type> plQuatTemplate<Type>::MakeFromAxisAndAngle(const plVec3Template<Type>& vRotationAxis, plAngle angle)
{
  const plAngle halfAngle = angle * 0.5f;

  plVec3 v = static_cast<Type>(plMath::Sin(halfAngle)) * vRotationAxis;
  float w = plMath::Cos(halfAngle);

  return plQuatTemplate<Type>(v.x, v.y, v.z, w);
}

template <typename Type>
void plQuatTemplate<Type>::Normalize()
{
  PLASMA_NAN_ASSERT(this);

  Type n = x * x + y * y + z * z + w * w;

  n = plMath::Invert(plMath::Sqrt(n));

  x *= n;
  y *= n;
  z *= n;
  w *= n;
}

template <typename Type>
void plQuatTemplate<Type>::GetRotationAxisAndAngle(plVec3Template<Type>& out_vAxis, plAngle& out_angle, Type fEpsilon) const
{
  PLASMA_NAN_ASSERT(this);

  out_angle = 2 * plMath::ACos(static_cast<float>(w));

  const float s = plMath::Sqrt(1 - w * w);

  if (s < fEpsilon)
  {
    out_vAxis.Set(1, 0, 0);
  }
  else
  {
    const float ds = 1.0f / s;
    out_vAxis.x = x * ds;
    out_vAxis.y = y * ds;
    out_vAxis.z = z * ds;
  }
}

template <typename Type>
PLASMA_FORCE_INLINE void plQuatTemplate<Type>::Invert()
{
  PLASMA_NAN_ASSERT(this);

  *this = GetInverse();
}

template <typename Type>
PLASMA_FORCE_INLINE const plQuatTemplate<Type> plQuatTemplate<Type>::GetInverse() const
{
  PLASMA_NAN_ASSERT(this);

  return (plQuatTemplate(-x, -y, -z, w));
}

template <typename Type>
PLASMA_FORCE_INLINE const plQuatTemplate<Type> plQuatTemplate<Type>::GetNegated() const
{
  PLASMA_NAN_ASSERT(this);

  return (plQuatTemplate(-x, -y, -z, -w));
}

template <typename Type>
PLASMA_FORCE_INLINE Type plQuatTemplate<Type>::Dot(const plQuatTemplate& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return GetVectorPart().Dot(rhs.GetVectorPart()) + w * rhs.w;
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plVec3Template<Type> operator*(const plQuatTemplate<Type>& q, const plVec3Template<Type>& v)
{
  plVec3Template<Type> t = q.GetVectorPart().CrossRH(v) * (Type)2;
  return v + q.w * t + q.GetVectorPart().CrossRH(t);
}

template <typename Type>
PLASMA_ALWAYS_INLINE const plQuatTemplate<Type> operator*(const plQuatTemplate<Type>& q1, const plQuatTemplate<Type>& q2)
{
  plQuatTemplate<Type> q;

  q.w = q1.w * q2.w - (q1.x * q2.x + q1.y * q2.y + q1.z * q2.z);

  const plVec3 v1 = q1.GetVectorPart();
  const plVec3 v2 = q2.GetVectorPart();

  const plVec3 vr = q1.w * v2 + q2.w * v1 + v1.CrossRH(v2);
  q.x = vr.x;
  q.y = vr.y;
  q.z = vr.z;

  return q;
}

template <typename Type>
bool plQuatTemplate<Type>::IsValid(Type fEpsilon) const
{
  if (!GetVectorPart().IsValid())
    return false;
  if (!plMath::IsFinite(w))
    return false;

  Type n = x * x + y * y + z * z + w * w;

  return plMath::IsEqual(n, (Type)1, fEpsilon);
}

template <typename Type>
bool plQuatTemplate<Type>::IsNaN() const
{
  return plMath::IsNaN(x) || plMath::IsNaN(y) || plMath::IsNaN(z) || plMath::IsNaN(w);
}

template <typename Type>
bool plQuatTemplate<Type>::IsEqualRotation(const plQuatTemplate<Type>& qOther, Type fEpsilon) const
{
  if (GetVectorPart().IsEqual(qOther.GetVectorPart(), (Type)0.00001) && plMath::IsEqual(w, qOther.w, (Type)0.00001))
  {
    return true;
  }

  plVec3Template<Type> vA1, vA2;
  plAngle A1, A2;

  GetRotationAxisAndAngle(vA1, A1);
  qOther.GetRotationAxisAndAngle(vA2, A2);

  if ((A1.IsEqualSimple(A2, plAngle::MakeFromDegree(static_cast<float>(fEpsilon)))) && (vA1.IsEqual(vA2, fEpsilon)))
    return true;

  if ((A1.IsEqualSimple(-A2, plAngle::MakeFromDegree(static_cast<float>(fEpsilon)))) && (vA1.IsEqual(-vA2, fEpsilon)))
    return true;

  return false;
}

template <typename Type>
const plMat3Template<Type> plQuatTemplate<Type>::GetAsMat3() const
{
  PLASMA_NAN_ASSERT(this);

  plMat3Template<Type> m;

  const Type fTx = x + x;
  const Type fTy = y + y;
  const Type fTz = z + z;
  const Type fTwx = fTx * w;
  const Type fTwy = fTy * w;
  const Type fTwz = fTz * w;
  const Type fTxx = fTx * x;
  const Type fTxy = fTy * x;
  const Type fTxz = fTz * x;
  const Type fTyy = fTy * y;
  const Type fTyz = fTz * y;
  const Type fTzz = fTz * z;

  m.Element(0, 0) = (Type)1 - (fTyy + fTzz);
  m.Element(1, 0) = fTxy - fTwz;
  m.Element(2, 0) = fTxz + fTwy;
  m.Element(0, 1) = fTxy + fTwz;
  m.Element(1, 1) = (Type)1 - (fTxx + fTzz);
  m.Element(2, 1) = fTyz - fTwx;
  m.Element(0, 2) = fTxz - fTwy;
  m.Element(1, 2) = fTyz + fTwx;
  m.Element(2, 2) = (Type)1 - (fTxx + fTyy);
  return m;
}

template <typename Type>
const plMat4Template<Type> plQuatTemplate<Type>::GetAsMat4() const
{
  PLASMA_NAN_ASSERT(this);

  plMat4Template<Type> m;

  const Type fTx = x + x;
  const Type fTy = y + y;
  const Type fTz = z + z;
  const Type fTwx = fTx * w;
  const Type fTwy = fTy * w;
  const Type fTwz = fTz * w;
  const Type fTxx = fTx * x;
  const Type fTxy = fTy * x;
  const Type fTxz = fTz * x;
  const Type fTyy = fTy * y;
  const Type fTyz = fTz * y;
  const Type fTzz = fTz * z;

  m.Element(0, 0) = (Type)1 - (fTyy + fTzz);
  m.Element(1, 0) = fTxy - fTwz;
  m.Element(2, 0) = fTxz + fTwy;
  m.Element(3, 0) = (Type)0;
  m.Element(0, 1) = fTxy + fTwz;
  m.Element(1, 1) = (Type)1 - (fTxx + fTzz);
  m.Element(2, 1) = fTyz - fTwx;
  m.Element(3, 1) = (Type)0;
  m.Element(0, 2) = fTxz - fTwy;
  m.Element(1, 2) = fTyz + fTwx;
  m.Element(2, 2) = (Type)1 - (fTxx + fTyy);
  m.Element(3, 2) = (Type)0;
  m.Element(0, 3) = (Type)0;
  m.Element(1, 3) = (Type)0;
  m.Element(2, 3) = (Type)0;
  m.Element(3, 3) = (Type)1;
  return m;
}

template <typename Type>
plQuatTemplate<Type> plQuatTemplate<Type>::MakeFromMat3(const plMat3Template<Type>& m)
{
  PLASMA_NAN_ASSERT(&m);

  const Type trace = m.Element(0, 0) + m.Element(1, 1) + m.Element(2, 2);
  const Type half = (Type)0.5;

  Type val[4];

  if (trace > (Type)0)
  {
    Type s = plMath::Sqrt(trace + (Type)1);
    Type t = half / s;

    val[0] = (m.Element(1, 2) - m.Element(2, 1)) * t;
    val[1] = (m.Element(2, 0) - m.Element(0, 2)) * t;
    val[2] = (m.Element(0, 1) - m.Element(1, 0)) * t;

    val[3] = half * s;
  }
  else
  {
    const plInt32 next[] = {1, 2, 0};
    plInt32 i = 0;

    if (m.Element(1, 1) > m.Element(0, 0))
      i = 1;

    if (m.Element(2, 2) > m.Element(i, i))
      i = 2;

    plInt32 j = next[i];
    plInt32 k = next[j];

    Type s = plMath::Sqrt(m.Element(i, i) - (m.Element(j, j) + m.Element(k, k)) + (Type)1);
    Type t = half / s;

    val[i] = half * s;
    val[3] = (m.Element(j, k) - m.Element(k, j)) * t;
    val[j] = (m.Element(i, j) + m.Element(j, i)) * t;
    val[k] = (m.Element(i, k) + m.Element(k, i)) * t;
  }

  plQuatTemplate<Type> q;
  q.x = val[0];
  q.y = val[1];
  q.z = val[2];
  q.w = val[3];
  return q;
}

template <typename Type>
void plQuatTemplate<Type>::ReconstructFromMat3(const plMat3Template<Type>& mMat)
{
  const plVec3 x = (mMat * plVec3(1, 0, 0)).GetNormalized();
  const plVec3 y = (mMat * plVec3(0, 1, 0)).GetNormalized();
  const plVec3 z = x.CrossRH(y);

  plMat3 m;
  m.SetColumn(0, x);
  m.SetColumn(1, y);
  m.SetColumn(2, z);

  *this = plQuat::MakeFromMat3(m);
}

template <typename Type>
void plQuatTemplate<Type>::ReconstructFromMat4(const plMat4Template<Type>& mMat)
{
  const plVec3 x = mMat.TransformDirection(plVec3(1, 0, 0)).GetNormalized();
  const plVec3 y = mMat.TransformDirection(plVec3(0, 1, 0)).GetNormalized();
  const plVec3 z = x.CrossRH(y);

  plMat3 m;
  m.SetColumn(0, x);
  m.SetColumn(1, y);
  m.SetColumn(2, z);

  *this = plQuat::MakeFromMat3(m);
}

/*! \note This function will ALWAYS return a quaternion that rotates from one direction to another.
  If both directions are identical, it is the unit rotation (none). If they are exactly opposing, this will be
  ANY 180.0 degree rotation. That means the vectors will align perfectly, but there is no determine rotation for other points
  that might be rotated with this quaternion. If a main / fallback axis is needed to rotate points, you need to calculate
  such a rotation with other means.
*/
template <typename Type>
plQuatTemplate<Type> plQuatTemplate<Type>::MakeShortestRotation(const plVec3Template<Type>& vDirFrom, const plVec3Template<Type>& vDirTo)
{
  const plVec3Template<Type> v0 = vDirFrom.GetNormalized();
  const plVec3Template<Type> v1 = vDirTo.GetNormalized();

  const Type fDot = v0.Dot(v1);

  // if both vectors are identical -> no rotation needed
  if (plMath::IsEqual(fDot, (Type)1, (Type)0.0000001))
  {
    return MakeIdentity();
  }
  else if (plMath::IsEqual(fDot, (Type)-1, (Type)0.0000001)) // if both vectors are opposing
  {
    // find an axis, that is not identical and not opposing, plVec3Template::Cross-product to find perpendicular vector, rotate around that
    if (plMath::Abs(v0.Dot(plVec3Template<Type>(1, 0, 0))) < (Type)0.8)
      return MakeFromAxisAndAngle(v0.CrossRH(plVec3Template<Type>(1, 0, 0)).GetNormalized(), plAngle::MakeFromRadian(plMath::Pi<float>()));
    else
      return MakeFromAxisAndAngle(v0.CrossRH(plVec3Template<Type>(0, 1, 0)).GetNormalized(), plAngle::MakeFromRadian(plMath::Pi<float>()));
  }

  const plVec3Template<Type> c = v0.CrossRH(v1);
  const Type d = v0.Dot(v1);
  const Type s = plMath::Sqrt(((Type)1 + d) * (Type)2);

  PLASMA_ASSERT_DEBUG(c.IsValid(), "SetShortestRotation failed.");

  const Type fOneDivS = 1.0f / s;

  plQuatTemplate<Type> q;
  q.x = c.x * fOneDivS;
  q.y = c.y * fOneDivS;
  q.z = c.z * fOneDivS;
  q.w = s / (Type)2;
  q.Normalize();

  return q;
}

template <typename Type>
plQuatTemplate<Type> plQuatTemplate<Type>::MakeSlerp(const plQuatTemplate<Type>& qFrom, const plQuatTemplate<Type>& qTo, Type t)
{
  PLASMA_ASSERT_DEBUG((t >= (Type)0) && (t <= (Type)1), "Invalid lerp factor.");

  const Type one = 1;
  const Type qdelta = (Type)1 - (Type)0.001;

  const Type fDot = (qFrom.x * qTo.x + qFrom.y * qTo.y + qFrom.z * qTo.z + qFrom.w * qTo.w);

  Type cosTheta = fDot;

  bool bFlipSign = false;
  if (cosTheta < (Type)0)
  {
    bFlipSign = true;
    cosTheta = -cosTheta;
  }

  Type t0, t1;

  if (cosTheta < qdelta)
  {
    plAngle theta = plMath::ACos((float)cosTheta);

    // use sqrtInv(1+c^2) instead of 1.0/sin(theta)
    const Type iSinTheta = (Type)1 / plMath::Sqrt(one - (cosTheta * cosTheta));
    const plAngle tTheta = static_cast<float>(t) * theta;

    Type s0 = plMath::Sin(theta - tTheta);
    Type s1 = plMath::Sin(tTheta);

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

  plQuatTemplate<Type> q;

  q.x = t0 * qFrom.x;
  q.y = t0 * qFrom.y;
  q.z = t0 * qFrom.z;
  q.w = t0 * qFrom.w;

  q.x += t1 * qTo.x;
  q.y += t1 * qTo.y;
  q.z += t1 * qTo.z;
  q.w += t1 * qTo.w;

  q.Normalize();

  return q;
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool operator==(const plQuatTemplate<Type>& q1, const plQuatTemplate<Type>& q2)
{
  return q1.x == q2.x && q1.y == q2.y && q1.z == q2.z && q1.w == q2.w;
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool operator!=(const plQuatTemplate<Type>& q1, const plQuatTemplate<Type>& q2)
{
  return !(q1 == q2);
}

template <typename Type>
void plQuatTemplate<Type>::GetAsEulerAngles(plAngle& out_x, plAngle& out_y, plAngle& out_z) const
{
  // Taken from https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  // and http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
  // adapted to our convention (yaw->pitch->roll, ZYX order or 3-2-1 order)

  auto& yaw = out_z;
  auto& pitch = out_y;
  auto& roll = out_x;

  const double fSingularityTest = w * y - z * x;
  const double fSingularityThreshold = 0.4999995;

  if (fSingularityTest > fSingularityThreshold) // singularity at north pole
  {
    yaw = -2.0f * plMath::ATan2(x, w);
    pitch = plAngle::MakeFromDegree(90.0f);
    roll = plAngle::MakeFromDegree(0.0f);
  }
  else if (fSingularityTest < -fSingularityThreshold) // singularity at south pole
  {
    yaw = 2.0f * plMath::ATan2(x, w);
    pitch = plAngle::MakeFromDegree(-90.0f);
    roll = plAngle::MakeFromDegree(0.0f);
  }
  else
  {
    // yaw (z-axis rotation)
    const double siny = 2.0 * (w * z + x * y);
    const double cosy = 1.0 - 2.0 * (y * y + z * z);
    yaw = plMath::ATan2((float)siny, (float)cosy);

    // pitch (y-axis rotation)
    pitch = plMath::ASin(2.0f * (float)fSingularityTest);

    // roll (x-axis rotation)
    const double sinr = 2.0 * (w * x + y * z);
    const double cosr = 1.0 - 2.0 * (x * x + y * y);
    roll = plMath::ATan2((float)sinr, (float)cosr);
  }
}

template <typename Type>
plQuatTemplate<Type> plQuatTemplate<Type>::MakeFromEulerAngles(const plAngle& x, const plAngle& y, const plAngle& z)
{
  /// Taken from here (yaw->pitch->roll, ZYX order or 3-2-1 order):
  /// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  const auto& yaw = z;
  const auto& pitch = y;
  const auto& roll = x;
  const double cy = plMath::Cos(yaw * 0.5);
  const double sy = plMath::Sin(yaw * 0.5);
  const double cp = plMath::Cos(pitch * 0.5);
  const double sp = plMath::Sin(pitch * 0.5);
  const double cr = plMath::Cos(roll * 0.5);
  const double sr = plMath::Sin(roll * 0.5);

  plQuatTemplate<Type> q;
  q.w = (float)(cy * cp * cr + sy * sp * sr);
  q.x = (float)(cy * cp * sr - sy * sp * cr);
  q.y = (float)(cy * sp * cr + sy * cp * sr);
  q.z = (float)(sy * cp * cr - cy * sp * sr);
  return q;
}

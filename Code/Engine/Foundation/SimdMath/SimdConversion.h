#pragma once

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>

namespace plSimdConversion
{
  PLASMA_ALWAYS_INLINE plVec3 ToVec3(const plSimdVec4f& v)
  {
    plVec4 tmp;
    v.Store<4>(&tmp.x);
    return *reinterpret_cast<plVec3*>(&tmp.x);
  }

  PLASMA_ALWAYS_INLINE plSimdVec4f ToVec3(const plVec3& v)
  {
    plSimdVec4f tmp;
    tmp.Load<3>(&v.x);
    return tmp;
  }

  PLASMA_ALWAYS_INLINE plVec4 ToVec4(const plSimdVec4f& v)
  {
    plVec4 tmp;
    v.Store<4>(&tmp.x);
    return tmp;
  }

  PLASMA_ALWAYS_INLINE plSimdVec4f ToVec4(const plVec4& v)
  {
    plSimdVec4f tmp;
    tmp.Load<4>(&v.x);
    return tmp;
  }

  PLASMA_ALWAYS_INLINE plQuat ToQuat(const plSimdQuat& q)
  {
    plQuat tmp;
    q.m_v.Store<4>(&tmp.v.x);
    return tmp;
  }

  PLASMA_ALWAYS_INLINE plSimdQuat ToQuat(const plQuat& q)
  {
    plSimdVec4f tmp;
    tmp.Load<4>(&q.v.x);
    return plSimdQuat(tmp);
  }

  PLASMA_ALWAYS_INLINE plTransform ToTransform(const plSimdTransform& t)
  {
    return plTransform(ToVec3(t.m_Position), ToQuat(t.m_Rotation), ToVec3(t.m_Scale));
  }

  inline plSimdTransform ToTransform(const plTransform& t)
  {
    return plSimdTransform(ToVec3(t.m_vPosition), ToQuat(t.m_qRotation), ToVec3(t.m_vScale));
  }

  PLASMA_ALWAYS_INLINE plMat4 ToMat4(const plSimdMat4f& m)
  {
    plMat4 tmp;
    m.GetAsArray(tmp.m_fElementsCM, plMatrixLayout::ColumnMajor);
    return tmp;
  }

  PLASMA_ALWAYS_INLINE plSimdMat4f ToMat4(const plMat4& m)
  {
    plSimdMat4f tmp;
    tmp.SetFromArray(m.m_fElementsCM, plMatrixLayout::ColumnMajor);
    return tmp;
  }

  PLASMA_ALWAYS_INLINE plBoundingBoxSphere ToBBoxSphere(const plSimdBBoxSphere& b)
  {
    plVec4 centerAndRadius = ToVec4(b.m_CenterAndRadius);
    return plBoundingBoxSphere(centerAndRadius.GetAsVec3(), ToVec3(b.m_BoxHalfExtents), centerAndRadius.w);
  }

  PLASMA_ALWAYS_INLINE plSimdBBoxSphere ToBBoxSphere(const plBoundingBoxSphere& b)
  {
    return plSimdBBoxSphere(ToVec3(b.m_vCenter), ToVec3(b.m_vBoxHalfExtends), b.m_fSphereRadius);
  }

  PLASMA_ALWAYS_INLINE plBoundingSphere ToBSphere(const plSimdBSphere& s)
  {
    plVec4 centerAndRadius = ToVec4(s.m_CenterAndRadius);
    return plBoundingSphere(centerAndRadius.GetAsVec3(), centerAndRadius.w);
  }

  PLASMA_ALWAYS_INLINE plSimdBSphere ToBSphere(const plBoundingSphere& s) { return plSimdBSphere(ToVec3(s.m_vCenter), s.m_fRadius); }

  PLASMA_ALWAYS_INLINE plSimdBBox ToBBox(const plBoundingBox& b) { return plSimdBBox(ToVec3(b.m_vMin), ToVec3(b.m_vMax)); }

  PLASMA_ALWAYS_INLINE plBoundingBox ToBBox(const plSimdBBox& b) { return plBoundingBox(ToVec3(b.m_Min), ToVec3(b.m_Max)); }

}; // namespace plSimdConversion

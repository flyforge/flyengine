#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Jolt/Core/Color.h>
#include <Jolt/Math/Float3.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Vec4.h>

namespace plJoltConversionUtils
{
  PL_ALWAYS_INLINE plVec3 ToVec3(const JPH::Vec3& v)
  {
    return plVec3(v.mF32[0], v.mF32[1], v.mF32[2]);
  }

  PL_ALWAYS_INLINE plVec3 ToVec3(const JPH::Float3& v)
  {
    return reinterpret_cast<const plVec3&>(v);
  }

  PL_ALWAYS_INLINE plColor ToColor(const JPH::ColorArg& c)
  {
    const JPH::Vec4 v4 = c.ToVec4();
    return reinterpret_cast<const plColor&>(v4);
  }

  PL_ALWAYS_INLINE plSimdVec4f ToSimdVec3(const JPH::Vec3& v)
  {
    return plSimdVec4f(v.mF32[0], v.mF32[1], v.mF32[2], v.mF32[3]);
  }

  PL_ALWAYS_INLINE JPH::Vec3 ToVec3(const plVec3& v)
  {
    return JPH::Vec3(v.x, v.y, v.z);
  }

  PL_ALWAYS_INLINE JPH::Float3 ToFloat3(const plVec3& v)
  {
    return reinterpret_cast<const JPH::Float3&>(v);
  }

  PL_ALWAYS_INLINE JPH::Vec3 ToVec3(const plSimdVec4f& v)
  {
    return reinterpret_cast<const JPH::Vec3&>(v);
  }

  PL_ALWAYS_INLINE plQuat ToQuat(const JPH::Quat& q)
  {
    return reinterpret_cast<const plQuat&>(q);
  }

  PL_ALWAYS_INLINE plSimdQuat ToSimdQuat(const JPH::Quat& q)
  {
    return reinterpret_cast<const plSimdQuat&>(q);
  }

  PL_ALWAYS_INLINE JPH::Quat ToQuat(const plQuat& q)
  {
    return JPH::Quat(q.x, q.y, q.z, q.w);
  }

  PL_ALWAYS_INLINE JPH::Quat ToQuat(const plSimdQuat& q)
  {
    return reinterpret_cast<const JPH::Quat&>(q);
  }

  PL_ALWAYS_INLINE plTransform ToTransform(const JPH::Vec3& pos, const JPH::Quat& rot)
  {
    return plTransform(ToVec3(pos), ToQuat(rot));
  }

  PL_ALWAYS_INLINE plTransform ToTransform(const JPH::Vec3& pos)
  {
    return plTransform(ToVec3(pos));
  }

} // namespace plJoltConversionUtils

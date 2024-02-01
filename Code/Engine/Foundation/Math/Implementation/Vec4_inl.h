#pragma once

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>

// *** Vec2 and Vec3 Code ***
// Cannot put this into the Vec3_inl.h file, that would result in circular dependencies

template <typename Type>
PL_FORCE_INLINE const plVec3Template<Type> plVec2Template<Type>::GetAsVec3(Type z) const
{
  PL_NAN_ASSERT(this);

  return plVec3Template<Type>(x, y, z);
}

template <typename Type>
PL_FORCE_INLINE const plVec4Template<Type> plVec2Template<Type>::GetAsVec4(Type z, Type w) const
{
  PL_NAN_ASSERT(this);

  return plVec4Template<Type>(x, y, z, w);
}

template <typename Type>
PL_FORCE_INLINE const plVec2Template<Type> plVec3Template<Type>::GetAsVec2() const
{
  // don't assert here, as the 3rd and 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // PL_NAN_ASSERT(this);

  return plVec2Template<Type>(x, y);
}

template <typename Type>
PL_FORCE_INLINE const plVec4Template<Type> plVec3Template<Type>::GetAsVec4(Type w) const
{
  PL_NAN_ASSERT(this);

  return plVec4Template<Type>(x, y, z, w);
}

template <typename Type>
PL_FORCE_INLINE const plVec4Template<Type> plVec3Template<Type>::GetAsPositionVec4() const
{
  // don't assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // PL_NAN_ASSERT(this);

  return plVec4Template<Type>(x, y, z, 1);
}

template <typename Type>
PL_FORCE_INLINE const plVec4Template<Type> plVec3Template<Type>::GetAsDirectionVec4() const
{
  // don't assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // PL_NAN_ASSERT(this);

  return plVec4Template<Type>(x, y, z, 0);
}

// *****************

template <typename Type>
PL_ALWAYS_INLINE plVec4Template<Type>::plVec4Template()
{
#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = plMath::NaN<Type>();
  x = TypeNaN;
  y = TypeNaN;
  z = 0;
  w = 0;
#endif
}

template <typename Type>
PL_ALWAYS_INLINE plVec4Template<Type>::plVec4Template(Type x, Type y, Type z, Type w)
  : x(x)
  , y(y)
  , z(z)
  , w(w)
{
}

template <typename Type>
PL_ALWAYS_INLINE plVec4Template<Type>::plVec4Template(plVec3Template<Type> vXyz, Type w)
  : x(vXyz.x)
  , y(vXyz.y)
  , z(vXyz.z)
  , w(w)
{
}

template <typename Type>
PL_ALWAYS_INLINE plVec4Template<Type>::plVec4Template(Type v)
  : x(v)
  , y(v)
  , z(v)
  , w(v)
{
}

template <typename Type>
PL_FORCE_INLINE const plVec2Template<Type> plVec4Template<Type>::GetAsVec2() const
{
  // don't assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // PL_NAN_ASSERT(this);

  return plVec2Template<Type>(x, y);
}

template <typename Type>
PL_FORCE_INLINE const plVec3Template<Type> plVec4Template<Type>::GetAsVec3() const
{
  // don't assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // PL_NAN_ASSERT(this);

  return plVec3Template<Type>(x, y, z);
}

template <typename Type>
PL_ALWAYS_INLINE void plVec4Template<Type>::Set(Type xyzw)
{
  x = xyzw;
  y = xyzw;
  z = xyzw;
  w = xyzw;
}

template <typename Type>
PL_ALWAYS_INLINE void plVec4Template<Type>::Set(Type inX, Type inY, Type inZ, Type inW)
{
  x = inX;
  y = inY;
  z = inZ;
  w = inW;
}

template <typename Type>
inline void plVec4Template<Type>::SetZero()
{
  x = y = z = w = 0;
}

template <typename Type>
PL_IMPLEMENT_IF_FLOAT_TYPE PL_ALWAYS_INLINE Type plVec4Template<Type>::GetLength() const
{
  return (plMath::Sqrt(GetLengthSquared()));
}

template <typename Type>
PL_FORCE_INLINE Type plVec4Template<Type>::GetLengthSquared() const
{
  PL_NAN_ASSERT(this);

  return (x * x + y * y + z * z + w * w);
}

template <typename Type>
PL_IMPLEMENT_IF_FLOAT_TYPE PL_FORCE_INLINE Type plVec4Template<Type>::GetLengthAndNormalize()
{
  const Type fLength = GetLength();
  *this /= fLength;
  return fLength;
}

template <typename Type>
PL_IMPLEMENT_IF_FLOAT_TYPE PL_FORCE_INLINE const plVec4Template<Type> plVec4Template<Type>::GetNormalized() const
{
  const Type fLen = GetLength();

  const Type fLengthInv = plMath::Invert(fLen);
  return plVec4Template<Type>(x * fLengthInv, y * fLengthInv, z * fLengthInv, w * fLengthInv);
}

template <typename Type>
PL_IMPLEMENT_IF_FLOAT_TYPE PL_ALWAYS_INLINE void plVec4Template<Type>::Normalize()
{
  *this /= GetLength();
}

template <typename Type>
PL_IMPLEMENT_IF_FLOAT_TYPE inline plResult plVec4Template<Type>::NormalizeIfNotZero(const plVec4Template<Type>& vFallback, Type fEpsilon)
{
  PL_NAN_ASSERT(&vFallback);

  const Type fLength = GetLength();

  if (!plMath::IsFinite(fLength) || plMath::IsZero(fLength, fEpsilon))
  {
    *this = vFallback;
    return PL_FAILURE;
  }

  *this /= fLength;
  return PL_SUCCESS;
}

/*! \note Normalization, especially with SSE is not very precise. So this function checks whether the (squared)
  length is between a lower and upper limit.
*/
template <typename Type>
PL_IMPLEMENT_IF_FLOAT_TYPE inline bool plVec4Template<Type>::IsNormalized(Type fEpsilon /* = plMath::HugeEpsilon<Type>() */) const
{
  const Type t = GetLengthSquared();
  return plMath::IsEqual(t, (Type)1, fEpsilon);
}

template <typename Type>
inline bool plVec4Template<Type>::IsZero() const
{
  PL_NAN_ASSERT(this);

  return ((x == 0.0f) && (y == 0.0f) && (z == 0.0f) && (w == 0.0f));
}

template <typename Type>
inline bool plVec4Template<Type>::IsZero(Type fEpsilon) const
{
  PL_NAN_ASSERT(this);

  return (plMath::IsZero(x, fEpsilon) && plMath::IsZero(y, fEpsilon) && plMath::IsZero(z, fEpsilon) && plMath::IsZero(w, fEpsilon));
}

template <typename Type>
inline bool plVec4Template<Type>::IsNaN() const
{
  if (plMath::IsNaN(x))
    return true;
  if (plMath::IsNaN(y))
    return true;
  if (plMath::IsNaN(z))
    return true;
  if (plMath::IsNaN(w))
    return true;

  return false;
}

template <typename Type>
inline bool plVec4Template<Type>::IsValid() const
{
  if (!plMath::IsFinite(x))
    return false;
  if (!plMath::IsFinite(y))
    return false;
  if (!plMath::IsFinite(z))
    return false;
  if (!plMath::IsFinite(w))
    return false;

  return true;
}

template <typename Type>
PL_FORCE_INLINE const plVec4Template<Type> plVec4Template<Type>::operator-() const
{
  PL_NAN_ASSERT(this);

  return plVec4Template<Type>(-x, -y, -z, -w);
}

template <typename Type>
PL_FORCE_INLINE void plVec4Template<Type>::operator+=(const plVec4Template<Type>& vCc)
{
  x += vCc.x;
  y += vCc.y;
  z += vCc.z;
  w += vCc.w;

  PL_NAN_ASSERT(this);
}

template <typename Type>
PL_FORCE_INLINE void plVec4Template<Type>::operator-=(const plVec4Template<Type>& vCc)
{
  x -= vCc.x;
  y -= vCc.y;
  z -= vCc.z;
  w -= vCc.w;

  PL_NAN_ASSERT(this);
}

template <typename Type>
PL_FORCE_INLINE void plVec4Template<Type>::operator*=(Type f)
{
  x *= f;
  y *= f;
  z *= f;
  w *= f;

  PL_NAN_ASSERT(this);
}

template <typename Type>
PL_FORCE_INLINE void plVec4Template<Type>::operator/=(Type f)
{
  if constexpr (std::is_floating_point_v<Type>)
  {
    const Type f_inv = plMath::Invert(f);
    x *= f_inv;
    y *= f_inv;
    z *= f_inv;
    w *= f_inv;
  }
  else
  {
    x /= f;
    y /= f;
    z /= f;
    w /= f;
  }

  PL_NAN_ASSERT(this);
}

template <typename Type>
PL_FORCE_INLINE Type plVec4Template<Type>::Dot(const plVec4Template<Type>& rhs) const
{
  PL_NAN_ASSERT(this);
  PL_NAN_ASSERT(&rhs);

  return ((x * rhs.x) + (y * rhs.y) + (z * rhs.z) + (w * rhs.w));
}

template <typename Type>
inline const plVec4Template<Type> plVec4Template<Type>::CompMin(const plVec4Template<Type>& rhs) const
{
  PL_NAN_ASSERT(this);
  PL_NAN_ASSERT(&rhs);

  return plVec4Template<Type>(plMath::Min(x, rhs.x), plMath::Min(y, rhs.y), plMath::Min(z, rhs.z), plMath::Min(w, rhs.w));
}

template <typename Type>
inline const plVec4Template<Type> plVec4Template<Type>::CompMax(const plVec4Template<Type>& rhs) const
{
  PL_NAN_ASSERT(this);
  PL_NAN_ASSERT(&rhs);

  return plVec4Template<Type>(plMath::Max(x, rhs.x), plMath::Max(y, rhs.y), plMath::Max(z, rhs.z), plMath::Max(w, rhs.w));
}

template <typename Type>
inline const plVec4Template<Type> plVec4Template<Type>::CompClamp(const plVec4Template& vLow, const plVec4Template& vHigh) const
{
  PL_NAN_ASSERT(this);
  PL_NAN_ASSERT(&vLow);
  PL_NAN_ASSERT(&vHigh);

  return plVec4Template<Type>(plMath::Clamp(x, vLow.x, vHigh.x), plMath::Clamp(y, vLow.y, vHigh.y), plMath::Clamp(z, vLow.z, vHigh.z), plMath::Clamp(w, vLow.w, vHigh.w));
}

template <typename Type>
inline const plVec4Template<Type> plVec4Template<Type>::CompMul(const plVec4Template<Type>& rhs) const
{
  PL_NAN_ASSERT(this);
  PL_NAN_ASSERT(&rhs);

  return plVec4Template<Type>(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
}

PL_MSVC_ANALYSIS_WARNING_PUSH
PL_MSVC_ANALYSIS_WARNING_DISABLE(4723)
template <typename Type>
inline const plVec4Template<Type> plVec4Template<Type>::CompDiv(const plVec4Template<Type>& rhs) const
{
  PL_NAN_ASSERT(this);
  PL_NAN_ASSERT(&rhs);

  return plVec4Template<Type>(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
}
PL_MSVC_ANALYSIS_WARNING_POP

template <typename Type>
inline const plVec4Template<Type> plVec4Template<Type>::Abs() const
{
  PL_NAN_ASSERT(this);

  return plVec4Template<Type>(plMath::Abs(x), plMath::Abs(y), plMath::Abs(z), plMath::Abs(w));
}

template <typename Type>
PL_FORCE_INLINE const plVec4Template<Type> operator+(const plVec4Template<Type>& v1, const plVec4Template<Type>& v2)
{
  PL_NAN_ASSERT(&v1);
  PL_NAN_ASSERT(&v2);

  return plVec4Template<Type>(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}

template <typename Type>
PL_FORCE_INLINE const plVec4Template<Type> operator-(const plVec4Template<Type>& v1, const plVec4Template<Type>& v2)
{
  PL_NAN_ASSERT(&v1);
  PL_NAN_ASSERT(&v2);

  return plVec4Template<Type>(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}

template <typename Type>
PL_FORCE_INLINE const plVec4Template<Type> operator*(Type f, const plVec4Template<Type>& v)
{
  PL_NAN_ASSERT(&v);

  return plVec4Template<Type>(v.x * f, v.y * f, v.z * f, v.w * f);
}

template <typename Type>
PL_FORCE_INLINE const plVec4Template<Type> operator*(const plVec4Template<Type>& v, Type f)
{
  PL_NAN_ASSERT(&v);

  return plVec4Template<Type>(v.x * f, v.y * f, v.z * f, v.w * f);
}

template <typename Type>
PL_FORCE_INLINE const plVec4Template<Type> operator/(const plVec4Template<Type>& v, Type f)
{
  PL_NAN_ASSERT(&v);

  if constexpr (std::is_floating_point_v<Type>)
  {
    // multiplication is much faster than division
    const Type f_inv = plMath::Invert(f);
    return plVec4Template<Type>(v.x * f_inv, v.y * f_inv, v.z * f_inv, v.w * f_inv);
  }
  else
  {
    return plVec4Template<Type>(v.x / f, v.y / f, v.z / f, v.w / f);
  }
}

template <typename Type>
inline bool plVec4Template<Type>::IsIdentical(const plVec4Template<Type>& rhs) const
{
  PL_NAN_ASSERT(this);
  PL_NAN_ASSERT(&rhs);

  return ((x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (w == rhs.w));
}

template <typename Type>
inline bool plVec4Template<Type>::IsEqual(const plVec4Template<Type>& rhs, Type fEpsilon) const
{
  PL_NAN_ASSERT(this);
  PL_NAN_ASSERT(&rhs);

  return (plMath::IsEqual(x, rhs.x, fEpsilon) && plMath::IsEqual(y, rhs.y, fEpsilon) && plMath::IsEqual(z, rhs.z, fEpsilon) && plMath::IsEqual(w, rhs.w, fEpsilon));
}

template <typename Type>
PL_ALWAYS_INLINE bool operator==(const plVec4Template<Type>& v1, const plVec4Template<Type>& v2)
{
  return v1.IsIdentical(v2);
}

template <typename Type>
PL_ALWAYS_INLINE bool operator!=(const plVec4Template<Type>& v1, const plVec4Template<Type>& v2)
{
  return !v1.IsIdentical(v2);
}

template <typename Type>
PL_FORCE_INLINE bool operator<(const plVec4Template<Type>& v1, const plVec4Template<Type>& v2)
{
  PL_NAN_ASSERT(&v1);
  PL_NAN_ASSERT(&v2);

  if (v1.x < v2.x)
    return true;
  if (v1.x > v2.x)
    return false;
  if (v1.y < v2.y)
    return true;
  if (v1.y > v2.y)
    return false;
  if (v1.z < v2.z)
    return true;
  if (v1.z > v2.z)
    return false;

  return (v1.w < v2.w);
}

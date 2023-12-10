#pragma once

template <typename Type>
PLASMA_FORCE_INLINE plVec3Template<Type>::plVec3Template()
{
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = plMath::NaN<Type>();
  x = TypeNaN;
  y = TypeNaN;
  z = TypeNaN;
#endif
}

template <typename Type>
PLASMA_ALWAYS_INLINE plVec3Template<Type>::plVec3Template(Type x, Type y, Type z)
  : x(x)
  , y(y)
  , z(z)
{
}

template <typename Type>
PLASMA_ALWAYS_INLINE plVec3Template<Type>::plVec3Template(Type v)
  : x(v)
  , y(v)
  , z(v)
{
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plVec3Template<Type>::Set(Type xyz)
{
  x = xyz;
  y = xyz;
  z = xyz;
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plVec3Template<Type>::Set(Type inX, Type inY, Type inZ)
{
  x = inX;
  y = inY;
  z = inZ;
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plVec3Template<Type>::SetZero()
{
  x = y = z = 0;
}

template <typename Type>
PLASMA_ALWAYS_INLINE Type plVec3Template<Type>::GetLength() const
{
  return (plMath::Sqrt(GetLengthSquared()));
}

template <typename Type>
plResult plVec3Template<Type>::SetLength(Type fNewLength, Type fEpsilon /* = plMath::DefaultEpsilon<Type>() */)
{
  if (NormalizeIfNotZero(plVec3Template<Type>::MakeZero(), fEpsilon) == PLASMA_FAILURE)
    return PLASMA_FAILURE;

  *this *= fNewLength;
  return PLASMA_SUCCESS;
}

template <typename Type>
PLASMA_FORCE_INLINE Type plVec3Template<Type>::GetLengthSquared() const
{
  PLASMA_NAN_ASSERT(this);

  return (x * x + y * y + z * z);
}

template <typename Type>
PLASMA_FORCE_INLINE Type plVec3Template<Type>::GetLengthAndNormalize()
{
  const Type fLength = GetLength();
  *this /= fLength;
  return fLength;
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> plVec3Template<Type>::GetNormalized() const
{
  const Type fLen = GetLength();

  const Type fLengthInv = plMath::Invert(fLen);
  return plVec3Template<Type>(x * fLengthInv, y * fLengthInv, z * fLengthInv);
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plVec3Template<Type>::Normalize()
{
  *this /= GetLength();
}

template <typename Type>
plResult plVec3Template<Type>::NormalizeIfNotZero(const plVec3Template<Type>& vFallback, Type fEpsilon)
{
  PLASMA_NAN_ASSERT(&vFallback);

  const Type fLength = GetLength();

  if (!plMath::IsFinite(fLength) || plMath::IsZero(fLength, fEpsilon))
  {
    *this = vFallback;
    return PLASMA_FAILURE;
  }

  *this /= fLength;
  return PLASMA_SUCCESS;
}

/*! \note Normalization, especially with SSE is not very precise. So this function checks whether the (squared)
  length is between a lower and upper limit.
*/
template <typename Type>
PLASMA_FORCE_INLINE bool plVec3Template<Type>::IsNormalized(Type fEpsilon /* = plMath::HugeEpsilon<Type>() */) const
{
  const Type t = GetLengthSquared();
  return plMath::IsEqual(t, (Type)1, fEpsilon);
}

template <typename Type>
PLASMA_FORCE_INLINE bool plVec3Template<Type>::IsZero() const
{
  PLASMA_NAN_ASSERT(this);

  return ((x == 0.0f) && (y == 0.0f) && (z == 0.0f));
}

template <typename Type>
bool plVec3Template<Type>::IsZero(Type fEpsilon) const
{
  PLASMA_NAN_ASSERT(this);

  return (plMath::IsZero(x, fEpsilon) && plMath::IsZero(y, fEpsilon) && plMath::IsZero(z, fEpsilon));
}

template <typename Type>
bool plVec3Template<Type>::IsNaN() const
{
  if (plMath::IsNaN(x))
    return true;
  if (plMath::IsNaN(y))
    return true;
  if (plMath::IsNaN(z))
    return true;

  return false;
}

template <typename Type>
bool plVec3Template<Type>::IsValid() const
{
  if (!plMath::IsFinite(x))
    return false;
  if (!plMath::IsFinite(y))
    return false;
  if (!plMath::IsFinite(z))
    return false;

  return true;
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> plVec3Template<Type>::operator-() const
{
  PLASMA_NAN_ASSERT(this);

  return plVec3Template<Type>(-x, -y, -z);
}

template <typename Type>
PLASMA_FORCE_INLINE void plVec3Template<Type>::operator+=(const plVec3Template<Type>& rhs)
{
  x += rhs.x;
  y += rhs.y;
  z += rhs.z;

  PLASMA_NAN_ASSERT(this);
}

template <typename Type>
PLASMA_FORCE_INLINE void plVec3Template<Type>::operator-=(const plVec3Template<Type>& rhs)
{
  x -= rhs.x;
  y -= rhs.y;
  z -= rhs.z;

  PLASMA_NAN_ASSERT(this);
}

template <typename Type>
PLASMA_FORCE_INLINE void plVec3Template<Type>::operator*=(const plVec3Template& rhs)
{
  /// \test this is new

  x *= rhs.x;
  y *= rhs.y;
  z *= rhs.z;

  PLASMA_NAN_ASSERT(this);
}

template <typename Type>
PLASMA_FORCE_INLINE void plVec3Template<Type>::operator/=(const plVec3Template& rhs)
{
  /// \test this is new

  x /= rhs.x;
  y /= rhs.y;
  z /= rhs.z;

  PLASMA_NAN_ASSERT(this);
}

template <typename Type>
PLASMA_FORCE_INLINE void plVec3Template<Type>::operator*=(Type f)
{
  x *= f;
  y *= f;
  z *= f;

  PLASMA_NAN_ASSERT(this);
}

template <typename Type>
PLASMA_FORCE_INLINE void plVec3Template<Type>::operator/=(Type f)
{
  const Type f_inv = plMath::Invert(f);

  x *= f_inv;
  y *= f_inv;
  z *= f_inv;

  // if this assert fires, you might have tried to normalize a zero-length vector
  PLASMA_NAN_ASSERT(this);
}

template <typename Type>
plResult plVec3Template<Type>::CalculateNormal(const plVec3Template<Type>& v1, const plVec3Template<Type>& v2, const plVec3Template<Type>& v3)
{
  *this = (v3 - v2).CrossRH(v1 - v2);
  return NormalizeIfNotZero();
}

template <typename Type>
void plVec3Template<Type>::MakeOrthogonalTo(const plVec3Template<Type>& vNormal)
{
  PLASMA_ASSERT_DEBUG(
    vNormal.IsNormalized(), "The vector to make this vector orthogonal to, must be normalized. It's length is {0}", plArgF(vNormal.GetLength(), 3));

  plVec3Template<Type> vOrtho = vNormal.CrossRH(*this);
  *this = vOrtho.CrossRH(vNormal);
}

template <typename Type>
const plVec3Template<Type> plVec3Template<Type>::GetOrthogonalVector() const
{
  PLASMA_ASSERT_DEBUG(!IsZero(plMath::SmallEpsilon<Type>()), "The vector must not be zero to be able to compute an orthogonal vector.");

  Type fDot = plMath::Abs(this->Dot(plVec3Template<Type>(0, 1, 0)));
  if (fDot < 0.999f)
    return this->CrossRH(plVec3Template<Type>(0, 1, 0));

  return this->CrossRH(plVec3Template<Type>(1, 0, 0));
}

template <typename Type>
const plVec3Template<Type> plVec3Template<Type>::GetReflectedVector(const plVec3Template<Type>& vNormal) const
{
  PLASMA_ASSERT_DEBUG(vNormal.IsNormalized(), "vNormal must be normalized.");

  return ((*this) - ((Type)2 * this->Dot(vNormal) * vNormal));
}

template <typename Type>
PLASMA_FORCE_INLINE Type plVec3Template<Type>::Dot(const plVec3Template<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return ((x * rhs.x) + (y * rhs.y) + (z * rhs.z));
}

template <typename Type>
const plVec3Template<Type> plVec3Template<Type>::CrossRH(const plVec3Template<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return plVec3Template<Type>(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
}

template <typename Type>
plAngle plVec3Template<Type>::GetAngleBetween(const plVec3Template<Type>& rhs) const
{
  PLASMA_ASSERT_DEBUG(this->IsNormalized(), "This vector must be normalized.");
  PLASMA_ASSERT_DEBUG(rhs.IsNormalized(), "The other vector must be normalized.");

  return plMath::ACos(static_cast<float>(plMath::Clamp(this->Dot(rhs), (Type)-1, (Type)1)));
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> plVec3Template<Type>::CompMin(const plVec3Template<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return plVec3Template<Type>(plMath::Min(x, rhs.x), plMath::Min(y, rhs.y), plMath::Min(z, rhs.z));
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> plVec3Template<Type>::CompMax(const plVec3Template<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return plVec3Template<Type>(plMath::Max(x, rhs.x), plMath::Max(y, rhs.y), plMath::Max(z, rhs.z));
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> plVec3Template<Type>::CompClamp(const plVec3Template& vLow, const plVec3Template& vHigh) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&vLow);
  PLASMA_NAN_ASSERT(&vHigh);

  return plVec3Template<Type>(plMath::Clamp(x, vLow.x, vHigh.x), plMath::Clamp(y, vLow.y, vHigh.y), plMath::Clamp(z, vLow.z, vHigh.z));
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> plVec3Template<Type>::CompMul(const plVec3Template<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return plVec3Template<Type>(x * rhs.x, y * rhs.y, z * rhs.z);
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> plVec3Template<Type>::CompDiv(const plVec3Template<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return plVec3Template<Type>(x / rhs.x, y / rhs.y, z / rhs.z);
}

template <typename Type>
inline const plVec3Template<Type> plVec3Template<Type>::Abs() const
{
  PLASMA_NAN_ASSERT(this);

  return plVec3Template<Type>(plMath::Abs(x), plMath::Abs(y), plMath::Abs(z));
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> operator+(const plVec3Template<Type>& v1, const plVec3Template<Type>& v2)
{
  PLASMA_NAN_ASSERT(&v1);
  PLASMA_NAN_ASSERT(&v2);

  return plVec3Template<Type>(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> operator-(const plVec3Template<Type>& v1, const plVec3Template<Type>& v2)
{
  PLASMA_NAN_ASSERT(&v1);
  PLASMA_NAN_ASSERT(&v2);

  return plVec3Template<Type>(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> operator*(Type f, const plVec3Template<Type>& v)
{
  PLASMA_NAN_ASSERT(&v);

  return plVec3Template<Type>(v.x * f, v.y * f, v.z * f);
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> operator*(const plVec3Template<Type>& v, Type f)
{
  PLASMA_NAN_ASSERT(&v);

  return plVec3Template<Type>(v.x * f, v.y * f, v.z * f);
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec3Template<Type> operator/(const plVec3Template<Type>& v, Type f)
{
  PLASMA_NAN_ASSERT(&v);

  // multiplication is much faster than division
  const Type f_inv = plMath::Invert(f);
  return plVec3Template<Type>(v.x * f_inv, v.y * f_inv, v.z * f_inv);
}

template <typename Type>
bool plVec3Template<Type>::IsIdentical(const plVec3Template<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return ((x == rhs.x) && (y == rhs.y) && (z == rhs.z));
}

template <typename Type>
bool plVec3Template<Type>::IsEqual(const plVec3Template<Type>& rhs, Type fEpsilon) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return (plMath::IsEqual(x, rhs.x, fEpsilon) && plMath::IsEqual(y, rhs.y, fEpsilon) && plMath::IsEqual(z, rhs.z, fEpsilon));
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool operator==(const plVec3Template<Type>& v1, const plVec3Template<Type>& v2)
{
  return v1.IsIdentical(v2);
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool operator!=(const plVec3Template<Type>& v1, const plVec3Template<Type>& v2)
{
  return !v1.IsIdentical(v2);
}

template <typename Type>
PLASMA_FORCE_INLINE bool operator<(const plVec3Template<Type>& v1, const plVec3Template<Type>& v2)
{
  PLASMA_NAN_ASSERT(&v1);
  PLASMA_NAN_ASSERT(&v2);

  if (v1.x < v2.x)
    return true;
  if (v1.x > v2.x)
    return false;
  if (v1.y < v2.y)
    return true;
  if (v1.y > v2.y)
    return false;

  return (v1.z < v2.z);
}

template <typename Type>
const plVec3Template<Type> plVec3Template<Type>::GetRefractedVector(const plVec3Template<Type>& vNormal, Type fRefIndex1, Type fRefIndex2) const
{
  PLASMA_ASSERT_DEBUG(vNormal.IsNormalized(), "vNormal must be normalized.");

  const Type n = fRefIndex1 / fRefIndex2;
  const Type cosI = this->Dot(vNormal);
  const Type sinT2 = n * n * (1.0f - (cosI * cosI));

  // invalid refraction
  if (sinT2 > 1.0f)
    return (*this);

  return ((n * (*this)) - (n + plMath::Sqrt(1.0f - sinT2)) * vNormal);
}

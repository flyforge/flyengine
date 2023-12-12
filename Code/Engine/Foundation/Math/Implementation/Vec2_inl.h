#pragma once

template <typename Type>
PLASMA_ALWAYS_INLINE plVec2Template<Type>::plVec2Template()
{
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = plMath::NaN<Type>();
  x = TypeNaN;
  y = TypeNaN;
#endif
}

template <typename Type>
PLASMA_ALWAYS_INLINE plVec2Template<Type>::plVec2Template(Type x, Type y)
  : x(x)
  , y(y)
{
}

template <typename Type>
PLASMA_ALWAYS_INLINE plVec2Template<Type>::plVec2Template(Type v)
  : x(v)
  , y(v)
{
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plVec2Template<Type>::Set(Type xy)
{
  x = xy;
  y = xy;
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plVec2Template<Type>::Set(Type inX, Type inY)
{
  x = inX;
  y = inY;
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plVec2Template<Type>::SetZero()
{
  x = y = 0;
}

template <typename Type>
PLASMA_ALWAYS_INLINE Type plVec2Template<Type>::GetLength() const
{
  return (plMath::Sqrt(GetLengthSquared()));
}

template <typename Type>
plResult plVec2Template<Type>::SetLength(Type fNewLength, Type fEpsilon /* = plMath::DefaultEpsilon<Type>() */)
{
  if (NormalizeIfNotZero(plVec2Template<Type>::ZeroVector(), fEpsilon) == PLASMA_FAILURE)
    return PLASMA_FAILURE;

  *this *= fNewLength;
  return PLASMA_SUCCESS;
}

template <typename Type>
PLASMA_ALWAYS_INLINE Type plVec2Template<Type>::GetLengthSquared() const
{
  return (x * x + y * y);
}

template <typename Type>
PLASMA_FORCE_INLINE Type plVec2Template<Type>::GetLengthAndNormalize()
{
  const Type fLength = GetLength();
  *this /= fLength;
  return fLength;
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec2Template<Type> plVec2Template<Type>::GetNormalized() const
{
  const Type fLen = GetLength();

  const Type fLengthInv = plMath::Invert(fLen);
  return plVec2Template<Type>(x * fLengthInv, y * fLengthInv);
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plVec2Template<Type>::Normalize()
{
  *this /= GetLength();
}

template <typename Type>
inline plResult plVec2Template<Type>::NormalizeIfNotZero(const plVec2Template<Type>& vFallback, Type fEpsilon)
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
inline bool plVec2Template<Type>::IsNormalized(Type fEpsilon /* = plMath::HugeEpsilon<Type>() */) const
{
  const Type t = GetLengthSquared();
  return plMath::IsEqual(t, (Type)(1), fEpsilon);
}

template <typename Type>
inline bool plVec2Template<Type>::IsZero() const
{
  return (x == 0 && y == 0);
}

template <typename Type>
inline bool plVec2Template<Type>::IsZero(Type fEpsilon) const
{
  PLASMA_NAN_ASSERT(this);

  return (plMath::IsZero(x, fEpsilon) && plMath::IsZero(y, fEpsilon));
}

template <typename Type>
inline bool plVec2Template<Type>::IsNaN() const
{
  if (plMath::IsNaN(x))
    return true;
  if (plMath::IsNaN(y))
    return true;

  return false;
}

template <typename Type>
inline bool plVec2Template<Type>::IsValid() const
{
  if (!plMath::IsFinite(x))
    return false;
  if (!plMath::IsFinite(y))
    return false;

  return true;
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec2Template<Type> plVec2Template<Type>::operator-() const
{
  PLASMA_NAN_ASSERT(this);

  return plVec2Template<Type>(-x, -y);
}

template <typename Type>
PLASMA_FORCE_INLINE void plVec2Template<Type>::operator+=(const plVec2Template<Type>& rhs)
{
  x += rhs.x;
  y += rhs.y;

  PLASMA_NAN_ASSERT(this);
}

template <typename Type>
PLASMA_FORCE_INLINE void plVec2Template<Type>::operator-=(const plVec2Template<Type>& rhs)
{
  x -= rhs.x;
  y -= rhs.y;

  PLASMA_NAN_ASSERT(this);
}

template <typename Type>
PLASMA_FORCE_INLINE void plVec2Template<Type>::operator*=(Type f)
{
  x *= f;
  y *= f;

  PLASMA_NAN_ASSERT(this);
}

template <typename Type>
PLASMA_FORCE_INLINE void plVec2Template<Type>::operator/=(Type f)
{
  const Type f_inv = plMath::Invert(f);

  x *= f_inv;
  y *= f_inv;

  PLASMA_NAN_ASSERT(this);
}

template <typename Type>
inline void plVec2Template<Type>::MakeOrthogonalTo(const plVec2Template<Type>& vNormal)
{
  PLASMA_ASSERT_DEBUG(vNormal.IsNormalized(), "The normal must be normalized.");

  const Type fDot = this->Dot(vNormal);
  *this -= fDot * vNormal;
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec2Template<Type> plVec2Template<Type>::GetOrthogonalVector() const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_ASSERT_DEBUG(!IsZero(plMath::SmallEpsilon<Type>()), "The vector must not be zero to be able to compute an orthogonal vector.");

  return plVec2Template<Type>(-y, x);
}

template <typename Type>
inline const plVec2Template<Type> plVec2Template<Type>::GetReflectedVector(const plVec2Template<Type>& vNormal) const
{
  PLASMA_ASSERT_DEBUG(vNormal.IsNormalized(), "vNormal must be normalized.");

  return ((*this) - (2 * this->Dot(vNormal) * vNormal));
}

template <typename Type>
PLASMA_FORCE_INLINE Type plVec2Template<Type>::Dot(const plVec2Template<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return ((x * rhs.x) + (y * rhs.y));
}

template <typename Type>
inline plAngle plVec2Template<Type>::GetAngleBetween(const plVec2Template<Type>& rhs) const
{
  PLASMA_ASSERT_DEBUG(this->IsNormalized(), "This vector must be normalized.");
  PLASMA_ASSERT_DEBUG(rhs.IsNormalized(), "The other vector must be normalized.");

  return plMath::ACos(static_cast<float>(plMath::Clamp<Type>(this->Dot(rhs), (Type)-1, (Type)1)));
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec2Template<Type> plVec2Template<Type>::CompMin(const plVec2Template<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return plVec2Template<Type>(plMath::Min(x, rhs.x), plMath::Min(y, rhs.y));
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec2Template<Type> plVec2Template<Type>::CompMax(const plVec2Template<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return plVec2Template<Type>(plMath::Max(x, rhs.x), plMath::Max(y, rhs.y));
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec2Template<Type> plVec2Template<Type>::CompClamp(const plVec2Template<Type>& vLow, const plVec2Template<Type>& vHigh) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&vLow);
  PLASMA_NAN_ASSERT(&vHigh);

  return plVec2Template<Type>(plMath::Clamp(x, vLow.x, vHigh.x), plMath::Clamp(y, vLow.y, vHigh.y));
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec2Template<Type> plVec2Template<Type>::CompMul(const plVec2Template<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return plVec2Template<Type>(x * rhs.x, y * rhs.y);
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec2Template<Type> plVec2Template<Type>::CompDiv(const plVec2Template<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return plVec2Template<Type>(x / rhs.x, y / rhs.y);
}

template <typename Type>
inline const plVec2Template<Type> plVec2Template<Type>::Abs() const
{
  PLASMA_NAN_ASSERT(this);

  return plVec2Template<Type>(plMath::Abs(x), plMath::Abs(y));
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec2Template<Type> operator+(const plVec2Template<Type>& v1, const plVec2Template<Type>& v2)
{
  PLASMA_NAN_ASSERT(&v1);
  PLASMA_NAN_ASSERT(&v2);

  return plVec2Template<Type>(v1.x + v2.x, v1.y + v2.y);
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec2Template<Type> operator-(const plVec2Template<Type>& v1, const plVec2Template<Type>& v2)
{
  PLASMA_NAN_ASSERT(&v1);
  PLASMA_NAN_ASSERT(&v2);

  return plVec2Template<Type>(v1.x - v2.x, v1.y - v2.y);
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec2Template<Type> operator*(Type f, const plVec2Template<Type>& v)
{
  PLASMA_NAN_ASSERT(&v);

  return plVec2Template<Type>(v.x * f, v.y * f);
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec2Template<Type> operator*(const plVec2Template<Type>& v, Type f)
{
  PLASMA_NAN_ASSERT(&v);

  return plVec2Template<Type>(v.x * f, v.y * f);
}

template <typename Type>
PLASMA_FORCE_INLINE const plVec2Template<Type> operator/(const plVec2Template<Type>& v, Type f)
{
  PLASMA_NAN_ASSERT(&v);

  // multiplication is much faster than division
  const Type f_inv = plMath::Invert(f);
  return plVec2Template<Type>(v.x * f_inv, v.y * f_inv);
}

template <typename Type>
inline bool plVec2Template<Type>::IsIdentical(const plVec2Template<Type>& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return ((x == rhs.x) && (y == rhs.y));
}

template <typename Type>
inline bool plVec2Template<Type>::IsEqual(const plVec2Template<Type>& rhs, Type fEpsilon) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return (plMath::IsEqual(x, rhs.x, fEpsilon) && plMath::IsEqual(y, rhs.y, fEpsilon));
}

template <typename Type>
PLASMA_FORCE_INLINE bool operator==(const plVec2Template<Type>& v1, const plVec2Template<Type>& v2)
{
  return v1.IsIdentical(v2);
}

template <typename Type>
PLASMA_FORCE_INLINE bool operator!=(const plVec2Template<Type>& v1, const plVec2Template<Type>& v2)
{
  return !v1.IsIdentical(v2);
}

template <typename Type>
PLASMA_FORCE_INLINE bool operator<(const plVec2Template<Type>& v1, const plVec2Template<Type>& v2)
{
  PLASMA_NAN_ASSERT(&v1);
  PLASMA_NAN_ASSERT(&v2);

  if (v1.x < v2.x)
    return true;
  if (v1.x > v2.x)
    return false;

  return (v1.y < v2.y);
}

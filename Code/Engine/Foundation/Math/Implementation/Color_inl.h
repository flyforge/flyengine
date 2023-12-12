#pragma once

inline plColor::plColor()
{
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const float TypeNaN = plMath::NaN<float>();
  r = TypeNaN;
  g = TypeNaN;
  b = TypeNaN;
  a = TypeNaN;
#endif
}

PLASMA_FORCE_INLINE constexpr plColor::plColor(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha /* = 1.0f */)
  : r(fLinearRed)
  , g(fLinearGreen)
  , b(fLinearBlue)
  , a(fLinearAlpha)
{
}

inline plColor::plColor(const plColorLinearUB& cc)
{
  *this = cc;
}

inline plColor::plColor(const plColorGammaUB& cc)
{
  *this = cc;
}

inline void plColor::SetRGB(float fLinearRed, float fLinearGreen, float fLinearBlue)
{
  r = fLinearRed;
  g = fLinearGreen;
  b = fLinearBlue;
}

inline void plColor::SetRGBA(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha /* = 1.0f */)
{
  r = fLinearRed;
  g = fLinearGreen;
  b = fLinearBlue;
  a = fLinearAlpha;
}

inline void plColor::SetZero()
{
  *this = ZeroColor();
}

// http://en.wikipedia.org/wiki/Luminance_%28relative%29
PLASMA_FORCE_INLINE float plColor::GetLuminance() const
{
  return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

inline plColor plColor::GetInvertedColor() const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_ASSERT_DEBUG(IsNormalized(), "Cannot invert a color that has values outside the [0; 1] range");

  return plColor(1.0f - r, 1.0f - g, 1.0f - b, 1.0f - a);
}

inline bool plColor::IsNaN() const
{
  if (plMath::IsNaN(r))
    return true;
  if (plMath::IsNaN(g))
    return true;
  if (plMath::IsNaN(b))
    return true;
  if (plMath::IsNaN(a))
    return true;

  return false;
}

inline void plColor::operator+=(const plColor& rhs)
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  r += rhs.r;
  g += rhs.g;
  b += rhs.b;
  a += rhs.a;
}

inline void plColor::operator-=(const plColor& rhs)
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  r -= rhs.r;
  g -= rhs.g;
  b -= rhs.b;
  a -= rhs.a;
}

inline void plColor::operator*=(const plColor& rhs)
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  r *= rhs.r;
  g *= rhs.g;
  b *= rhs.b;
  a *= rhs.a;
}
inline void plColor::operator*=(float f)
{
  r *= f;
  g *= f;
  b *= f;
  a *= f;

  PLASMA_NAN_ASSERT(this);
}

inline bool plColor::IsIdenticalRGB(const plColor& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return r == rhs.r && g == rhs.g && b == rhs.b;
}

inline bool plColor::IsIdenticalRGBA(const plColor& rhs) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
}

inline plColor plColor::WithAlpha(float fAlpha) const
{
  return plColor(r, g, b, fAlpha);
}

inline const plColor operator+(const plColor& c1, const plColor& c2)
{
  PLASMA_NAN_ASSERT(&c1);
  PLASMA_NAN_ASSERT(&c2);

  return plColor(c1.r + c2.r, c1.g + c2.g, c1.b + c2.b, c1.a + c2.a);
}

inline const plColor operator-(const plColor& c1, const plColor& c2)
{
  PLASMA_NAN_ASSERT(&c1);
  PLASMA_NAN_ASSERT(&c2);

  return plColor(c1.r - c2.r, c1.g - c2.g, c1.b - c2.b, c1.a - c2.a);
}

inline const plColor operator*(const plColor& c1, const plColor& c2)
{
  PLASMA_NAN_ASSERT(&c1);
  PLASMA_NAN_ASSERT(&c2);

  return plColor(c1.r * c2.r, c1.g * c2.g, c1.b * c2.b, c1.a * c2.a);
}

inline const plColor operator*(float f, const plColor& c)
{
  PLASMA_NAN_ASSERT(&c);

  return plColor(c.r * f, c.g * f, c.b * f, c.a * f);
}

inline const plColor operator*(const plColor& c, float f)
{
  PLASMA_NAN_ASSERT(&c);

  return plColor(c.r * f, c.g * f, c.b * f, c.a * f);
}

inline const plColor operator*(const plMat4& lhs, const plColor& rhs)
{
  plColor r = rhs;
  r *= lhs;
  return r;
}

inline const plColor operator/(const plColor& c, float f)
{
  PLASMA_NAN_ASSERT(&c);

  float f_inv = 1.0f / f;
  return plColor(c.r * f_inv, c.g * f_inv, c.b * f_inv, c.a * f_inv);
}

PLASMA_ALWAYS_INLINE bool operator==(const plColor& c1, const plColor& c2)
{
  return c1.IsIdenticalRGBA(c2);
}

PLASMA_ALWAYS_INLINE bool operator!=(const plColor& c1, const plColor& c2)
{
  return !c1.IsIdenticalRGBA(c2);
}

PLASMA_FORCE_INLINE bool operator<(const plColor& c1, const plColor& c2)
{
  if (c1.r < c2.r)
    return true;
  if (c1.r > c2.r)
    return false;
  if (c1.g < c2.g)
    return true;
  if (c1.g > c2.g)
    return false;
  if (c1.b < c2.b)
    return true;
  if (c1.b > c2.b)
    return false;

  return (c1.a < c2.a);
}

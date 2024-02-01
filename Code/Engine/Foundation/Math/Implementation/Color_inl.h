#pragma once

inline plColor::plColor()
{
#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const float TypeNaN = plMath::NaN<float>();
  r = TypeNaN;
  g = TypeNaN;
  b = TypeNaN;
  a = TypeNaN;
#endif
}

PL_FORCE_INLINE constexpr plColor::plColor(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha /* = 1.0f */)
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

inline plColor plColor::MakeFromKelvin(plUInt32 uiKelvin)
{
  plColor finalColor;
  float kelvin = plMath::Clamp(uiKelvin, 1000u, 40000u) / 1000.0f;
  float kelvin2 = kelvin * kelvin;

  // Red
  finalColor.r = kelvin < 6.570f ? 1.0f : plMath::Clamp((1.35651f + 0.216422f * kelvin + 0.000633715f * kelvin2) / (-3.24223f + 0.918711f * kelvin), 0.0f, 1.0f);
  // Green
  finalColor.g = kelvin < 6.570f ? plMath::Clamp((-399.809f + 414.271f * kelvin + 111.543f * kelvin2) / (2779.24f + 164.143f * kelvin + 84.7356f * kelvin2), 0.0f, 1.0f) : plMath::Clamp((1370.38f + 734.616f * kelvin + 0.689955f * kelvin2) / (-4625.69f + 1699.87f * kelvin), 0.0f, 1.0f);
  // Blue
  finalColor.b = kelvin > 6.570f ? 1.0f : plMath::Clamp((348.963f - 523.53f * kelvin + 183.62f * kelvin2) / (2848.82f - 214.52f * kelvin + 78.8614f * kelvin2), 0.0f, 1.0f);

  return finalColor;
}

// http://en.wikipedia.org/wiki/Luminance_%28relative%29
PL_FORCE_INLINE float plColor::GetLuminance() const
{
  return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

inline plColor plColor::GetInvertedColor() const
{
  PL_NAN_ASSERT(this);
  PL_ASSERT_DEBUG(IsNormalized(), "Cannot invert a color that has values outside the [0; 1] range");

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
  PL_NAN_ASSERT(this);
  PL_NAN_ASSERT(&rhs);

  r += rhs.r;
  g += rhs.g;
  b += rhs.b;
  a += rhs.a;
}

inline void plColor::operator-=(const plColor& rhs)
{
  PL_NAN_ASSERT(this);
  PL_NAN_ASSERT(&rhs);

  r -= rhs.r;
  g -= rhs.g;
  b -= rhs.b;
  a -= rhs.a;
}

inline void plColor::operator*=(const plColor& rhs)
{
  PL_NAN_ASSERT(this);
  PL_NAN_ASSERT(&rhs);

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

  PL_NAN_ASSERT(this);
}

inline bool plColor::IsIdenticalRGB(const plColor& rhs) const
{
  PL_NAN_ASSERT(this);
  PL_NAN_ASSERT(&rhs);

  return r == rhs.r && g == rhs.g && b == rhs.b;
}

inline bool plColor::IsIdenticalRGBA(const plColor& rhs) const
{
  PL_NAN_ASSERT(this);
  PL_NAN_ASSERT(&rhs);

  return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
}

inline plColor plColor::WithAlpha(float fAlpha) const
{
  return plColor(r, g, b, fAlpha);
}

inline const plColor operator+(const plColor& c1, const plColor& c2)
{
  PL_NAN_ASSERT(&c1);
  PL_NAN_ASSERT(&c2);

  return plColor(c1.r + c2.r, c1.g + c2.g, c1.b + c2.b, c1.a + c2.a);
}

inline const plColor operator-(const plColor& c1, const plColor& c2)
{
  PL_NAN_ASSERT(&c1);
  PL_NAN_ASSERT(&c2);

  return plColor(c1.r - c2.r, c1.g - c2.g, c1.b - c2.b, c1.a - c2.a);
}

inline const plColor operator*(const plColor& c1, const plColor& c2)
{
  PL_NAN_ASSERT(&c1);
  PL_NAN_ASSERT(&c2);

  return plColor(c1.r * c2.r, c1.g * c2.g, c1.b * c2.b, c1.a * c2.a);
}

inline const plColor operator*(float f, const plColor& c)
{
  PL_NAN_ASSERT(&c);

  return plColor(c.r * f, c.g * f, c.b * f, c.a * f);
}

inline const plColor operator*(const plColor& c, float f)
{
  PL_NAN_ASSERT(&c);

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
  PL_NAN_ASSERT(&c);

  float f_inv = 1.0f / f;
  return plColor(c.r * f_inv, c.g * f_inv, c.b * f_inv, c.a * f_inv);
}

PL_ALWAYS_INLINE bool operator==(const plColor& c1, const plColor& c2)
{
  return c1.IsIdenticalRGBA(c2);
}

PL_ALWAYS_INLINE bool operator!=(const plColor& c1, const plColor& c2)
{
  return !c1.IsIdenticalRGBA(c2);
}

PL_FORCE_INLINE bool operator<(const plColor& c1, const plColor& c2)
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

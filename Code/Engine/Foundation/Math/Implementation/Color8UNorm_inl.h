#pragma once

PLASMA_ALWAYS_INLINE plColorBaseUB::plColorBaseUB(plUInt8 r, plUInt8 g, plUInt8 b, plUInt8 a /* = 255*/)
{
  this->r = r;
  this->g = g;
  this->b = b;
  this->a = a;
}

PLASMA_ALWAYS_INLINE plColorLinearUB::plColorLinearUB(plUInt8 r, plUInt8 g, plUInt8 b, plUInt8 a /* = 255*/)
  : plColorBaseUB(r, g, b, a)
{
}

inline plColorLinearUB::plColorLinearUB(const plColor& color)
{
  *this = color;
}

inline void plColorLinearUB::operator=(const plColor& color)
{
  r = plMath::ColorFloatToByte(color.r);
  g = plMath::ColorFloatToByte(color.g);
  b = plMath::ColorFloatToByte(color.b);
  a = plMath::ColorFloatToByte(color.a);
}

inline plColor plColorLinearUB::ToLinearFloat() const
{
  return plColor(plMath::ColorByteToFloat(r), plMath::ColorByteToFloat(g), plMath::ColorByteToFloat(b), plMath::ColorByteToFloat(a));
}

// *****************

PLASMA_ALWAYS_INLINE plColorGammaUB::plColorGammaUB(plUInt8 r, plUInt8 g, plUInt8 b, plUInt8 a)
  : plColorBaseUB(r, g, b, a)
{
}

inline plColorGammaUB::plColorGammaUB(const plColor& color)
{
  *this = color;
}

inline void plColorGammaUB::operator=(const plColor& color)
{
  const plVec3 gamma = plColor::LinearToGamma(plVec3(color.r, color.g, color.b));

  r = plMath::ColorFloatToByte(gamma.x);
  g = plMath::ColorFloatToByte(gamma.y);
  b = plMath::ColorFloatToByte(gamma.z);
  a = plMath::ColorFloatToByte(color.a);
}

inline plColor plColorGammaUB::ToLinearFloat() const
{
  plVec3 gamma;
  gamma.x = plMath::ColorByteToFloat(r);
  gamma.y = plMath::ColorByteToFloat(g);
  gamma.z = plMath::ColorByteToFloat(b);

  const plVec3 linear = plColor::GammaToLinear(gamma);

  return plColor(linear.x, linear.y, linear.z, plMath::ColorByteToFloat(a));
}

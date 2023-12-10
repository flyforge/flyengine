#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat4.h>

// ****** plColor ******

void plColor::operator=(const plColorLinearUB& cc)
{
  *this = cc.ToLinearFloat();
}

void plColor::operator=(const plColorGammaUB& cc)
{
  *this = cc.ToLinearFloat();
}

bool plColor::IsNormalized() const
{
  PLASMA_NAN_ASSERT(this);

  return r <= 1.0f && g <= 1.0f && b <= 1.0f && a <= 1.0f && r >= 0.0f && g >= 0.0f && b >= 0.0f && a >= 0.0f;
}


float plColor::CalcAverageRGB() const
{
  return (1.0f / 3.0f) * (r + g + b);
}

// http://en.literateprograms.org/RGB_to_HSV_color_space_conversion_%28C%29
void plColor::GetHSV(float& out_fHue, float& out_fSat, float& out_fValue) const
{
  // The formula below assumes values in gamma space
  const float r2 = LinearToGamma(r);
  const float g2 = LinearToGamma(g);
  const float b2 = LinearToGamma(b);

  out_fValue = plMath::Max(r2, g2, b2); // Value

  if (out_fValue < plMath::SmallEpsilon<float>())
  {
    out_fHue = 0.0f;
    out_fSat = 0.0f;
    out_fValue = 0.0f;
    return;
  }

  const float invV = 1.0f / out_fValue;
  float norm_r = r2 * invV;
  float norm_g = g2 * invV;
  float norm_b = b2 * invV;
  float rgb_min = plMath::Min(norm_r, norm_g, norm_b);
  float rgb_max = plMath::Max(norm_r, norm_g, norm_b);

  out_fSat = rgb_max - rgb_min; // Saturation

  if (out_fSat == 0)
  {
    out_fHue = 0;
    return;
  }

  // Normalize saturation
  const float rgb_delta_inv = 1.0f / (rgb_max - rgb_min);
  norm_r = (norm_r - rgb_min) * rgb_delta_inv;
  norm_g = (norm_g - rgb_min) * rgb_delta_inv;
  norm_b = (norm_b - rgb_min) * rgb_delta_inv;
  rgb_max = plMath::Max(norm_r, norm_g, norm_b);

  // hue
  if (rgb_max == norm_r)
  {
    out_fHue = 60.0f * (norm_g - norm_b);

    if (out_fHue < 0.0f)
      out_fHue += 360.0f;
  }
  else if (rgb_max == norm_g)
    out_fHue = 120.0f + 60.0f * (norm_b - norm_r);
  else
    out_fHue = 240.0f + 60.0f * (norm_r - norm_g);
}

// http://www.rapidtables.com/convert/color/hsv-to-rgb.htm
plColor plColor::MakeHSV(float fHue, float fSat, float fVal)
{
  PLASMA_ASSERT_DEBUG(fHue <= 360 && fHue >= 0, "HSV 'hue' is in invalid range.");
  PLASMA_ASSERT_DEBUG(fSat <= 1 && fVal >= 0, "HSV 'saturation' is in invalid range.");
  PLASMA_ASSERT_DEBUG(fVal >= 0, "HSV 'value' is in invalid range.");

  float c = fSat * fVal;
  float x = c * (1.0f - plMath::Abs(plMath::Mod(fHue / 60.0f, 2) - 1.0f));
  float m = fVal - c;

  plColor res;
  res.a = 1.0f;

  if (fHue < 60)
  {
    res.r = c + m;
    res.g = x + m;
    res.b = 0 + m;
  }
  else if (fHue < 120)
  {
    res.r = x + m;
    res.g = c + m;
    res.b = 0 + m;
  }
  else if (fHue < 180)
  {
    res.r = 0 + m;
    res.g = c + m;
    res.b = x + m;
  }
  else if (fHue < 240)
  {
    res.r = 0 + m;
    res.g = x + m;
    res.b = c + m;
  }
  else if (fHue < 300)
  {
    res.r = x + m;
    res.g = 0 + m;
    res.b = c + m;
  }
  else
  {
    res.r = c + m;
    res.g = 0 + m;
    res.b = x + m;
  }

  // The formula above produces value in gamma space
  res.r = GammaToLinear(res.r);
  res.g = GammaToLinear(res.g);
  res.b = GammaToLinear(res.b);

  return res;
}

float plColor::GetSaturation() const
{
  float hue, sat, val;
  GetHSV(hue, sat, val);

  return sat;
}

bool plColor::IsValid() const
{
  if (!plMath::IsFinite(r))
    return false;
  if (!plMath::IsFinite(g))
    return false;
  if (!plMath::IsFinite(b))
    return false;
  if (!plMath::IsFinite(a))
    return false;

  return true;
}

bool plColor::IsEqualRGB(const plColor& rhs, float fEpsilon) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return (plMath::IsEqual(r, rhs.r, fEpsilon) && plMath::IsEqual(g, rhs.g, fEpsilon) && plMath::IsEqual(b, rhs.b, fEpsilon));
}

bool plColor::IsEqualRGBA(const plColor& rhs, float fEpsilon) const
{
  PLASMA_NAN_ASSERT(this);
  PLASMA_NAN_ASSERT(&rhs);

  return (plMath::IsEqual(r, rhs.r, fEpsilon) && plMath::IsEqual(g, rhs.g, fEpsilon) && plMath::IsEqual(b, rhs.b, fEpsilon) &&
          plMath::IsEqual(a, rhs.a, fEpsilon));
}

void plColor::operator/=(float f)
{
  float f_inv = 1.0f / f;
  r *= f_inv;
  g *= f_inv;
  b *= f_inv;
  a *= f_inv;

  PLASMA_NAN_ASSERT(this);
}

void plColor::operator*=(const plMat4& rhs)
{
  plVec3 v(r, g, b);
  v = rhs.TransformPosition(v);

  r = v.x;
  g = v.y;
  b = v.z;
}


void plColor::ScaleRGB(float fFactor)
{
  r *= fFactor;
  g *= fFactor;
  b *= fFactor;
}

void plColor::ScaleRGBA(float fFactor)
{
  r *= fFactor;
  g *= fFactor;
  b *= fFactor;
  a *= fFactor;
}

float plColor::ComputeHdrMultiplier() const
{
  return plMath::Max(1.0f, r, g, b);
}

float plColor::ComputeHdrExposureValue() const
{
  return plMath::Log2(ComputeHdrMultiplier());
}

void plColor::ApplyHdrExposureValue(float fEv)
{
  const float factor = plMath::Pow2(fEv);
  r *= factor;
  g *= factor;
  b *= factor;
}


void plColor::NormalizeToLdrRange()
{
  ScaleRGB(1.0f / ComputeHdrMultiplier());
}

plColor plColor::GetDarker(float fFactor /*= 2.0f*/) const
{
  float h, s, v;
  GetHSV(h, s, v);

  return plColor::MakeHSV(h, s, v / fFactor);
}

plColor plColor::GetComplementaryColor() const
{
  float hue, sat, val;
  GetHSV(hue, sat, val);

  plColor Shifted = plColor::MakeHSV(plMath::Mod(hue + 180.0f, 360.0f), sat, val);
  Shifted.a = a;

  return Shifted;
}

const plVec4 plColor::GetAsVec4() const
{
  return plVec4(r, g, b, a);
}

float plColor::GammaToLinear(float fGamma)
{
  return fGamma <= 0.04045f ? (fGamma / 12.92f) : (plMath::Pow((fGamma + 0.055f) / 1.055f, 2.4f));
}

float plColor::LinearToGamma(float fLinear)
{
  // assuming we have linear color (not CIE xyY or CIE XYZ)
  return fLinear <= 0.0031308f ? (12.92f * fLinear) : (1.055f * plMath::Pow(fLinear, 1.0f / 2.4f) - 0.055f);
}

plVec3 plColor::GammaToLinear(const plVec3& vGamma)
{
  return plVec3(GammaToLinear(vGamma.x), GammaToLinear(vGamma.y), GammaToLinear(vGamma.z));
}

plVec3 plColor::LinearToGamma(const plVec3& vLinear)
{
  // assuming we have linear color (not CIE xyY or CIE XYZ)
  return plVec3(LinearToGamma(vLinear.x), LinearToGamma(vLinear.y), LinearToGamma(vLinear.z));
}

const plColor plColor::AliceBlue(plColorGammaUB(0xF0, 0xF8, 0xFF));
const plColor plColor::AntiqueWhite(plColorGammaUB(0xFA, 0xEB, 0xD7));
const plColor plColor::Aqua(plColorGammaUB(0x00, 0xFF, 0xFF));
const plColor plColor::Aquamarine(plColorGammaUB(0x7F, 0xFF, 0xD4));
const plColor plColor::Azure(plColorGammaUB(0xF0, 0xFF, 0xFF));
const plColor plColor::Beige(plColorGammaUB(0xF5, 0xF5, 0xDC));
const plColor plColor::Bisque(plColorGammaUB(0xFF, 0xE4, 0xC4));
const plColor plColor::Black(plColorGammaUB(0x00, 0x00, 0x00));
const plColor plColor::BlanchedAlmond(plColorGammaUB(0xFF, 0xEB, 0xCD));
const plColor plColor::Blue(plColorGammaUB(0x00, 0x00, 0xFF));
const plColor plColor::BlueViolet(plColorGammaUB(0x8A, 0x2B, 0xE2));
const plColor plColor::Brown(plColorGammaUB(0xA5, 0x2A, 0x2A));
const plColor plColor::BurlyWood(plColorGammaUB(0xDE, 0xB8, 0x87));
const plColor plColor::CadetBlue(plColorGammaUB(0x5F, 0x9E, 0xA0));
const plColor plColor::Chartreuse(plColorGammaUB(0x7F, 0xFF, 0x00));
const plColor plColor::Chocolate(plColorGammaUB(0xD2, 0x69, 0x1E));
const plColor plColor::Coral(plColorGammaUB(0xFF, 0x7F, 0x50));
const plColor plColor::CornflowerBlue(plColorGammaUB(0x64, 0x95, 0xED)); // The Original!
const plColor plColor::Cornsilk(plColorGammaUB(0xFF, 0xF8, 0xDC));
const plColor plColor::Crimson(plColorGammaUB(0xDC, 0x14, 0x3C));
const plColor plColor::Cyan(plColorGammaUB(0x00, 0xFF, 0xFF));
const plColor plColor::DarkBlue(plColorGammaUB(0x00, 0x00, 0x8B));
const plColor plColor::DarkCyan(plColorGammaUB(0x00, 0x8B, 0x8B));
const plColor plColor::DarkGoldenRod(plColorGammaUB(0xB8, 0x86, 0x0B));
const plColor plColor::DarkGray(plColorGammaUB(0xA9, 0xA9, 0xA9));
const plColor plColor::DarkGrey(plColorGammaUB(0xA9, 0xA9, 0xA9));
const plColor plColor::DarkGreen(plColorGammaUB(0x00, 0x64, 0x00));
const plColor plColor::DarkKhaki(plColorGammaUB(0xBD, 0xB7, 0x6B));
const plColor plColor::DarkMagenta(plColorGammaUB(0x8B, 0x00, 0x8B));
const plColor plColor::DarkOliveGreen(plColorGammaUB(0x55, 0x6B, 0x2F));
const plColor plColor::DarkOrange(plColorGammaUB(0xFF, 0x8C, 0x00));
const plColor plColor::DarkOrchid(plColorGammaUB(0x99, 0x32, 0xCC));
const plColor plColor::DarkRed(plColorGammaUB(0x8B, 0x00, 0x00));
const plColor plColor::DarkSalmon(plColorGammaUB(0xE9, 0x96, 0x7A));
const plColor plColor::DarkSeaGreen(plColorGammaUB(0x8F, 0xBC, 0x8F));
const plColor plColor::DarkSlateBlue(plColorGammaUB(0x48, 0x3D, 0x8B));
const plColor plColor::DarkSlateGray(plColorGammaUB(0x2F, 0x4F, 0x4F));
const plColor plColor::DarkSlateGrey(plColorGammaUB(0x2F, 0x4F, 0x4F));
const plColor plColor::DarkTurquoise(plColorGammaUB(0x00, 0xCE, 0xD1));
const plColor plColor::DarkViolet(plColorGammaUB(0x94, 0x00, 0xD3));
const plColor plColor::DeepPink(plColorGammaUB(0xFF, 0x14, 0x93));
const plColor plColor::DeepSkyBlue(plColorGammaUB(0x00, 0xBF, 0xFF));
const plColor plColor::DimGray(plColorGammaUB(0x69, 0x69, 0x69));
const plColor plColor::DimGrey(plColorGammaUB(0x69, 0x69, 0x69));
const plColor plColor::DodgerBlue(plColorGammaUB(0x1E, 0x90, 0xFF));
const plColor plColor::FireBrick(plColorGammaUB(0xB2, 0x22, 0x22));
const plColor plColor::FloralWhite(plColorGammaUB(0xFF, 0xFA, 0xF0));
const plColor plColor::ForestGreen(plColorGammaUB(0x22, 0x8B, 0x22));
const plColor plColor::Fuchsia(plColorGammaUB(0xFF, 0x00, 0xFF));
const plColor plColor::Gainsboro(plColorGammaUB(0xDC, 0xDC, 0xDC));
const plColor plColor::GhostWhite(plColorGammaUB(0xF8, 0xF8, 0xFF));
const plColor plColor::Gold(plColorGammaUB(0xFF, 0xD7, 0x00));
const plColor plColor::GoldenRod(plColorGammaUB(0xDA, 0xA5, 0x20));
const plColor plColor::Gray(plColorGammaUB(0x80, 0x80, 0x80));
const plColor plColor::Grey(plColorGammaUB(0x80, 0x80, 0x80));
const plColor plColor::Green(plColorGammaUB(0x00, 0x80, 0x00));
const plColor plColor::GreenYellow(plColorGammaUB(0xAD, 0xFF, 0x2F));
const plColor plColor::HoneyDew(plColorGammaUB(0xF0, 0xFF, 0xF0));
const plColor plColor::HotPink(plColorGammaUB(0xFF, 0x69, 0xB4));
const plColor plColor::IndianRed(plColorGammaUB(0xCD, 0x5C, 0x5C));
const plColor plColor::Indigo(plColorGammaUB(0x4B, 0x00, 0x82));
const plColor plColor::Ivory(plColorGammaUB(0xFF, 0xFF, 0xF0));
const plColor plColor::Khaki(plColorGammaUB(0xF0, 0xE6, 0x8C));
const plColor plColor::Lavender(plColorGammaUB(0xE6, 0xE6, 0xFA));
const plColor plColor::LavenderBlush(plColorGammaUB(0xFF, 0xF0, 0xF5));
const plColor plColor::LawnGreen(plColorGammaUB(0x7C, 0xFC, 0x00));
const plColor plColor::LemonChiffon(plColorGammaUB(0xFF, 0xFA, 0xCD));
const plColor plColor::LightBlue(plColorGammaUB(0xAD, 0xD8, 0xE6));
const plColor plColor::LightCoral(plColorGammaUB(0xF0, 0x80, 0x80));
const plColor plColor::LightCyan(plColorGammaUB(0xE0, 0xFF, 0xFF));
const plColor plColor::LightGoldenRodYellow(plColorGammaUB(0xFA, 0xFA, 0xD2));
const plColor plColor::LightGray(plColorGammaUB(0xD3, 0xD3, 0xD3));
const plColor plColor::LightGrey(plColorGammaUB(0xD3, 0xD3, 0xD3));
const plColor plColor::LightGreen(plColorGammaUB(0x90, 0xEE, 0x90));
const plColor plColor::LightPink(plColorGammaUB(0xFF, 0xB6, 0xC1));
const plColor plColor::LightSalmon(plColorGammaUB(0xFF, 0xA0, 0x7A));
const plColor plColor::LightSeaGreen(plColorGammaUB(0x20, 0xB2, 0xAA));
const plColor plColor::LightSkyBlue(plColorGammaUB(0x87, 0xCE, 0xFA));
const plColor plColor::LightSlateGray(plColorGammaUB(0x77, 0x88, 0x99));
const plColor plColor::LightSlateGrey(plColorGammaUB(0x77, 0x88, 0x99));
const plColor plColor::LightSteelBlue(plColorGammaUB(0xB0, 0xC4, 0xDE));
const plColor plColor::LightYellow(plColorGammaUB(0xFF, 0xFF, 0xE0));
const plColor plColor::Lime(plColorGammaUB(0x00, 0xFF, 0x00));
const plColor plColor::LimeGreen(plColorGammaUB(0x32, 0xCD, 0x32));
const plColor plColor::Linen(plColorGammaUB(0xFA, 0xF0, 0xE6));
const plColor plColor::Magenta(plColorGammaUB(0xFF, 0x00, 0xFF));
const plColor plColor::Maroon(plColorGammaUB(0x80, 0x00, 0x00));
const plColor plColor::MediumAquaMarine(plColorGammaUB(0x66, 0xCD, 0xAA));
const plColor plColor::MediumBlue(plColorGammaUB(0x00, 0x00, 0xCD));
const plColor plColor::MediumOrchid(plColorGammaUB(0xBA, 0x55, 0xD3));
const plColor plColor::MediumPurple(plColorGammaUB(0x93, 0x70, 0xDB));
const plColor plColor::MediumSeaGreen(plColorGammaUB(0x3C, 0xB3, 0x71));
const plColor plColor::MediumSlateBlue(plColorGammaUB(0x7B, 0x68, 0xEE));
const plColor plColor::MediumSpringGreen(plColorGammaUB(0x00, 0xFA, 0x9A));
const plColor plColor::MediumTurquoise(plColorGammaUB(0x48, 0xD1, 0xCC));
const plColor plColor::MediumVioletRed(plColorGammaUB(0xC7, 0x15, 0x85));
const plColor plColor::MidnightBlue(plColorGammaUB(0x19, 0x19, 0x70));
const plColor plColor::MintCream(plColorGammaUB(0xF5, 0xFF, 0xFA));
const plColor plColor::MistyRose(plColorGammaUB(0xFF, 0xE4, 0xE1));
const plColor plColor::Moccasin(plColorGammaUB(0xFF, 0xE4, 0xB5));
const plColor plColor::NavajoWhite(plColorGammaUB(0xFF, 0xDE, 0xAD));
const plColor plColor::Navy(plColorGammaUB(0x00, 0x00, 0x80));
const plColor plColor::OldLace(plColorGammaUB(0xFD, 0xF5, 0xE6));
const plColor plColor::Olive(plColorGammaUB(0x80, 0x80, 0x00));
const plColor plColor::OliveDrab(plColorGammaUB(0x6B, 0x8E, 0x23));
const plColor plColor::Orange(plColorGammaUB(0xFF, 0xA5, 0x00));
const plColor plColor::OrangeRed(plColorGammaUB(0xFF, 0x45, 0x00));
const plColor plColor::Orchid(plColorGammaUB(0xDA, 0x70, 0xD6));
const plColor plColor::PaleGoldenRod(plColorGammaUB(0xEE, 0xE8, 0xAA));
const plColor plColor::PaleGreen(plColorGammaUB(0x98, 0xFB, 0x98));
const plColor plColor::PaleTurquoise(plColorGammaUB(0xAF, 0xEE, 0xEE));
const plColor plColor::PaleVioletRed(plColorGammaUB(0xDB, 0x70, 0x93));
const plColor plColor::PapayaWhip(plColorGammaUB(0xFF, 0xEF, 0xD5));
const plColor plColor::PeachPuff(plColorGammaUB(0xFF, 0xDA, 0xB9));
const plColor plColor::Peru(plColorGammaUB(0xCD, 0x85, 0x3F));
const plColor plColor::Pink(plColorGammaUB(0xFF, 0xC0, 0xCB));
const plColor plColor::Plum(plColorGammaUB(0xDD, 0xA0, 0xDD));
const plColor plColor::PowderBlue(plColorGammaUB(0xB0, 0xE0, 0xE6));
const plColor plColor::Purple(plColorGammaUB(0x80, 0x00, 0x80));
const plColor plColor::RebeccaPurple(plColorGammaUB(0x66, 0x33, 0x99));
const plColor plColor::Red(plColorGammaUB(0xFF, 0x00, 0x00));
const plColor plColor::RosyBrown(plColorGammaUB(0xBC, 0x8F, 0x8F));
const plColor plColor::RoyalBlue(plColorGammaUB(0x41, 0x69, 0xE1));
const plColor plColor::SaddleBrown(plColorGammaUB(0x8B, 0x45, 0x13));
const plColor plColor::Salmon(plColorGammaUB(0xFA, 0x80, 0x72));
const plColor plColor::SandyBrown(plColorGammaUB(0xF4, 0xA4, 0x60));
const plColor plColor::SeaGreen(plColorGammaUB(0x2E, 0x8B, 0x57));
const plColor plColor::SeaShell(plColorGammaUB(0xFF, 0xF5, 0xEE));
const plColor plColor::Sienna(plColorGammaUB(0xA0, 0x52, 0x2D));
const plColor plColor::Silver(plColorGammaUB(0xC0, 0xC0, 0xC0));
const plColor plColor::SkyBlue(plColorGammaUB(0x87, 0xCE, 0xEB));
const plColor plColor::SlateBlue(plColorGammaUB(0x6A, 0x5A, 0xCD));
const plColor plColor::SlateGray(plColorGammaUB(0x70, 0x80, 0x90));
const plColor plColor::SlateGrey(plColorGammaUB(0x70, 0x80, 0x90));
const plColor plColor::Snow(plColorGammaUB(0xFF, 0xFA, 0xFA));
const plColor plColor::SpringGreen(plColorGammaUB(0x00, 0xFF, 0x7F));
const plColor plColor::SteelBlue(plColorGammaUB(0x46, 0x82, 0xB4));
const plColor plColor::Tan(plColorGammaUB(0xD2, 0xB4, 0x8C));
const plColor plColor::Teal(plColorGammaUB(0x00, 0x80, 0x80));
const plColor plColor::Thistle(plColorGammaUB(0xD8, 0xBF, 0xD8));
const plColor plColor::Tomato(plColorGammaUB(0xFF, 0x63, 0x47));
const plColor plColor::Turquoise(plColorGammaUB(0x40, 0xE0, 0xD0));
const plColor plColor::Violet(plColorGammaUB(0xEE, 0x82, 0xEE));
const plColor plColor::Wheat(plColorGammaUB(0xF5, 0xDE, 0xB3));
const plColor plColor::White(plColorGammaUB(0xFF, 0xFF, 0xFF));
const plColor plColor::WhiteSmoke(plColorGammaUB(0xF5, 0xF5, 0xF5));
const plColor plColor::Yellow(plColorGammaUB(0xFF, 0xFF, 0x00));
const plColor plColor::YellowGreen(plColorGammaUB(0x9A, 0xCD, 0x32));

plColor plColor::MakeNaN()
{
  return plColor(plMath::NaN<float>(), plMath::NaN<float>(), plMath::NaN<float>(), plMath::NaN<float>());
}

plColor plColor::MakeZero()
{
  return plColor(0.0f, 0.0f, 0.0f, 0.0f);
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Color);

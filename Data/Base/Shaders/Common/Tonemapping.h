#pragma once

#include "Common.h"

//==========================================================================================
// LINEAR
//==========================================================================================
float3 ToneMapping_Linear(float3 color)
{
  return color;
}

//==========================================================================================
// UNCHARTED 2
//==========================================================================================
float3 ToneMapping_Uncharted2(float3 x)
{
  float A = 0.15;
  float B = 0.50;
  float C = 0.10;
  float D = 0.20;
  float E = 0.02;
  float F = 0.30;

  return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

//==========================================================================================
// REINHARD
//==========================================================================================
float3 ToneMapping_Reinhard(float3 hdr, float k = 1.0f)
{
  return hdr / (hdr + k);
}

float3 ToneMapping_Reinhard_Inverse(float3 sdr, float k = 1.0f)
{
  return k * sdr / (k - sdr);
}

//==========================================================================================
// ACES
//==========================================================================================

float3 ACESApproximate(float3 input)
{
    input *= 0.6f;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((input*(a*input+b))/(input*(c*input+d)+e), 0.0f, 1.0f);
}


float3 ToneMapping_ACES(float3 color)
{
  const float WhitePoint = 6.0;

  return LinearToSrgb(ACESApproximate(color));
}

//==========================================================================================
// AMD
//==========================================================================================

// General tonemapping operator, build 'b' term.
float ColToneB(float hdrMax, float contrast, float shoulder, float midIn, float midOut)
{
  return -((-pow(midIn, contrast) + (midOut * (pow(hdrMax, contrast * shoulder) * pow(midIn, contrast) -
                                                pow(hdrMax, contrast) * pow(midIn, contrast * shoulder) * midOut)) /
                                      (pow(hdrMax, contrast * shoulder) * midOut - pow(midIn, contrast * shoulder) * midOut)) /
           (pow(midIn, contrast * shoulder) * midOut));
}

// General tonemapping operator, build 'c' term.
float ColToneC(float hdrMax, float contrast, float shoulder, float midIn, float midOut)
{
  return (pow(hdrMax, contrast * shoulder) * pow(midIn, contrast) - pow(hdrMax, contrast) * pow(midIn, contrast * shoulder) * midOut) /
         (pow(hdrMax, contrast * shoulder) * midOut - pow(midIn, contrast * shoulder) * midOut);
}

// General tonemapping operator, p := {contrast,shoulder,b,c}.
float ColTone(float x, float4 p)
{
  float z = pow(x, p.r);
  return z / (pow(z, p.g) * p.b + p.a);
}

float3 ToneMapping_AMD(float3 color, float hdrMax, float contrast, float shoulder, float midIn, float midOut)
{
  float b = ColToneB(hdrMax, contrast, shoulder, midIn, midOut);
  float c = ColToneC(hdrMax, contrast, shoulder, midIn, midOut);

  float peak = max(color.r, max(color.g, color.b));
  peak = max(FLT_MIN, peak);

  float3 ratio = color / peak;
  peak = ColTone(peak, float4(contrast, shoulder, b, c));
  // then process ratio

  // probably want send these pre-computed (so send over saturation/crossSaturation as a constant)
  float crosstalk = 4.0;                   // controls amount of channel crosstalk
  float saturation = contrast;             // full tonal range saturation control
  float crossSaturation = contrast * 16.0; // crosstalk saturation

  float white = 1.0;

  // wrap crosstalk in transform
  float ratio_temp = saturation / crossSaturation;
  float pow_temp = pow(peak, crosstalk);
  ratio = pow(abs(ratio), float3(ratio_temp, ratio_temp, ratio_temp));
  ratio = lerp(ratio, float3(white, white, white), float3(pow_temp, pow_temp, pow_temp));
  ratio = pow(abs(ratio), float3(crossSaturation, crossSaturation, crossSaturation));

  // then apply ratio to peak
  color = peak * saturate(ratio);
  return LinearToSrgb(color);
}
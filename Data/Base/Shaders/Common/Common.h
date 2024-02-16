#pragma once

#include "Platforms.h"

// Common sampler states
SamplerState LinearSampler;
SamplerState LinearClampSampler;
SamplerState PointSampler;
SamplerState PointClampSampler;

static const float PI                  = 3.14159265f;
static const float PI2                 = 6.28318530f;
static const float PI4                 = 12.5663706f;
static const float INV_PI              = 0.31830988f;
static const float PI_HALF             = PI * 0.5f;
static const float FLT_MIN             = 0.00000001f;
static const float FLT_MAX_10          = 511.0f;
static const float FLT_MAX_11          = 1023.0f;
static const float FLT_MAX_14          = 8191.0f;
static const float FLT_MAX_16          = 32767.0f;
static const float FLT_MAX_16U         = 65535.0f;

#define sqr(a) ((a) * (a))

float4 RGBA8ToFloat4(uint x)
{
  float4 result;
  result.r = x & 0xFF;
  result.g = (x >> 8)  & 0xFF;
  result.b = (x >> 16) & 0xFF;
  result.a = (x >> 24) & 0xFF;

  return result / 255.0;
}

// for when the input data is already float4
float4 RGBA8ToFloat4(float4 x)
{
  return x;
}

float3 RGB8ToFloat3(uint x)
{
  float3 result;
  result.r = x & 0xFF;
  result.g = (x >> 8)  & 0xFF;
  result.b = (x >> 16) & 0xFF;

  return result / 255.0;
}

float3 RGB10ToFloat3(uint x)
{
  float3 result;
  result.r = x & 0x3FF;
  result.g = (x >> 10) & 0x3FF;
  result.b = (x >> 20) & 0x3FF;

  return result / 1023.0;
}

float2 RG16FToFloat2(uint x)
{
  float2 result;
  result.r = f16tof32(x);
  result.g = f16tof32(x >> 16);

  return result;
}

float4 RGBA16FToFloat4(uint rg, uint ba)
{
  return float4(RG16FToFloat2(rg), RG16FToFloat2(ba));
}

float GetLuminance(float3 color)
{
  return dot(color, float3(0.2126, 0.7152, 0.0722));
}

float3 SrgbToLinear(float3 color)
{
  return select(color < 0.04045, (color / 12.92), pow(color / 1.055 + 0.0521327, 2.4));
}

float3 LinearToSrgb(float3 color)
{
  return select(color < 0.0031308, (color * 12.92), (1.055 * pow(color, 1.0 / 2.4) - 0.055));
}

float3 CubeMapDirection(float3 inDirection)
{
  return float3(inDirection.x, inDirection.z, -inDirection.y);
}

float3 DecodeNormalTexture(float4 normalTex)
{
  float2 xy = normalTex.xy * 2.0f - 1.0f;
  float z = sqrt(max(1.0f - dot(xy, xy), 0.0));
  return float3(xy, z);
}

float InterleavedGradientNoise(float2 screenSpacePosition)
{
  float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
  return frac(magic.z * frac(dot(screenSpacePosition, magic.xy)));
}

float InterleavedGradientNoise(float2 uv, uint frameCount)
{
  const float2 magicFrameScale = float2(47, 17) * 0.695;
  uv += frameCount * magicFrameScale;

  const float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
  return frac(magic.z * frac(dot(uv, magic.xy)));
}

float3 NormalizeAndGetLength(float3 v, out float len)
{
  float squaredLen = dot(v, v);
  float reciprocalLen = rsqrt(squaredLen);
  len = squaredLen * reciprocalLen;
  return v * reciprocalLen;
}

float Square(float x)
{
  return x * x;
}
float2 Square(float2 x)
{
  return x * x;
}
float3 Square(float3 x)
{
  return x * x;
}
float4 Square(float4 x)
{
  return x * x;
}

float AdjustContrast(float value, float contrast)
{
  float a = -contrast;
  float b = contrast + 1;
  return saturate(lerp(a, b, value));
}

float3 Colorize(float3 baseColor, float3 color, float mask)
{
  return baseColor * lerp(1, 2 * color, mask);
}

// https://iquilplles.org/articles/smin/
float SmoothMin(float a, float b, float k = 0.1)
{
  float h = max(k - abs(a - b), 0.0) / k;
  return min(a, b) - h * h * k * (1.0 / 4.0);
}

float SmoothMinCubic(float a, float b, float k = 0.1)
{
  float h = max(k - abs(a - b), 0.0) / k;
  return min(a, b) - h * h * h * k * (1.0 / 6.0);
}

// Find good arbitrary axis vectors to represent U and V axes of a plane,
// given just the normal. 
void FindBestAxisVectors(float3 input, out float3 axis1, out float3 axis2)
{
  const float3 N = abs(input);

  // Find best basis vectors.
  if (N.z > N.x && N.z > N.y)
  {
    axis1 = float3(1, 0, 0);
  }
  else
  {
    axis1 = float3(0, 0, 1);
  }

  axis1 = normalize(axis1 - input * dot(axis1, input));
  axis2 = cross(axis1, input);
}

float saturate_11(float x)
{
  return clamp(x, FLT_MIN, FLT_MAX_11);
}

float2 saturate_11(float2 x)
{
  return clamp(x, FLT_MIN, FLT_MAX_11);
}

float3 saturate_11(float3 x)
{
  return clamp(x, FLT_MIN, FLT_MAX_11);
}

float4 saturate_11(float4 x)
{
  return clamp(x, FLT_MIN, FLT_MAX_11);
}

float saturate_16(float x)
{
  return clamp(x, FLT_MIN, FLT_MAX_16);
}

float2 saturate_16(float2 x)
{
  return clamp(x, FLT_MIN, FLT_MAX_16);
}

float3 saturate_16(float3 x)
{
  return clamp(x, FLT_MIN, FLT_MAX_16);
}

float4 saturate_16(float4 x)
{
  return clamp(x, FLT_MIN, FLT_MAX_16);
}

float2 Ndc2Uv(float2 x)
{
  return x * float2(0.5f, -0.5f) + 0.5f;
}

inline uint flatten2D(uint2 coord, uint2 dim)
{
  return coord.x + coord.y * dim.x;
}
// flattened array index to 2D array index
inline uint2 unflatten2D(uint idx, uint2 dim)
{
  return uint2(idx % dim.x, idx / dim.x);
}

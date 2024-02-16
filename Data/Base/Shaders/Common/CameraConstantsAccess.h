#pragma once

#if PL_ENABLED(PLATFORM_SHADER)

// For stereo support, set this at the beginning of the shader to access the correct values in all camera getters.
static uint s_ActiveCameraEyeIndex = 0;

float4x4 GetCameraToScreenMatrix()  { return CameraToScreenMatrix[s_ActiveCameraEyeIndex]; }
float4x4 GetScreenToCameraMatrix()  { return ScreenToCameraMatrix[s_ActiveCameraEyeIndex]; }
float4x4 GetWorldToCameraMatrix()   { return WorldToCameraMatrix[s_ActiveCameraEyeIndex]; }
float4x4 GetCameraToWorldMatrix()   { return CameraToWorldMatrix[s_ActiveCameraEyeIndex]; }
float4x4 GetWorldToScreenMatrix()   { return WorldToScreenMatrix[s_ActiveCameraEyeIndex]; }
float4x4 GetScreenToWorldMatrix()   { return ScreenToWorldMatrix[s_ActiveCameraEyeIndex]; }

float4x4 GetLastCameraToScreenMatrix()
{
  return LastCameraToScreenMatrix[s_ActiveCameraEyeIndex];
}
float4x4 GetLastScreenToCameraMatrix()
{
  return LastScreenToCameraMatrix[s_ActiveCameraEyeIndex];
}
float4x4 GetLastWorldToCameraMatrix()
{
  return LastWorldToCameraMatrix[s_ActiveCameraEyeIndex];
}
float4x4 GetLastCameraToWorldMatrix()
{
  return LastCameraToWorldMatrix[s_ActiveCameraEyeIndex];
}
float4x4 GetLastWorldToScreenMatrix()
{
  return LastWorldToScreenMatrix[s_ActiveCameraEyeIndex];
}
float4x4 GetLastScreenToWorldMatrix()
{
  return LastScreenToWorldMatrix[s_ActiveCameraEyeIndex];
}

float3 GetCameraPosition()     { return GetCameraToWorldMatrix()._m03_m13_m23; };
float3 GetCameraDirForwards()  { return GetCameraToWorldMatrix()._m02_m12_m22; };
float3 GetCameraDirRight()     { return GetCameraToWorldMatrix()._m00_m10_m20; };
float3 GetCameraDirUp()        { return GetCameraToWorldMatrix()._m01_m11_m21; };

// Computes linear depth from depth buffer depth.
// Note that computations like this are not set in stone as we may want to move to a different way of storing Z
// (for example flipped near/far plane is quite common for better float precision)
//
// Basically removes the w division from z again.
float LinearizeZBufferDepth(float depthFromZBuffer)
{
  return 1.0f / (depthFromZBuffer * GetScreenToCameraMatrix()._43 + GetScreenToCameraMatrix()._44);
}

float3 ScreenCoordToWorldSpace(float2 uv, float depth)
{
  float x = uv.x * 2.0f - 1.0f;
  float y = (1.0 - uv.y) * 2.0f - 1.0f;
  float4 pos_clip = float4(x, y, depth, 1.0f);
  float4 pos_world = mul(GetScreenToWorldMatrix(), pos_clip);
  return pos_world.xyz / pos_world.w;
}

float3 ScreenCoordToViewSpace(float2 uv, float depth)
{
  float x = uv.x * 2.0f - 1.0f;
  float y = (1.0 - uv.y) * 2.0f - 1.0f;
  float4 pos_clip = float4(x, y, depth, 1.0f);
  float4 pos_world = mul(GetScreenToCameraMatrix(), pos_clip);
  return pos_world.xyz / pos_world.w;
}


#else

// C++

#endif

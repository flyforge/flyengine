#pragma once

#include "ParticleSystemConstants.h"
#include "BaseParticleShaderData.h"

struct PL_SHADER_STRUCT plTrailParticleShaderData
{
  INT1(NumPoints);
  FLOAT3(dummy);
};

struct PL_SHADER_STRUCT plTrailParticlePointsData8
{
  FLOAT4(Positions[8]);
};

struct PL_SHADER_STRUCT plTrailParticlePointsData16
{
  FLOAT4(Positions[16]);
};

struct PL_SHADER_STRUCT plTrailParticlePointsData32
{
  FLOAT4(Positions[32]);
};

struct PL_SHADER_STRUCT plTrailParticlePointsData64
{
  FLOAT4(Positions[64]);
};

// this is only defined during shader compilation
#if PL_ENABLED(PLATFORM_SHADER)

StructuredBuffer<plTrailParticleShaderData> particleTrailData;

#if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT8
  StructuredBuffer<plTrailParticlePointsData8> particlePointsData;
#endif

#if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT16
  StructuredBuffer<plTrailParticlePointsData16> particlePointsData;
#endif

#if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT32
  StructuredBuffer<plTrailParticlePointsData32> particlePointsData;
#endif

#if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT64
  StructuredBuffer<plTrailParticlePointsData64> particlePointsData;
#endif

#else // C++

#endif


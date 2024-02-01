#pragma once

#include "BaseParticleShaderData.h"

struct PL_SHADER_STRUCT plTangentQuadParticleShaderData
{
  FLOAT3(Position);
  FLOAT1(dummy1);

  FLOAT3(TangentX);
  FLOAT1(dummy2);

  FLOAT3(TangentZ);
  FLOAT1(dummy3);
};

// this is only defined during shader compilation
#if PL_ENABLED(PLATFORM_SHADER)

StructuredBuffer<plTangentQuadParticleShaderData> particleTangentQuadData;

#else // C++

PL_CHECK_AT_COMPILETIME(sizeof(plTangentQuadParticleShaderData) == 48);

#endif


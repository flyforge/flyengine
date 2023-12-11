#pragma once

#include "BaseParticleShaderData.h"

struct PLASMA_SHADER_STRUCT plTangentQuadParticleShaderData
{
  FLOAT3(Position);
  FLOAT1(dummy1);

  FLOAT3(TangentX);
  FLOAT1(dummy2);

  FLOAT3(TangentZ);
  FLOAT1(dummy3);
};

// this is only defined during shader compilation
#if PLASMA_ENABLED(PLATFORM_SHADER)

StructuredBuffer<plTangentQuadParticleShaderData> particleTangentQuadData;

#else // C++

PLASMA_CHECK_AT_COMPILETIME(sizeof(plTangentQuadParticleShaderData) == 48);

#endif

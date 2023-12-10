#pragma once

#include "ParticleSystemConstants.h"
#include "BaseParticleShaderData.h"

struct PLASMA_SHADER_STRUCT plBillboardQuadParticleShaderData
{
  FLOAT3(Position);
  PACKEDHALF2(RotationOffset, RotationSpeed, RotationOffsetAndSpeed);
};

// this is only defined during shader compilation
#if PLASMA_ENABLED(PLATFORM_SHADER)

StructuredBuffer<plBillboardQuadParticleShaderData> particleBillboardQuadData;

#else // C++

PLASMA_CHECK_AT_COMPILETIME(sizeof(plBillboardQuadParticleShaderData) == 16);

#endif


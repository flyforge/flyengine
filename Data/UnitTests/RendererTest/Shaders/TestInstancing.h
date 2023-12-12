#pragma once

#include "../../../Base/Shaders/Common/Platforms.h"

#include "../../../Base/Shaders/Common/ConstantBufferMacros.h"

struct PLASMA_SHADER_STRUCT plTestShaderData
{
  FLOAT4(InstanceColor);
  TRANSFORM(InstanceTransform);
};

// this is only defined during shader compilation
#if PLASMA_ENABLED(PLATFORM_SHADER)

StructuredBuffer<plTestShaderData> instancingData;

#else // C++

PLASMA_CHECK_AT_COMPILETIME(sizeof(plTestShaderData) == 64);

#endif
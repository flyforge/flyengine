#pragma once

#include "../../../Base/Shaders/Common/Platforms.h"

#include "../../../Base/Shaders/Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(plTestColors, 2)
{
  FLOAT4(VertexColor);
};

CONSTANT_BUFFER(plTestPositions, 3)
{
  FLOAT4(Vertex0);
  FLOAT4(Vertex1);
  FLOAT4(Vertex2);
};

#pragma once

#include <Shaders/Common/Platforms.h>
#include <Shaders/Common/ConstantBufferMacros.h>

CONSTANT_BUFFER(plKrautTreeConstants, 4)
{
  FLOAT3(LeafCenter);
  FLOAT1(LeafShadowOffset);
};

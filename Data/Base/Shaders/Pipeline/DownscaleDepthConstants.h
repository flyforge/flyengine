#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(plDownscaleDepthConstants, 3)
{
  FLOAT2(PixelSize);
  BOOL1(LinearizeDepth);
};
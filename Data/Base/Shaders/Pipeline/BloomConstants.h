#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(plBloomConstants, 3)
{
  FLOAT2(PixelSize);
  FLOAT1(BloomIntensity);
  FLOAT1(BloomThreshold);
};

#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(plFXAAConstants, 3)
{
  FLOAT1(SubPixelAliasingRemovalAmount);
  FLOAT1(EdgeThreshold);
  FLOAT1(EdgeThresholdMin);
};

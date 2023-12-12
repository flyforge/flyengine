#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(plMotionBlurConstants, 3)
{
  UINT1(MotionBlurSamples);
  FLOAT1(MotionBlurStrength);
};

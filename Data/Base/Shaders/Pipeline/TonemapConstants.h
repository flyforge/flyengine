#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(plTonemapConstants, 3)
{
  UINT1(ToneMappingMode);
  FLOAT1(HDRMax);
  FLOAT1(Contrast);
  FLOAT1(Shoulder);
  
  FLOAT1(MidIn);
  FLOAT1(MidOut);
};

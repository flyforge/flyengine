#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

#define SSR_TILESIZE 32

CONSTANT_BUFFER(plSSRConstants, 4)
{
  FLOAT1(RoughnessCutoff);
  FLOAT1(Frame);
};

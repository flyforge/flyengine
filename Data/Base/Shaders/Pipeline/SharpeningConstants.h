#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(plSharpeningConstants, 3)
{
  FLOAT1(Strength);
};

#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(plBlurConstants, 3)
{
  INT1(BlurRadius);
};


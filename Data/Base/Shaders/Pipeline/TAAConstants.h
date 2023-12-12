#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(plTAAConstants, 3)
{
  BOOL1(UpsampleEnabled);
};
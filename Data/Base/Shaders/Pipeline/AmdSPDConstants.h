#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(plAmdSPDConstants, 3)
{
    UINT1(MipCount);
    UINT1(WorkGroupCount);
};
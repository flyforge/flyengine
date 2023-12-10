#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(plSelectionHighlightConstants, 3)
{
  COLOR4F(HighlightColor);
  FLOAT1(OverlayOpacity);
};


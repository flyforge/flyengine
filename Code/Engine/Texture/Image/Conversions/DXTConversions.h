#pragma once

#include <Texture/Image/Image.h>

class plColorLinear16f;

PLASMA_TEXTURE_DLL void plDecompressBlockBC1(const plUInt8* pSource, plColorBaseUB* pTarget, bool bForceFourColorMode);
PLASMA_TEXTURE_DLL void plDecompressBlockBC4(const plUInt8* pSource, plUInt8* pTarget, plUInt32 uiStride, plUInt8 bias);
PLASMA_TEXTURE_DLL void plDecompressBlockBC6(const plUInt8* pSource, plColorLinear16f* pTarget, bool isSigned);
PLASMA_TEXTURE_DLL void plDecompressBlockBC7(const plUInt8* pSource, plColorBaseUB* pTarget);

PLASMA_TEXTURE_DLL void plUnpackPaletteBC4(plUInt32 a0, plUInt32 a1, plUInt32* alphas);

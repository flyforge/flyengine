#pragma once

#include <Texture/Image/Image.h>

class plColorLinear16f;

PL_TEXTURE_DLL void plDecompressBlockBC1(const plUInt8* pSource, plColorBaseUB* pTarget, bool bForceFourColorMode);
PL_TEXTURE_DLL void plDecompressBlockBC4(const plUInt8* pSource, plUInt8* pTarget, plUInt32 uiStride, plUInt8 uiBias);
PL_TEXTURE_DLL void plDecompressBlockBC6(const plUInt8* pSource, plColorLinear16f* pTarget, bool bIsSigned);
PL_TEXTURE_DLL void plDecompressBlockBC7(const plUInt8* pSource, plColorBaseUB* pTarget);

PL_TEXTURE_DLL void plUnpackPaletteBC4(plUInt32 ui0, plUInt32 ui1, plUInt32* pAlphas);

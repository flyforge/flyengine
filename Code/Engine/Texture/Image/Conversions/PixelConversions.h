#pragma once

#include <Texture/Image/Image.h>

PL_TEXTURE_DLL plColorBaseUB plDecompressA4B4G4R4(plUInt16 uiColor);
PL_TEXTURE_DLL plColorBaseUB plDecompressB4G4R4A4(plUInt16 uiColor);
PL_TEXTURE_DLL plColorBaseUB plDecompressB5G6R5(plUInt16 uiColor);
PL_TEXTURE_DLL plColorBaseUB plDecompressB5G5R5X1(plUInt16 uiColor);
PL_TEXTURE_DLL plColorBaseUB plDecompressB5G5R5A1(plUInt16 uiColor);
PL_TEXTURE_DLL plColorBaseUB plDecompressX1B5G5R5(plUInt16 uiColor);
PL_TEXTURE_DLL plColorBaseUB plDecompressA1B5G5R5(plUInt16 uiColor);
PL_TEXTURE_DLL plUInt16 plCompressA4B4G4R4(plColorBaseUB color);
PL_TEXTURE_DLL plUInt16 plCompressB4G4R4A4(plColorBaseUB color);
PL_TEXTURE_DLL plUInt16 plCompressB5G6R5(plColorBaseUB color);
PL_TEXTURE_DLL plUInt16 plCompressB5G5R5X1(plColorBaseUB color);
PL_TEXTURE_DLL plUInt16 plCompressB5G5R5A1(plColorBaseUB color);
PL_TEXTURE_DLL plUInt16 plCompressX1B5G5R5(plColorBaseUB color);
PL_TEXTURE_DLL plUInt16 plCompressA1B5G5R5(plColorBaseUB color);

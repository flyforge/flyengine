#include <Foundation/Platform/PlatformDesc.h>

PL_ENUMERABLE_CLASS_IMPLEMENTATION(plPlatformDesc);

plPlatformDesc g_PlatformDescWin("Windows");

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)

const plPlatformDesc* plPlatformDesc::s_pThisPlatform = &g_PlatformDescWin;

#endif
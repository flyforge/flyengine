#include <Foundation/Platform/PlatformDesc.h>

plPlatformDesc g_PlatformDescOSX("OSX");

#if PL_ENABLED(PL_PLATFORM_OSX)

const plPlatformDesc* plPlatformDesc::s_pThisPlatform = &g_PlatformDescOSX;

#endif
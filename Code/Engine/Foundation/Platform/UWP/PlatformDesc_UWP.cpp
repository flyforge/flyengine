#include <Foundation/Platform/PlatformDesc.h>

plPlatformDesc g_PlatformDescUWP("UWP");

#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)

const plPlatformDesc* plPlatformDesc::s_pThisPlatform = &g_PlatformDescUWP;

#endif
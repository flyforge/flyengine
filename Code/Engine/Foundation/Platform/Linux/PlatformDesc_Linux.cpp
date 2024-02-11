#include <Foundation/Platform/PlatformDesc.h>

plPlatformDesc g_PlatformDescLinux("Linux");

#if PL_ENABLED(PL_PLATFORM_LINUX)

const plPlatformDesc* plPlatformDesc::s_pThisPlatform = &g_PlatformDescLinux;

#endif
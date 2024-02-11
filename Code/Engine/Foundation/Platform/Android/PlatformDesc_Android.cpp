#include <Foundation/Platform/PlatformDesc.h>

plPlatformDesc g_PlatformDescAndroid("Android");

#if PL_ENABLED(PL_PLATFORM_ANDROID)

const plPlatformDesc* plPlatformDesc::s_pThisPlatform = &g_PlatformDescAndroid;

#endif
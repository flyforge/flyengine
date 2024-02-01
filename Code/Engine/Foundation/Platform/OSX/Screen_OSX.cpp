#include <Foundation/FoundationPCH.h>

#include <Foundation/System/PlatformFeatures.h>

#if PL_ENABLED(PL_PLATFORM_OSX)

#  if PL_ENABLED(PL_SUPPORTS_GLFW)
#    include <Foundation/Platform/GLFW/Screen_GLFW.h>
#  else
#    include <Foundation/Platform/NoImpl/Screen_NoImpl.h>
#  endif

#endif



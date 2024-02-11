#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_LINUX)

#  if PL_ENABLED(PL_SUPPORTS_GLFW)
#    include <Foundation/Platform/GLFW/Screen_GLFW.h>
#  else
#    include <Foundation/Platform/NoImpl/Screen_NoImpl.h>
#  endif

#endif



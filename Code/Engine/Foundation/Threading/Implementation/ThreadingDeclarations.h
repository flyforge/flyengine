#pragma once

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/ThreadingDeclarations_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/ThreadingDeclarations_posix.h>
#else
#  error "Unknown Platform."
#endif

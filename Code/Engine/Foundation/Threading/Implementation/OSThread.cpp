#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Thread.h>

// Include inline file
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/OSThread_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/OSThread_posix.h>
#else
#  error "Thread functions are not implemented on current platform"
#endif



PLASMA_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_OSThread);

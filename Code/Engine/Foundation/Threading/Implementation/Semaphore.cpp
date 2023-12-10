#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Semaphore.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/Semaphore_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/Semaphore_posix.h>
#else
#  error "Semaphore is not implemented on current platform"
#endif


PLASMA_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_Semaphore);

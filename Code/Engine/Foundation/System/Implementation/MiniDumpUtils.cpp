#include <Foundation/FoundationPCH.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/System/Implementation/Win/MiniDumpUtils_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX)
#  include <Foundation/System/Implementation/OSX/MiniDumpUtils_OSX.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/System/Implementation/Posix/MiniDumpUtils_posix.h>
#else
#  error "Mini-dump functions are not implemented on current platform"
#endif



PLASMA_STATICLINK_FILE(Foundation, Foundation_System_Implementation_MiniDumpUtils);

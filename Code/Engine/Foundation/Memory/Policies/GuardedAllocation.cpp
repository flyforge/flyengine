#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/Policies/GuardedAllocation.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Memory/Policies/Win/GuardedAllocation_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Memory/Policies/Posix/GuardedAllocation_posix.h>
#else
#  error "plGuardedAllocation is not implemented on current platform"
#endif

PLASMA_STATICLINK_FILE(Foundation, Foundation_Memory_Policies_GuardedAllocation);

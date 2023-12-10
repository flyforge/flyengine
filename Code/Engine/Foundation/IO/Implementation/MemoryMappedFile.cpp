#include <Foundation/FoundationPCH.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/IO/Implementation/Win/MemoryMappedFile_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  include <Foundation/IO/Implementation/Win/MemoryMappedFile_uwp.h>
#elif PLASMA_ENABLED(PLASMA_USE_POSIX_FILE_API)
#  include <Foundation/IO/Implementation/Posix/MemoryMappedFile_posix.h>
#else
#  error "Unknown Platform."
#endif


PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_MemoryMappedFile);

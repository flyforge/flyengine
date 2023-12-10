#include <Foundation/FoundationPCH.h>


#if PLASMA_ENABLED(PLASMA_SUPPORTS_DIRECTORY_WATCHER)
#  if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#    include <Foundation/IO/Implementation/Win/DirectoryWatcher_win.h>
#  elif PLASMA_ENABLED(PLASMA_USE_POSIX_FILE_API)
#    include <Foundation/IO/Implementation/Posix/DirectoryWatcher_posix.h>
#  else
#    error "Unknown Platform."
#  endif
#endif


PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_DirectoryWatcher);

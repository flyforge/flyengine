#include <Foundation/FoundationPCH.h>

#if (PL_ENABLED(PL_PLATFORM_ANDROID) && PL_ENABLED(PL_SUPPORTS_DIRECTORY_WATCHER))
#  if PL_ENABLED(PL_USE_POSIX_FILE_API)
#    include <Foundation/Platform/Posix/DirectoryWatcher_Posix.h>
#  else
#    error "DirectoryWatcher not implemented."
#  endif
#endif



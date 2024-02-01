#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_OSX)
#  if PL_ENABLED(PL_USE_POSIX_FILE_API)
#    include <Foundation/Platform/Posix/MemoryMappedFile_Posix.h>
#  else
#    include <Foundation/Platform/NoImpl/MemoryMappedFile_NoImpl.h>
#  endif
#endif



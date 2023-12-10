#pragma once

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/PlatformFeatures_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX)
#  include <Foundation/Basics/Platform/OSX/PlatformFeatures_OSX.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
#  include <Foundation/Basics/Platform/Linux/PlatformFeatures_Linux.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/PlatformFeatures_Android.h>
#else
#  error "Undefined platform!"
#endif


// now check that the defines for each feature are set (either to 1 or 0, but they must be defined)

#ifndef PLASMA_SUPPORTS_FILE_ITERATORS
#  error "PLASMA_SUPPORTS_FILE_ITERATORS is not defined."
#endif

#ifndef PLASMA_USE_POSIX_FILE_API
#  error "PLASMA_USE_POSIX_FILE_API is not defined."
#endif

#ifndef PLASMA_SUPPORTS_FILE_STATS
#  error "PLASMA_SUPPORTS_FILE_STATS is not defined."
#endif

#ifndef PLASMA_SUPPORTS_MEMORY_MAPPED_FILE
#  error "PLASMA_SUPPORTS_MEMORY_MAPPED_FILE is not defined."
#endif

#ifndef PLASMA_SUPPORTS_SHARED_MEMORY
#  error "PLASMA_SUPPORTS_SHARED_MEMORY is not defined."
#endif

#ifndef PLASMA_SUPPORTS_DYNAMIC_PLUGINS
#  error "PLASMA_SUPPORTS_DYNAMIC_PLUGINS is not defined."
#endif

#ifndef PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#  error "PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS is not defined."
#endif

#ifndef PLASMA_SUPPORTS_CASE_INSENSITIVE_PATHS
#  error "PLASMA_SUPPORTS_CASE_INSENSITIVE_PATHS is not defined."
#endif

#ifndef PLASMA_SUPPORTS_LONG_PATHS
#  error "PLASMA_SUPPORTS_LONG_PATHS is not defined."
#endif

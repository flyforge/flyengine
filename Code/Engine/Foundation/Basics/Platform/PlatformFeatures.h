#pragma once

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/PlatformFeatures_win.h>
#elif PL_ENABLED(PL_PLATFORM_OSX)
#  include <Foundation/Basics/Platform/OSX/PlatformFeatures_OSX.h>
#elif PL_ENABLED(PL_PLATFORM_LINUX)
#  include <Foundation/Basics/Platform/Linux/PlatformFeatures_Linux.h>
#elif PL_ENABLED(PL_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/PlatformFeatures_Android.h>
#else
#  error "Undefined platform!"
#endif


// now check that the defines for each feature are set (either to 1 or 0, but they must be defined)

#ifndef PL_SUPPORTS_FILE_ITERATORS
#  error "PL_SUPPORTS_FILE_ITERATORS is not defined."
#endif

#ifndef PL_USE_POSIX_FILE_API
#  error "PL_USE_POSIX_FILE_API is not defined."
#endif

#ifndef PL_SUPPORTS_FILE_STATS
#  error "PL_SUPPORTS_FILE_STATS is not defined."
#endif

#ifndef PL_SUPPORTS_MEMORY_MAPPED_FILE
#  error "PL_SUPPORTS_MEMORY_MAPPED_FILE is not defined."
#endif

#ifndef PL_SUPPORTS_SHARED_MEMORY
#  error "PL_SUPPORTS_SHARED_MEMORY is not defined."
#endif

#ifndef PL_SUPPORTS_DYNAMIC_PLUGINS
#  error "PL_SUPPORTS_DYNAMIC_PLUGINS is not defined."
#endif

#ifndef PL_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#  error "PL_SUPPORTS_UNRESTRICTED_FILE_ACCESS is not defined."
#endif

#ifndef PL_SUPPORTS_CASE_INSENSITIVE_PATHS
#  error "PL_SUPPORTS_CASE_INSENSITIVE_PATHS is not defined."
#endif

#ifndef PL_SUPPORTS_LONG_PATHS
#  error "PL_SUPPORTS_LONG_PATHS is not defined."
#endif

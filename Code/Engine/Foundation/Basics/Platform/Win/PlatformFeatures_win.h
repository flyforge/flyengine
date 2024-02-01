#pragma once

/// \file

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef PL_USE_POSIX_FILE_API

#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
#  define PL_USE_POSIX_FILE_API PL_ON
#else
#  define PL_USE_POSIX_FILE_API PL_OFF
#endif

/// Iterating through the file system is supported
#undef PL_SUPPORTS_FILE_ITERATORS

#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
#  define PL_SUPPORTS_FILE_ITERATORS PL_OFF
#else
#  define PL_SUPPORTS_FILE_ITERATORS PL_ON
#endif

/// Getting the stats of a file (modification times etc.) is supported.
#undef PL_SUPPORTS_FILE_STATS
#define PL_SUPPORTS_FILE_STATS PL_ON

/// Directory watcher is supported on non uwp platforms.
#undef PL_SUPPORTS_DIRECTORY_WATCHER
#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
#  define PL_SUPPORTS_DIRECTORY_WATCHER PL_OFF
#else
#  define PL_SUPPORTS_DIRECTORY_WATCHER PL_ON
#endif

/// Memory mapping a file is supported.
#undef PL_SUPPORTS_MEMORY_MAPPED_FILE
#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
#  define PL_SUPPORTS_MEMORY_MAPPED_FILE PL_OFF
#else
#  define PL_SUPPORTS_MEMORY_MAPPED_FILE PL_ON
#endif

/// Shared memory IPC is supported.
#undef PL_SUPPORTS_SHARED_MEMORY
#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
#  define PL_SUPPORTS_SHARED_MEMORY PL_OFF
#else
#  define PL_SUPPORTS_SHARED_MEMORY PL_ON
#endif

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef PL_SUPPORTS_DYNAMIC_PLUGINS
#define PL_SUPPORTS_DYNAMIC_PLUGINS PL_ON

/// Whether applications can access any file (not sandboxed)
#undef PL_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
#  define PL_SUPPORTS_UNRESTRICTED_FILE_ACCESS PL_OFF
#else
#  define PL_SUPPORTS_UNRESTRICTED_FILE_ACCESS PL_ON
#endif

/// Whether file accesses can be done through paths that do not match exact casing
#undef PL_SUPPORTS_CASE_INSENSITIVE_PATHS
#define PL_SUPPORTS_CASE_INSENSITIVE_PATHS PL_ON

/// Whether starting other processes is supported.
#undef PL_SUPPORTS_PROCESSES
#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
#  define PL_SUPPORTS_PROCESSES PL_OFF
#else
#  define PL_SUPPORTS_PROCESSES PL_ON
#endif

// SIMD support
#undef PL_SIMD_IMPLEMENTATION

#if PL_ENABLED(PL_PLATFORM_ARCH_X86)
#  define PL_SIMD_IMPLEMENTATION PL_SIMD_IMPLEMENTATION_SSE
#elif PL_ENABLED(PL_PLATFORM_ARCH_ARM)
#  define PL_SIMD_IMPLEMENTATION PL_SIMD_IMPLEMENTATION_FPU
#else
#  error "Unknown architecture."
#endif

// Writing crashdumps is only supported on windows desktop
#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
#  undef PL_SUPPORTS_CRASH_DUMPS
#  define PL_SUPPORTS_CRASH_DUMPS PL_ON
#endif

// support for writing to files with very long paths is not implemented for UWP
#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
#  undef PL_SUPPORTS_LONG_PATHS
#  define PL_SUPPORTS_LONG_PATHS PL_ON
#endif

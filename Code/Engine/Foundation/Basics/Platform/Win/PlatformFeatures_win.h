#pragma once

/// \file

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef PLASMA_USE_POSIX_FILE_API

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  define PLASMA_USE_POSIX_FILE_API PLASMA_ON
#else
#  define PLASMA_USE_POSIX_FILE_API PLASMA_OFF
#endif

/// Iterating through the file system is supported
#undef PLASMA_SUPPORTS_FILE_ITERATORS

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  define PLASMA_SUPPORTS_FILE_ITERATORS PLASMA_OFF
#else
#  define PLASMA_SUPPORTS_FILE_ITERATORS PLASMA_ON
#endif

/// Getting the stats of a file (modification times etc.) is supported.
#undef PLASMA_SUPPORTS_FILE_STATS
#define PLASMA_SUPPORTS_FILE_STATS PLASMA_ON

/// Directory watcher is supported on non uwp platforms.
#undef PLASMA_SUPPORTS_DIRECTORY_WATCHER
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  define PLASMA_SUPPORTS_DIRECTORY_WATCHER PLASMA_OFF
#else
#  define PLASMA_SUPPORTS_DIRECTORY_WATCHER PLASMA_ON
#endif

/// Memory mapping a file is supported.
#undef PLASMA_SUPPORTS_MEMORY_MAPPED_FILE
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  define PLASMA_SUPPORTS_MEMORY_MAPPED_FILE PLASMA_OFF
#else
#  define PLASMA_SUPPORTS_MEMORY_MAPPED_FILE PLASMA_ON
#endif

/// Shared memory IPC is supported.
#undef PLASMA_SUPPORTS_SHARED_MEMORY
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  define PLASMA_SUPPORTS_SHARED_MEMORY PLASMA_OFF
#else
#  define PLASMA_SUPPORTS_SHARED_MEMORY PLASMA_ON
#endif

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef PLASMA_SUPPORTS_DYNAMIC_PLUGINS
#define PLASMA_SUPPORTS_DYNAMIC_PLUGINS PLASMA_ON

/// Whether applications can access any file (not sandboxed)
#undef PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  define PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS PLASMA_OFF
#else
#  define PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS PLASMA_ON
#endif

/// Whether file accesses can be done through paths that do not match exact casing
#undef PLASMA_SUPPORTS_CASE_INSENSITIVE_PATHS
#define PLASMA_SUPPORTS_CASE_INSENSITIVE_PATHS PLASMA_ON

/// Whether starting other processes is supported.
#undef PLASMA_SUPPORTS_PROCESSES
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  define PLASMA_SUPPORTS_PROCESSES PLASMA_OFF
#else
#  define PLASMA_SUPPORTS_PROCESSES PLASMA_ON
#endif

// SIMD support
#undef PLASMA_SIMD_IMPLEMENTATION

#if PLASMA_ENABLED(PLASMA_PLATFORM_ARCH_X86)
#  define PLASMA_SIMD_IMPLEMENTATION PLASMA_SIMD_IMPLEMENTATION_SSE
#elif PLASMA_ENABLED(PLASMA_PLATFORM_ARCH_ARM)
#  define PLASMA_SIMD_IMPLEMENTATION PLASMA_SIMD_IMPLEMENTATION_FPU
#else
#  error "Unknown architecture."
#endif

// Writing crashdumps is only supported on windows desktop
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#  undef PLASMA_SUPPORTS_CRASH_DUMPS
#  define PLASMA_SUPPORTS_CRASH_DUMPS PLASMA_ON
#endif

// support for writing to files with very long paths is not implemented for UWP
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#  undef PLASMA_SUPPORTS_LONG_PATHS
#  define PLASMA_SUPPORTS_LONG_PATHS PLASMA_ON
#endif

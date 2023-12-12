#pragma once

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef PLASMA_USE_POSIX_FILE_API
#define PLASMA_USE_POSIX_FILE_API PLASMA_ON

/// Iterating through the file system is not supported
#undef PLASMA_SUPPORTS_FILE_ITERATORS
#define PLASMA_SUPPORTS_FILE_ITERATORS PLASMA_OFF

/// Directory watcher is not supported
#undef PLASMA_SUPPORTS_DIRECTORY_WATCHER
#define PLASMA_SUPPORTS_DIRECTORY_WATCHER PLASMA_OFF

/// Getting the stats of a file (modification times etc.) is supported.
#undef PLASMA_SUPPORTS_FILE_STATS
#define PLASMA_SUPPORTS_FILE_STATS PLASMA_ON

/// Memory mapping a file is supported.
#undef PLASMA_SUPPORTS_MEMORY_MAPPED_FILE
#define PLASMA_SUPPORTS_MEMORY_MAPPED_FILE PLASMA_ON

/// Shared memory IPC is not supported.
/// shm_open / shm_unlink deprecated.
/// There is an alternative in ASharedMemory_create but that is only
/// available in API 26 upwards.
/// Could be implemented via JNI which defeats the purpose of a fast IPC channel
/// or we could just use an actual file as the shared memory block.
#undef PLASMA_SUPPORTS_SHARED_MEMORY
#define PLASMA_SUPPORTS_SHARED_MEMORY PLASMA_OFF

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef PLASMA_SUPPORTS_DYNAMIC_PLUGINS
#define PLASMA_SUPPORTS_DYNAMIC_PLUGINS PLASMA_OFF

/// Whether applications can access any file (not sandboxed)
#undef PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#define PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS PLASMA_OFF

/// Whether file accesses can be done through paths that do not match exact casing
#undef PLASMA_SUPPORTS_CASE_INSENSITIVE_PATHS
#define PLASMA_SUPPORTS_CASE_INSENSITIVE_PATHS PLASMA_OFF

/// Whether writing to files with very long paths is supported / implemented
#undef PLASMA_SUPPORTS_LONG_PATHS
#define PLASMA_SUPPORTS_LONG_PATHS PLASMA_ON

/// Whether starting other processes is supported.
#undef PLASMA_SUPPORTS_PROCESSES
#define PLASMA_SUPPORTS_PROCESSES PLASMA_ON

// SIMD support
#undef PLASMA_SIMD_IMPLEMENTATION
#if PLASMA_ENABLED(PLASMA_PLATFORM_64BIT)
#  define PLASMA_SIMD_IMPLEMENTATION PLASMA_SIMD_IMPLEMENTATION_NEON
#else
#  define PLASMA_SIMD_IMPLEMENTATION PLASMA_SIMD_IMPLEMENTATION_FPU
#endif

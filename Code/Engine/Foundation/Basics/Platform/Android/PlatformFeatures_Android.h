#pragma once

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef PL_USE_POSIX_FILE_API
#define PL_USE_POSIX_FILE_API PL_ON

/// If set to one linux posix extensions such as pipe2, dup3, etc are used.
#undef PL_USE_LINUX_POSIX_EXTENSIONS
#define PL_USE_LINUX_POSIX_EXTENSIONS PL_ON

/// Iterating through the file system is not supported
#undef PL_SUPPORTS_FILE_ITERATORS
#define PL_SUPPORTS_FILE_ITERATORS PL_OFF

/// Directory watcher is not supported
#undef PL_SUPPORTS_DIRECTORY_WATCHER
#define PL_SUPPORTS_DIRECTORY_WATCHER PL_OFF

/// Getting the stats of a file (modification times etc.) is supported.
#undef PL_SUPPORTS_FILE_STATS
#define PL_SUPPORTS_FILE_STATS PL_ON

/// Memory mapping a file is supported.
#undef PL_SUPPORTS_MEMORY_MAPPED_FILE
#define PL_SUPPORTS_MEMORY_MAPPED_FILE PL_ON

/// Shared memory IPC is not supported.
/// shm_open / shm_unlink deprecated.
/// There is an alternative in ASharedMemory_create but that is only
/// available in API 26 upwards.
/// Could be implemented via JNI which defeats the purpose of a fast IPC channel
/// or we could just use an actual file as the shared memory block.
#undef PL_SUPPORTS_SHARED_MEMORY
#define PL_SUPPORTS_SHARED_MEMORY PL_OFF

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef PL_SUPPORTS_DYNAMIC_PLUGINS
#define PL_SUPPORTS_DYNAMIC_PLUGINS PL_OFF

/// Whether applications can access any file (not sandboxed)
#undef PL_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#define PL_SUPPORTS_UNRESTRICTED_FILE_ACCESS PL_OFF

/// Whether file accesses can be done through paths that do not match exact casing
#undef PL_SUPPORTS_CASE_INSENSITIVE_PATHS
#define PL_SUPPORTS_CASE_INSENSITIVE_PATHS PL_OFF

/// Whether writing to files with very long paths is supported / implemented
#undef PL_SUPPORTS_LONG_PATHS
#define PL_SUPPORTS_LONG_PATHS PL_ON

/// Whether starting other processes is supported.
#undef PL_SUPPORTS_PROCESSES
#define PL_SUPPORTS_PROCESSES PL_ON

// SIMD support
#undef PL_SIMD_IMPLEMENTATION
#if PL_ENABLED(PL_PLATFORM_64BIT)
#  define PL_SIMD_IMPLEMENTATION PL_SIMD_IMPLEMENTATION_NEON
#else
#  define PL_SIMD_IMPLEMENTATION PL_SIMD_IMPLEMENTATION_FPU
#endif

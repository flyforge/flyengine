#pragma once

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef PL_USE_POSIX_FILE_API
#define PL_USE_POSIX_FILE_API PL_ON

/// Iterating through the file system is not supported
#undef PL_SUPPORTS_FILE_ITERATORS
#define PL_SUPPORTS_FILE_ITERATORS PL_OFF

/// Getting the stats of a file (modification times etc.) is supported.
#undef PL_SUPPORTS_FILE_STATS
#define PL_SUPPORTS_FILE_STATS PL_ON

/// Directory watcher is not supported
#undef PL_SUPPORTS_DIRECTORY_WATCHER
#define PL_SUPPORTS_DIRECTORY_WATCHER PL_OFF

/// Memory mapping a file is supported.
#undef PL_SUPPORTS_MEMORY_MAPPED_FILE
#define PL_SUPPORTS_MEMORY_MAPPED_FILE PL_ON

/// Shared memory IPC is supported.
#undef PL_SUPPORTS_SHARED_MEMORY
#define PL_SUPPORTS_SHARED_MEMORY PL_ON

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef PL_SUPPORTS_DYNAMIC_PLUGINS
#define PL_SUPPORTS_DYNAMIC_PLUGINS PL_OFF

/// Whether applications can access any file (not sandboxed)
#undef PL_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#define PL_SUPPORTS_UNRESTRICTED_FILE_ACCESS PL_ON

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
#define PL_SIMD_IMPLEMENTATION PL_SIMD_IMPLEMENTATION_FPU

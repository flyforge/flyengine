#pragma once

/// \file

/// \brief Used in conjunction with PL_ENABLED and PL_DISABLED for safe checks. Define something to PL_ON or PL_OFF to work with those macros.
#define PL_ON =

/// \brief Used in conjunction with PL_ENABLED and PL_DISABLED for safe checks. Define something to PL_ON or PL_OFF to work with those macros.
#define PL_OFF !

/// \brief Used in conjunction with PL_ON and PL_OFF for safe checks. Use #if PL_ENABLED(x) or #if PL_DISABLED(x) in conditional compilation.
#define PL_ENABLED(x) (1 PL_CONCAT(x, =) 1)

/// \brief Used in conjunction with PL_ON and PL_OFF for safe checks. Use #if PL_ENABLED(x) or #if PL_DISABLED(x) in conditional compilation.
#define PL_DISABLED(x) (1 PL_CONCAT(x, =) 2)

/// \brief Checks whether x AND y are both defined as PL_ON or PL_OFF. Usually used to check whether configurations overlap, to issue an error.
#define PL_IS_NOT_EXCLUSIVE(x, y) ((1 PL_CONCAT(x, =) 1) == (1 PL_CONCAT(y, =) 1))



// All the supported Platforms
#define PL_PLATFORM_WINDOWS PL_OFF         // enabled for all Windows platforms, both UWP and desktop
#define PL_PLATFORM_WINDOWS_UWP PL_OFF     // enabled for UWP apps, together with PL_PLATFORM_WINDOWS
#define PL_PLATFORM_WINDOWS_DESKTOP PL_OFF // enabled for desktop apps, together with PL_PLATFORM_WINDOWS
#define PL_PLATFORM_OSX PL_OFF
#define PL_PLATFORM_LINUX PL_OFF
#define PL_PLATFORM_IOS PL_OFF
#define PL_PLATFORM_ANDROID PL_OFF

// Different Bit OSes
#define PL_PLATFORM_32BIT PL_OFF
#define PL_PLATFORM_64BIT PL_OFF

// Different CPU architectures
#define PL_PLATFORM_ARCH_X86 PL_OFF
#define PL_PLATFORM_ARCH_ARM PL_OFF

// Endianess
#define PL_PLATFORM_LITTLE_ENDIAN PL_OFF
#define PL_PLATFORM_BIG_ENDIAN PL_OFF

// Different Compilers
#define PL_COMPILER_MSVC PL_OFF
#define PL_COMPILER_MSVC_CLANG PL_OFF // Clang front-end with MSVC CodeGen
#define PL_COMPILER_MSVC_PURE PL_OFF  // MSVC front-end and CodeGen, no mixed compilers
#define PL_COMPILER_CLANG PL_OFF
#define PL_COMPILER_GCC PL_OFF

// How to compile the engine
#define PL_COMPILE_ENGINE_AS_DLL PL_OFF
#define PL_COMPILE_FOR_DEBUG PL_OFF
#define PL_COMPILE_FOR_DEVELOPMENT PL_OFF

// Platform Features
#define PL_USE_POSIX_FILE_API PL_OFF
#define PL_USE_LINUX_POSIX_EXTENSIONS PL_OFF // linux specific posix extensions like pipe2, dup3, etc.
#define PL_USE_CPP20_OPERATORS PL_OFF
#define PL_SUPPORTS_FILE_ITERATORS PL_OFF
#define PL_SUPPORTS_FILE_STATS PL_OFF
#define PL_SUPPORTS_DIRECTORY_WATCHER PL_OFF
#define PL_SUPPORTS_MEMORY_MAPPED_FILE PL_OFF
#define PL_SUPPORTS_SHARED_MEMORY PL_OFF
#define PL_SUPPORTS_DYNAMIC_PLUGINS PL_OFF
#define PL_SUPPORTS_UNRESTRICTED_FILE_ACCESS PL_OFF
#define PL_SUPPORTS_CASE_INSENSITIVE_PATHS PL_OFF
#define PL_SUPPORTS_CRASH_DUMPS PL_OFF
#define PL_SUPPORTS_LONG_PATHS PL_OFF

// Allocators
#define PL_ALLOC_GUARD_ALLOCATIONS PL_OFF
#define PL_ALLOC_TRACKING_DEFAULT plAllocatorTrackingMode::Nothing

// Other Features
#define PL_USE_PROFILING PL_OFF

// Hashed String
/// \brief Ref counting on hashed strings adds the possibility to cleanup unused strings. Since ref counting has a performance overhead it is disabled
/// by default.
#define PL_HASHED_STRING_REF_COUNTING PL_OFF

// Math Debug Checks
#define PL_MATH_CHECK_FOR_NAN PL_OFF

// SIMD support
#define PL_SIMD_IMPLEMENTATION_FPU 1
#define PL_SIMD_IMPLEMENTATION_SSE 2
#define PL_SIMD_IMPLEMENTATION_NEON 3

#define PL_SIMD_IMPLEMENTATION 0

// Application entry point code injection (undef and redefine in UserConfig.h if needed)
#define PL_APPLICATION_ENTRY_POINT_CODE_INJECTION

// Whether 'RuntimeConfigs' files should be searched in the old location
#define PL_MIGRATE_RUNTIMECONFIGS PL_OFF

// Interoperability with other libraries
#define PL_INTEROP_STL_STRINGS PL_OFF

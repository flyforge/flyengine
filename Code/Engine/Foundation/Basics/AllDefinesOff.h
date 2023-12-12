#pragma once

/// \file

/// \brief Used in conjunction with PLASMA_ENABLED and PLASMA_DISABLED for safe checks. Define something to PLASMA_ON or PLASMA_OFF to work with those macros.
#define PLASMA_ON =

/// \brief Used in conjunction with PLASMA_ENABLED and PLASMA_DISABLED for safe checks. Define something to PLASMA_ON or PLASMA_OFF to work with those macros.
#define PLASMA_OFF !

/// \brief Used in conjunction with PLASMA_ON and PLASMA_OFF for safe checks. Use #if PLASMA_ENABLED(x) or #if PLASMA_DISABLED(x) in conditional compilation.
#define PLASMA_ENABLED(x) (1 PLASMA_CONCAT(x, =) 1)

/// \brief Used in conjunction with PLASMA_ON and PLASMA_OFF for safe checks. Use #if PLASMA_ENABLED(x) or #if PLASMA_DISABLED(x) in conditional compilation.
#define PLASMA_DISABLED(x) (1 PLASMA_CONCAT(x, =) 2)

/// \brief Checks whether x AND y are both defined as PLASMA_ON or PLASMA_OFF. Usually used to check whether configurations overlap, to issue an error.
#define PLASMA_IS_NOT_EXCLUSIVE(x, y) ((1 PLASMA_CONCAT(x, =) 1) == (1 PLASMA_CONCAT(y, =) 1))



// All the supported Platforms
#define PLASMA_PLATFORM_WINDOWS PLASMA_OFF         // enabled for all Windows platforms, both UWP and desktop
#define PLASMA_PLATFORM_WINDOWS_UWP PLASMA_OFF     // enabled for UWP apps, together with PLASMA_PLATFORM_WINDOWS
#define PLASMA_PLATFORM_WINDOWS_DESKTOP PLASMA_OFF // enabled for desktop apps, together with PLASMA_PLATFORM_WINDOWS
#define PLASMA_PLATFORM_OSX PLASMA_OFF
#define PLASMA_PLATFORM_LINUX PLASMA_OFF
#define PLASMA_PLATFORM_IOS PLASMA_OFF
#define PLASMA_PLATFORM_ANDROID PLASMA_OFF

// Different Bit OSes
#define PLASMA_PLATFORM_32BIT PLASMA_OFF
#define PLASMA_PLATFORM_64BIT PLASMA_OFF

// Different CPU architectures
#define PLASMA_PLATFORM_ARCH_X86 PLASMA_OFF
#define PLASMA_PLATFORM_ARCH_ARM PLASMA_OFF

// Endianess
#define PLASMA_PLATFORM_LITTLE_ENDIAN PLASMA_OFF
#define PLASMA_PLATFORM_BIG_ENDIAN PLASMA_OFF

// Different Compilers
#define PLASMA_COMPILER_MSVC PLASMA_OFF
#define PLASMA_COMPILER_MSVC_CLANG PLASMA_OFF // Clang front-end with MSVC CodeGen
#define PLASMA_COMPILER_MSVC_PURE PLASMA_OFF  // MSVC front-end and CodeGen, no mixed compilers
#define PLASMA_COMPILER_CLANG PLASMA_OFF
#define PLASMA_COMPILER_GCC PLASMA_OFF

// How to compile the engine
#define PLASMA_COMPILE_ENGINE_AS_DLL PLASMA_OFF
#define PLASMA_COMPILE_FOR_DEBUG PLASMA_OFF
#define PLASMA_COMPILE_FOR_DEVELOPMENT PLASMA_OFF

// Platform Features
#define PLASMA_USE_POSIX_FILE_API PLASMA_OFF
#define PLASMA_SUPPORTS_FILE_ITERATORS PLASMA_OFF
#define PLASMA_SUPPORTS_FILE_STATS PLASMA_OFF
#define PLASMA_SUPPORTS_DIRECTORY_WATCHER PLASMA_OFF
#define PLASMA_SUPPORTS_MEMORY_MAPPED_FILE PLASMA_OFF
#define PLASMA_SUPPORTS_SHARED_MEMORY PLASMA_OFF
#define PLASMA_SUPPORTS_DYNAMIC_PLUGINS PLASMA_OFF
#define PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS PLASMA_OFF
#define PLASMA_SUPPORTS_CASE_INSENSITIVE_PATHS PLASMA_OFF
#define PLASMA_SUPPORTS_CRASH_DUMPS PLASMA_OFF
#define PLASMA_SUPPORTS_LONG_PATHS PLASMA_OFF
#define PLASMA_SUPPORTS_GLFW PLASMA_OFF

// Allocators
#define PLASMA_USE_ALLOCATION_TRACKING PLASMA_OFF
#define PLASMA_USE_ALLOCATION_STACK_TRACING PLASMA_OFF
#define PLASMA_USE_GUARDED_ALLOCATIONS PLASMA_OFF

// Other Features
#define PLASMA_USE_PROFILING PLASMA_OFF

// Hashed String
/// \brief Ref counting on hashed strings adds the possibility to cleanup unused strings. Since ref counting has a performance overhead it is disabled
/// by default.
#define PLASMA_HASHED_STRING_REF_COUNTING PLASMA_OFF

// Math Debug Checks
#define PLASMA_MATH_CHECK_FOR_NAN PLASMA_OFF

// SIMD support
#define PLASMA_SIMD_IMPLEMENTATION_FPU 1
#define PLASMA_SIMD_IMPLEMENTATION_SSE 2
#define PLASMA_SIMD_IMPLEMENTATION_NEON 3

#define PLASMA_SIMD_IMPLEMENTATION 0

// Application entry point code injection (undef and redefine in UserConfig.h if needed)
#define PLASMA_APPLICATION_ENTRY_POINT_CODE_INJECTION

// Whether 'RuntimeConfigs' files should be searched in the old location
#define PLASMA_MIGRATE_RUNTIMECONFIGS PLASMA_OFF

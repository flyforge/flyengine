#pragma once

#if PLASMA_DISABLED(PLASMA_PLATFORM_WINDOWS)
#  error "This header should only be included on windows platforms"
#endif

#ifdef _WIN64
#  undef PLASMA_PLATFORM_64BIT
#  define PLASMA_PLATFORM_64BIT PLASMA_ON
#else
#  undef PLASMA_PLATFORM_32BIT
#  define PLASMA_PLATFORM_32BIT PLASMA_ON
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <winapifamily.h>

#undef PLASMA_PLATFORM_WINDOWS_UWP
#undef PLASMA_PLATFORM_WINDOWS_DESKTOP

// Distinguish between Windows desktop and Windows UWP.
#if WINAPI_FAMILY == WINAPI_FAMILY_APP
#  define PLASMA_PLATFORM_WINDOWS_UWP PLASMA_ON
#  define PLASMA_PLATFORM_WINDOWS_DESKTOP PLASMA_OFF
#else
#  define PLASMA_PLATFORM_WINDOWS_UWP PLASMA_OFF
#  define PLASMA_PLATFORM_WINDOWS_DESKTOP PLASMA_ON
#endif

#ifndef NULL
#  define NULL 0
#endif

#undef PLASMA_PLATFORM_LITTLE_ENDIAN
#define PLASMA_PLATFORM_LITTLE_ENDIAN PLASMA_ON

#include <Foundation/Basics/Compiler/Clang/Clang.h>
#include <Foundation/Basics/Compiler/GCC/GCC.h>
#include <Foundation/Basics/Compiler/MSVC/MSVC.h>

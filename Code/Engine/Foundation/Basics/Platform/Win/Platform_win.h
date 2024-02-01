#pragma once

#if PL_DISABLED(PL_PLATFORM_WINDOWS)
#  error "This header should only be included on windows platforms"
#endif

#ifdef _WIN64
#  undef PL_PLATFORM_64BIT
#  define PL_PLATFORM_64BIT PL_ON
#else
#  undef PL_PLATFORM_32BIT
#  define PL_PLATFORM_32BIT PL_ON
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <winapifamily.h>

#undef PL_PLATFORM_WINDOWS_UWP
#undef PL_PLATFORM_WINDOWS_DESKTOP

// Distinguish between Windows desktop and Windows UWP.
#if WINAPI_FAMILY == WINAPI_FAMILY_APP
#  define PL_PLATFORM_WINDOWS_UWP PL_ON
#  define PL_PLATFORM_WINDOWS_DESKTOP PL_OFF
#else
#  define PL_PLATFORM_WINDOWS_UWP PL_OFF
#  define PL_PLATFORM_WINDOWS_DESKTOP PL_ON
#endif

#ifndef NULL
#  define NULL 0
#endif

#undef PL_PLATFORM_LITTLE_ENDIAN
#define PL_PLATFORM_LITTLE_ENDIAN PL_ON

#include <Foundation/Basics/Compiler/Clang/Clang.h>
#include <Foundation/Basics/Compiler/GCC/GCC.h>
#include <Foundation/Basics/Compiler/MSVC/MSVC.h>

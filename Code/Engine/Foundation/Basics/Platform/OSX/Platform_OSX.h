#pragma once

#if PLASMA_DISABLED(PLASMA_PLATFORM_OSX)
#  error "This header should only be included on OSX"
#endif

#include <cstdio>
#include <pthread.h>
#include <sys/malloc.h>
#include <sys/time.h>

// unset common macros
#undef min
#undef max

#include <Foundation/Basics/Compiler/Clang/Clang.h>
#include <Foundation/Basics/Compiler/GCC/GCC.h>

#undef PLASMA_PLATFORM_LITTLE_ENDIAN
#define PLASMA_PLATFORM_LITTLE_ENDIAN PLASMA_ON

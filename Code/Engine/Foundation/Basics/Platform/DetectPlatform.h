#pragma once

#if defined(_WINDOWS) || defined(_WIN32)
#  undef PLASMA_PLATFORM_WINDOWS
#  define PLASMA_PLATFORM_WINDOWS PLASMA_ON

// further distinction between desktop, UWP etc. is done in Platform_win.h

#elif defined(__APPLE__) && defined(__MACH__)
#  include <TargetConditionals.h>

#  if TARGET_OS_MAC == 1
#    undef PLASMA_PLATFORM_OSX
#    define PLASMA_PLATFORM_OSX PLASMA_ON
#  elif TARGET_OS_IPHONE == 1 || TARGET_IPHONE_SIMULATOR == 1
#    undef PLASMA_PLATFORM_IOS
#    define PLASMA_PLATFORM_IOS PLASMA_ON
#  endif

#elif defined(ANDROID)

#  undef PLASMA_PLATFORM_ANDROID
#  define PLASMA_PLATFORM_ANDROID PLASMA_ON

#elif defined(__linux)

#  undef PLASMA_PLATFORM_LINUX
#  define PLASMA_PLATFORM_LINUX PLASMA_ON

//#elif defined(...)
//  #undef PLASMA_PLATFORM_LINUX
//  #define PLASMA_PLATFORM_LINUX PLASMA_ON
#else
#  error "Unknown Platform."
#endif

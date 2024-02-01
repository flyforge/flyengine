#pragma once

#if defined(_WINDOWS) || defined(_WIN32)
#  undef PL_PLATFORM_WINDOWS
#  define PL_PLATFORM_WINDOWS PL_ON

// further distinction between desktop, UWP etc. is done in Platform_win.h

#elif defined(__APPLE__) && defined(__MACH__)
#  include <TargetConditionals.h>

#  if TARGET_OS_MAC == 1
#    undef PL_PLATFORM_OSX
#    define PL_PLATFORM_OSX PL_ON
#  elif TARGET_OS_IPHONE == 1 || TARGET_IPHONE_SIMULATOR == 1
#    undef PL_PLATFORM_IOS
#    define PL_PLATFORM_IOS PL_ON
#  endif

#elif defined(ANDROID)

#  undef PL_PLATFORM_ANDROID
#  define PL_PLATFORM_ANDROID PL_ON

#elif defined(__linux)

#  undef PL_PLATFORM_LINUX
#  define PL_PLATFORM_LINUX PL_ON

//#elif defined(...)
//  #undef PL_PLATFORM_LINUX
//  #define PL_PLATFORM_LINUX PL_ON
#else
#  error "Unknown Platform."
#endif

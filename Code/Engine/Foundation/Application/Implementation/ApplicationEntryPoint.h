#pragma once

#include <Foundation/Basics.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Application/Implementation/Win/ApplicationEntryPoint_win.h>

#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)

#  include <Foundation/Application/Implementation/uwp/ApplicationEntryPoint_uwp.h>

#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)

#  include <Foundation/Application/Implementation/Posix/ApplicationEntryPoint_posix.h>

#elif PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)

#  include <Foundation/Application/Implementation/Android/ApplicationEntryPoint_android.h>

#else
#  error "Missing definition of platform specific entry point!"
#endif

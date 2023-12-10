#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Time/Time.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Time)

  // no dependencies

  ON_BASESYSTEMS_STARTUP
  {
    plTime::Initialize();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

// Include inline file
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Time/Implementation/Win/Time_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX)
#  include <Foundation/Time/Implementation/OSX/Time_osx.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Time/Implementation/Posix/Time_posix.h>
#else
#  error "Time functions are not implemented on current platform"
#endif



PLASMA_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Time);

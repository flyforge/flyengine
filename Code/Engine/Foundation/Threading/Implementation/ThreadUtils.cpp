#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ThreadUtils)

  // no dependencies

  ON_BASESYSTEMS_STARTUP
  {
    plThreadUtils::Initialize();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

// Include inline file
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/ThreadUtils_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/ThreadUtils_posix.h>
#else
#  error "ThreadUtils functions are not implemented on current platform"
#endif

PLASMA_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ThreadUtils);

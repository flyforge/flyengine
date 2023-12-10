#include <Foundation/FoundationPCH.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Application/Implementation/Win/ApplicationEntryPoint_win.h>
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

namespace plApplicationDetails
{
  void SetConsoleCtrlHandler(plMinWindows::BOOL(PLASMA_WINDOWS_WINAPI* consoleHandler)(plMinWindows::DWORD dwCtrlType))
  {
    ::SetConsoleCtrlHandler(consoleHandler, TRUE);
  }

  static plMutex s_shutdownMutex;

  plMutex& GetShutdownMutex()
  {
    return s_shutdownMutex;
  }

} // namespace plApplicationDetails
#endif


PLASMA_STATICLINK_FILE(Foundation, Foundation_Application_Implementation_Win_ApplicationEntryPoint_win);

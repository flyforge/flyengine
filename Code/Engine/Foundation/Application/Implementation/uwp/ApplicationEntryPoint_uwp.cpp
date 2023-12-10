#include <Foundation/FoundationPCH.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  include <Foundation/Application/Implementation/Uwp/ApplicationEntryPoint_uwp.h>
#  include <roapi.h>

namespace plApplicationDetails
{
  plResult InitializeWinrt()
  {
    HRESULT result = RoInitialize(RO_INIT_MULTITHREADED);
    if (FAILED(result))
    {
      plLog::Printf("Failed to init WinRT: %i", result);
      return PLASMA_FAILURE;
    }

    return PLASMA_SUCCESS;
  }

  void UninitializeWinrt() { RoUninitialize(); }
} // namespace plApplicationDetails
#endif


PLASMA_STATICLINK_FILE(Foundation, Foundation_Application_Implementation_uwp_ApplicationEntryPoint_uwp);

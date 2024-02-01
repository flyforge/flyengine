#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
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
      return PL_FAILURE;
    }

    return PL_SUCCESS;
  }

  void UninitializeWinrt() { RoUninitialize(); }
} // namespace plApplicationDetails
#endif



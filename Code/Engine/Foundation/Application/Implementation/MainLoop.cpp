#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>

plResult plRun_Startup(plApplication* pApplicationInstance)
{
  PL_ASSERT_ALWAYS(pApplicationInstance != nullptr, "plRun() requires a valid non-null application instance pointer.");
  PL_ASSERT_ALWAYS(plApplication::s_pApplicationInstance == nullptr, "There can only be one plApplication.");

  // Set application instance pointer to the supplied instance
  plApplication::s_pApplicationInstance = pApplicationInstance;

  PL_SUCCEED_OR_RETURN(pApplicationInstance->BeforeCoreSystemsStartup());

  // this will startup all base and core systems
  // 'StartupHighLevelSystems' must not be done before a window is available (if at all)
  // so we don't do that here
  plStartup::StartupCoreSystems();

  pApplicationInstance->AfterCoreSystemsStartup();
  return PL_SUCCESS;
}

void plRun_MainLoop(plApplication* pApplicationInstance)
{
  while (pApplicationInstance->Run() == plApplication::Execution::Continue)
  {
  }
}

void plRun_Shutdown(plApplication* pApplicationInstance)
{
  // high level systems shutdown
  // may do nothing, if the high level systems were never initialized
  {
    pApplicationInstance->BeforeHighLevelSystemsShutdown();
    plStartup::ShutdownHighLevelSystems();
    pApplicationInstance->AfterHighLevelSystemsShutdown();
  }

  // core systems shutdown
  {
    pApplicationInstance->BeforeCoreSystemsShutdown();
    plStartup::ShutdownCoreSystems();
    pApplicationInstance->AfterCoreSystemsShutdown();
  }

  // Flush standard output to make log available.
  fflush(stdout);
  fflush(stderr);

  // Reset application instance so code running after the app will trigger asserts etc. to be cleaned up
  // Destructor is called by entry point function
  plApplication::s_pApplicationInstance = nullptr;

  // memory leak reporting cannot be done here, because the application instance is still alive and may still hold on to memory that needs
  // to be freed first
}

void plRun(plApplication* pApplicationInstance)
{
  if (plRun_Startup(pApplicationInstance).Succeeded())
  {
    plRun_MainLoop(pApplicationInstance);
  }
  plRun_Shutdown(pApplicationInstance);
}



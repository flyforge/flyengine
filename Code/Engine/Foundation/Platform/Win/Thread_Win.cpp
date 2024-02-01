#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)

#  include <Foundation/Threading/Thread.h>

// Thread entry point used to launch plRunnable instances
DWORD __stdcall plThreadClassEntryPoint(LPVOID pThreadParameter)
{
  PL_ASSERT_RELEASE(pThreadParameter != nullptr, "thread parameter in thread entry point must not be nullptr!");

  plThread* pThread = reinterpret_cast<plThread*>(pThreadParameter);

  return RunThread(pThread);
}

#endif



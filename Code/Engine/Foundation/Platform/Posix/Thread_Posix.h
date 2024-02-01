#include <Foundation/FoundationInternal.h>
PL_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Threading/Thread.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

// Thread entry point used to launch plRunnable instances
void* plThreadClassEntryPoint(void* pThreadParameter)
{
  PL_ASSERT_RELEASE(pThreadParameter != nullptr, "thread parameter in thread entry point must not be nullptr!");

  plThread* pThread = reinterpret_cast<plThread*>(pThreadParameter);

  RunThread(pThread);

  return nullptr;
}

/// \endcond

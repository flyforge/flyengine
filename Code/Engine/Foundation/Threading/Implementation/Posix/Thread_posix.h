#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

// Deactivate Doxygen document generation for the following block.
/// \cond

// Thread entry point used to launch plRunnable instances
void* plThreadClassEntryPoint(void* pThreadParameter)
{
  PLASMA_ASSERT_RELEASE(pThreadParameter != nullptr, "thread parameter in thread entry point must not be nullptr!");

  plThread* pThread = reinterpret_cast<plThread*>(pThreadParameter);

  RunThread(pThread);

  return nullptr;
}

/// \endcond

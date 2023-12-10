#ifdef PLASMA_STACKTRACER_UWP_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define PLASMA_STACKTRACER_UWP_INL_H_INCLUDED

#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

void plStackTracer::OnPluginEvent(const plPluginEvent& e) {}

// static
plUInt32 plStackTracer::GetStackTrace(plArrayPtr<void*>& trace, void* pContext)
{
  return 0;
}

// static
void plStackTracer::ResolveStackTrace(const plArrayPtr<void*>& trace, PrintFunc printFunc)
{
  char szBuffer[512] = "Stack Traces are currently not supported on UWP";

  printFunc(szBuffer);
}

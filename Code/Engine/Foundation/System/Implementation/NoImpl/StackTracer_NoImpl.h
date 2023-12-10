#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

void plStackTracer::OnPluginEvent(const plPluginEvent& e) {}

plUInt32 plStackTracer::GetStackTrace(plArrayPtr<void*>& trace, void* pContext)
{
  return 0;
}

void plStackTracer::ResolveStackTrace(const plArrayPtr<void*>& trace, PrintFunc printFunc)
{
}

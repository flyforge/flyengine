#include <Foundation/System/StackTracer.h>

void plStackTracer::OnPluginEvent(const plPluginEvent& e)
{
}

plUInt32 plStackTracer::GetStackTrace(plArrayPtr<void*>& trace, void* pContext)
{
  return 0;
}

void plStackTracer::ResolveStackTrace(const plArrayPtr<void*>& trace, PrintFunc printFunc)
{
}

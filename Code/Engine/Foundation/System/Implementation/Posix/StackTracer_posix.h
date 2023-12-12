#ifdef PLASMA_STACKTRACER_POSIX_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define PLASMA_STACKTRACER_POSIX_INL_H_INCLUDED

#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Math/Math.h>
#include <execinfo.h>

void plStackTracer::OnPluginEvent(const plPluginEvent& e) {}

// static
plUInt32 plStackTracer::GetStackTrace(plArrayPtr<void*>& trace, void* pContext)
{
  int iSymbols = backtrace(trace.GetPtr(), trace.GetCount());

  return iSymbols;
}

// static
void plStackTracer::ResolveStackTrace(const plArrayPtr<void*>& trace, PrintFunc printFunc)
{
  char szBuffer[512];

  char** ppSymbols = backtrace_symbols(trace.GetPtr(), trace.GetCount());

  if (ppSymbols != nullptr)
  {
    for (plUInt32 i = 0; i < trace.GetCount(); i++)
    {
      int iLen = plMath::Min(strlen(ppSymbols[i]), (size_t)PLASMA_ARRAY_SIZE(szBuffer) - 2);
      memcpy(szBuffer, ppSymbols[i], iLen);
      szBuffer[iLen] = '\n';
      szBuffer[iLen + 1] = '\0';

      printFunc(szBuffer);
    }

    free(ppSymbols);
  }
}

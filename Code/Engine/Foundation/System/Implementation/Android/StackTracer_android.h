#ifdef PLASMA_STACKTRACER_ANDROID_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define PLASMA_STACKTRACER_ANDROID_INL_H_INCLUDED

#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <dlfcn.h>
#include <unwind.h>

void plStackTracer::OnPluginEvent(const plPluginEvent& e) {}

struct Backtrace
{
  plUInt32 uiPos = 0;
  plArrayPtr<void*> trace;
};

static _Unwind_Reason_Code BacktraceCallback(struct _Unwind_Context* pContext, void* pData)
{
  Backtrace& backtrace = *(Backtrace*)pData;

  if (backtrace.uiPos < backtrace.trace.GetCount())
  {
    backtrace.trace[backtrace.uiPos] = reinterpret_cast<void*>(_Unwind_GetIP(pContext));
    backtrace.uiPos++;
    return _URC_NO_REASON;
  }
  else
  {
    return _URC_END_OF_STACK;
  }
}
// static
plUInt32 plStackTracer::GetStackTrace(plArrayPtr<void*>& trace, void* pContext)
{
  Backtrace backtrace;
  backtrace.trace = trace;
  _Unwind_Reason_Code res = _Unwind_Backtrace(BacktraceCallback, &backtrace);
  return backtrace.uiPos;
}

// static
void plStackTracer::ResolveStackTrace(const plArrayPtr<void*>& trace, PrintFunc printFunc)
{
  char szBuffer[512];
  for (plUInt32 i = 0; i < trace.GetCount(); i++)
  {
    Dl_info info;
    if (dladdr(trace[i], &info) && info.dli_sname)
    {
      int iLen = plMath::Min(strlen(info.dli_sname), (size_t)PLASMA_ARRAY_SIZE(szBuffer) - 2);
      memcpy(szBuffer, info.dli_sname, iLen);
      szBuffer[iLen] = '\n';
      szBuffer[iLen + 1] = '\0';
      printFunc(szBuffer);
    }
    else
    {
      printFunc("Unresolved stack.\n");
    }
  }
}

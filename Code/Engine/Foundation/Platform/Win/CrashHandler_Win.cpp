#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)

#  include <Foundation/Logging/Log.h>
#  include <Foundation/System/CrashHandler.h>
#  include <Foundation/System/MiniDumpUtils.h>
#  include <Foundation/System/StackTracer.h>

static void PrintHelper(const char* szString)
{
  plLog::Printf("%s", szString);
}

static LONG WINAPI plCrashHandlerFunc(struct _EXCEPTION_POINTERS* pExceptionInfo)
{
  static plMutex s_CrashMutex;
  PL_LOCK(s_CrashMutex);

  static bool s_bAlreadyHandled = false;

  if (s_bAlreadyHandled == false)
  {
    if (plCrashHandler::GetCrashHandler() != nullptr)
    {
      s_bAlreadyHandled = true;
      plCrashHandler::GetCrashHandler()->HandleCrash(pExceptionInfo);
    }
  }

  return EXCEPTION_CONTINUE_SEARCH;
}

void plCrashHandler::SetCrashHandler(plCrashHandler* pHandler)
{
  s_pActiveHandler = pHandler;

  if (s_pActiveHandler != nullptr)
  {
    SetUnhandledExceptionFilter(plCrashHandlerFunc);
  }
  else
  {
    SetUnhandledExceptionFilter(nullptr);
  }
}

bool plCrashHandler_WriteMiniDump::WriteOwnProcessMiniDump(void* pOsSpecificData)
{
#  if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
  plStatus res = plMiniDumpUtils::WriteOwnProcessMiniDump(m_sDumpFilePath, (_EXCEPTION_POINTERS*)pOsSpecificData);
  if (res.Failed())
    plLog::Printf("WriteOwnProcessMiniDump failed: %s\n", res.m_sMessage.GetData());
  return res.Succeeded();
#  else
  return false;
#  endif
}

void plCrashHandler_WriteMiniDump::PrintStackTrace(void* pOsSpecificData)
{
  _EXCEPTION_POINTERS* pExceptionInfo = (_EXCEPTION_POINTERS*)pOsSpecificData;

  plLog::Printf("***Unhandled Exception:***\n");
  plLog::Printf("Exception: %08x", (plUInt32)pExceptionInfo->ExceptionRecord->ExceptionCode);

  {
    plLog::Printf("\n\n***Stack Trace:***\n");
    void* pBuffer[64];
    plArrayPtr<void*> tempTrace(pBuffer);
    const plUInt32 uiNumTraces = plStackTracer::GetStackTrace(tempTrace, pExceptionInfo->ContextRecord);

    plStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &PrintHelper);
  }
}

#endif



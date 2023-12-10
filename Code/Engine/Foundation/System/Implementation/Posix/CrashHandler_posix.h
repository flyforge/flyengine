#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Logging/Log.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/StackTracer.h>

#include <csignal>
#include <cxxabi.h>
#include <unistd.h>

static void plCrashHandlerFunc() noexcept
{
  if (plCrashHandler::GetCrashHandler() != nullptr)
  {
    plCrashHandler::GetCrashHandler()->HandleCrash(nullptr);
  }

  // restore the original signal handler for the abort signal and raise one so the kernel can do a core dump
  std::signal(SIGABRT, SIG_DFL);
  std::raise(SIGABRT);
}

static void plSignalHandler(int signum)
{
  plLog::Printf("***Unhandled Signal:***\n");
  switch (signum)
  {
    case SIGINT:
      plLog::Printf("Signal SIGINT: interrupt\n");
      break;
    case SIGILL:
      plLog::Printf("Signal SIGILL: illegal instruction - invalid function image\n");
      break;
    case SIGFPE:
      plLog::Printf("Signal SIGFPE: floating point exception\n");
      break;
    case SIGSEGV:
      plLog::Printf("Signal SIGSEGV: segment violation\n");
      break;
    case SIGTERM:
      plLog::Printf("Signal SIGTERM: Software termination signal from kill\n");
      break;
    case SIGABRT:
      plLog::Printf("Signal SIGABRT: abnormal termination triggered by abort call\n");
      break;
    default:
      plLog::Printf("Signal %i: unknown signal\n", signal);
      break;
  }

  if (plCrashHandler::GetCrashHandler() != nullptr)
  {
    plCrashHandler::GetCrashHandler()->HandleCrash(nullptr);
  }

  // forward the signal back to the OS so that it can write a core dump
  std::signal(signum, SIG_DFL);
  kill(getpid(), signum);
}

void plCrashHandler::SetCrashHandler(plCrashHandler* pHandler)
{
  s_pActiveHandler = pHandler;

  if (s_pActiveHandler != nullptr)
  {
    std::signal(SIGINT, plSignalHandler);
    std::signal(SIGILL, plSignalHandler);
    std::signal(SIGFPE, plSignalHandler);
    std::signal(SIGSEGV, plSignalHandler);
    std::signal(SIGTERM, plSignalHandler);
    std::signal(SIGABRT, plSignalHandler);
    std::set_terminate(plCrashHandlerFunc);
  }
  else
  {
    std::signal(SIGINT, nullptr);
    std::signal(SIGILL, nullptr);
    std::signal(SIGFPE, nullptr);
    std::signal(SIGSEGV, nullptr);
    std::signal(SIGTERM, nullptr);
    std::signal(SIGABRT, nullptr);
    std::set_terminate(nullptr);
  }
}

bool plCrashHandler_WriteMiniDump::WriteOwnProcessMiniDump(void* pOsSpecificData)
{
  return false;
}

void plCrashHandler_WriteMiniDump::PrintStackTrace(void* pOsSpecificData)
{
  plLog::Printf("***Unhandled Exception:***\n");

  // plLog::Printf exception type
  if (std::type_info* type = abi::__cxa_current_exception_type())
  {
    if (const char* szName = type->name())
    {
      int status = -1;
      // Try to print nice name
      if (char* szNiceName = abi::__cxa_demangle(szName, 0, 0, &status))
        plLog::Printf("Exception: %s\n", szNiceName);
      else
        plLog::Printf("Exception: %s\n", szName);
    }
  }

  {
    plLog::Printf("\n\n***Stack Trace:***\n");

    void* pBuffer[64];
    plArrayPtr<void*> tempTrace(pBuffer);
    const plUInt32 uiNumTraces = plStackTracer::GetStackTrace(tempTrace);

    plStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &PrintHelper);
  }
}

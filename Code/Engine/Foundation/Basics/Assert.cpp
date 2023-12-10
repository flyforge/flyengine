#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Utilities/ConversionUtils.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS) && PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
#  include <crtdbg.h>
#endif

#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC)
void MSVC_OutOfLine_DebugBreak(...)
{
  __debugbreak();
}
#endif

bool plDefaultAssertHandler(const char* szSourceFile, plUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  char szTemp[1024 * 4] = "";
  plStringUtils::snprintf(szTemp, PLASMA_ARRAY_SIZE(szTemp),
    "\n\n *** Assertion ***\n\n    Expression: \"%s\"\n    Function: \"%s\"\n    File: \"%s\"\n    Line: %u\n    Message: \"%s\"\n\n", szExpression,
    szFunction, szSourceFile, uiLine, szAssertMsg);
  szTemp[1024 * 4 - 1] = '\0';

  plLog::Print(szTemp);

  if (plSystemInformation::IsDebuggerAttached())
    return true;

  // If no debugger is attached we append the assert to a common file so that postmortem debugging is easier
  if (FILE* assertLogFP = fopen("plDefaultAssertHandlerOutput.txt", "a"))
  {
    time_t timeUTC = time(&timeUTC);
    tm* ptm = gmtime(&timeUTC);

    char szTimeStr[256] = {0};
    sprintf(szTimeStr, "UTC: %s", asctime(ptm));
    fputs(szTimeStr, assertLogFP);

    fputs(szTemp, assertLogFP);

    fclose(assertLogFP);
  }

  // if the environment variable "PLASMA_SILENT_ASSERTS" is set to a value like "1", "on", "true", "enable" or "yes"
  // the assert handler will never show a GUI that may block the application from continuing to run
  // this should be set on machines that run tests which should never get stuck but rather crash asap
  bool bSilentAsserts = false;

  if (plEnvironmentVariableUtils::IsVariableSet("PLASMA_SILENT_ASSERTS"))
  {
    bSilentAsserts = plEnvironmentVariableUtils::GetValueInt("PLASMA_SILENT_ASSERTS", bSilentAsserts ? 1 : 0) != 0;
  }

  if (bSilentAsserts)
    return true;

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)

    // make sure the cursor is definitely shown, since the user must be able to click buttons
#  if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
    // Todo: Use modern Windows API to show cursor in current window.
    // http://stackoverflow.com/questions/37956628/change-mouse-pointer-in-uwp-app
#  else
  plInt32 iHideCursor = 1;
  while (ShowCursor(true) < 0)
    ++iHideCursor;
#  endif

#  if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)

  plInt32 iRes = _CrtDbgReport(_CRT_ASSERT, szSourceFile, uiLine, nullptr, "'%s'\nFunction: %s\nMessage: %s", szExpression, szFunction, szAssertMsg);

  // currently we will ALWAYS trigger the breakpoint / crash (except for when the user presses 'ignore')
  if (iRes == 0)
  {
    // when the user ignores the assert, restore the cursor show/hide state to the previous count
#    if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
    // Todo: Use modern Windows API to restore cursor.
#    else
    for (plInt32 i = 0; i < iHideCursor; ++i)
      ShowCursor(false);
#    endif

    return false;
  }

#  else


#    if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  MessageBoxA(nullptr, szTemp, "Assertion", MB_ICONERROR);
#    endif

#  endif

#endif

  // always do a debug-break
  // in release-builds this will just crash the app
  return true;
}

static plAssertHandler g_AssertHandler = &plDefaultAssertHandler;

plAssertHandler plGetAssertHandler()
{
  return g_AssertHandler;
}

void plSetAssertHandler(plAssertHandler handler)
{
  g_AssertHandler = handler;
}

bool plFailedCheck(const char* szSourceFile, plUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szMsg)
{
  // always do a debug-break if no assert handler is installed
  if (g_AssertHandler == nullptr)
    return true;

  return (*g_AssertHandler)(szSourceFile, uiLine, szFunction, szExpression, szMsg);
}

bool plFailedCheck(const char* szSourceFile, plUInt32 uiLine, const char* szFunction, const char* szExpression, const class plFormatString& msg)
{
  plStringBuilder tmp;
  return plFailedCheck(szSourceFile, uiLine, szFunction, szExpression, msg.GetTextCStr(tmp));
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_Basics_Assert);

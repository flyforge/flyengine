
#pragma once

/// \file

#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/MemoryTracker.h>

// Disable C++/CX adds.
#pragma warning(disable : 4447)

class plApplication;
extern PLASMA_FOUNDATION_DLL plResult plUWPRun(plApplication* pApp);

namespace plApplicationDetails
{
  PLASMA_FOUNDATION_DLL plResult InitializeWinrt();
  PLASMA_FOUNDATION_DLL void UninitializeWinrt();

  template <typename AppClass, typename... Args>
  int EntryFunc(Args&&... arguments)
  {
    alignas(PLASMA_ALIGNMENT_OF(AppClass)) static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.

    if (InitializeWinrt().Failed())
    {
      return 1;
    }

    AppClass* pApp = new (appBuffer) AppClass(std::forward<Args>(arguments)...);

    plUWPRun(pApp).IgnoreResult();

    const int iReturnCode = pApp->GetReturnCode();
    if (iReturnCode != 0)
    {
      std::string text = pApp->TranslateReturnCode();
      if (!text.empty())
        printf("Return Code: '%s'\n", text.c_str());
    }

    UninitializeWinrt();

    return iReturnCode;
  }
} // namespace plApplicationDetails

/// \brief Same as PLASMA_APPLICATION_ENTRY_POINT but should be used for applications that shall always show a console window.
#define PLASMA_CONSOLEAPP_ENTRY_POINT(AppClass, ...)                                                                                 \
  alignas(PLASMA_ALIGNMENT_OF(AppClass)) static char appBuffer[sizeof(AppClass)]; /* Not on the stack to cope with smaller stacks */ \
                                                                                                                                 \
  PLASMA_APPLICATION_ENTRY_POINT_CODE_INJECTION                                                                                      \
  int main(int argc, const char** argv) { return ::plApplicationDetails::EntryFunc<AppClass>(__VA_ARGS__); }

/// \brief This macro allows for easy creation of application entry points (since they can't be placed in DLLs)
///
/// Just use the macro in a cpp file of your application and supply your app class (must be derived from plApplication).
/// The additional (optional) parameters are passed to the constructor of your app class.
#define PLASMA_APPLICATION_ENTRY_POINT(AppClass, ...)                                                   \
  PLASMA_APPLICATION_ENTRY_POINT_CODE_INJECTION                                                         \
  int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) \
  {                                                                                                 \
    return ::plApplicationDetails::EntryFunc<AppClass>(__VA_ARGS__);                                \
  }

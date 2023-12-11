
#pragma once

/// \file

#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

namespace plApplicationDetails
{
  PLASMA_FOUNDATION_DLL void SetConsoleCtrlHandler(plMinWindows::BOOL(PLASMA_WINDOWS_WINAPI* consoleHandler)(plMinWindows::DWORD dwCtrlType));
  PLASMA_FOUNDATION_DLL plMutex& GetShutdownMutex();

  template <typename AppClass, typename... Args>
  int ConsoleEntry(int iArgc, const char** pArgv, Args&&... arguments)
  {
#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC)             // Internal compiler error in MSVC. Can not align buffer otherwise the compiler will crash.
    static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.
#else
    alignas(PLASMA_ALIGNMENT_OF(AppClass)) static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.
#endif

    // This mutex will prevent the console shutdown handler to return
    // as long as this entry point is not finished executing
    // (see consoleHandler below).
    PLASMA_LOCK(GetShutdownMutex());

    static AppClass* pApp = new (appBuffer) AppClass(std::forward<Args>(arguments)...);
    pApp->SetCommandLineArguments((plUInt32)iArgc, pArgv);

    // This handler overrides the default handler
    // (which would call ExitProcess, which leads to disorderly engine shutdowns)
    const auto consoleHandler = [](plMinWindows::DWORD ctrlType) -> plMinWindows::BOOL {
      // We have to wait until the application has shut down orderly
      // since Windows will kill everything after this handler returns
      pApp->SetReturnCode(ctrlType);
      pApp->RequestQuit();
      PLASMA_LOCK(GetShutdownMutex());
      return 1; // returns TRUE, which deactivates the default console control handler
    };
    SetConsoleCtrlHandler(consoleHandler);

    plRun(pApp); // Life cycle & run method calling

    const int iReturnCode = pApp->GetReturnCode();
    if (iReturnCode != 0)
    {
      std::string text = pApp->TranslateReturnCode();
      if (!text.empty())
        plLog::Printf("Return Code: %i = '%s'\n", iReturnCode, text.c_str());
      else
        plLog::Printf("Return Code: %i\n", iReturnCode, text.c_str());
    }

    const bool memLeaks = pApp->IsMemoryLeakReportingEnabled();
    pApp->~AppClass();
    memset((void*)pApp, 0, sizeof(AppClass));
    if (memLeaks)
      plMemoryTracker::DumpMemoryLeaks();

    return iReturnCode;
  }

  template <typename AppClass, typename... Args>
  int ApplicationEntry(Args&&... arguments)
  {
#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC)             // Internal compiler error in MSVC. Can not align buffer otherwise the compiler will crash.
    static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.
#else
    alignas(PLASMA_ALIGNMENT_OF(AppClass)) static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.
#endif

    AppClass* pApp = new (appBuffer) AppClass(std::forward<Args>(arguments)...);
    pApp->SetCommandLineArguments((plUInt32)__argc, const_cast<const char**>(__argv));
    plRun(pApp); // Life cycle & run method calling

    const int iReturnCode = pApp->GetReturnCode();
    if (iReturnCode != 0)
    {
      std::string text = pApp->TranslateReturnCode();
      if (!text.empty())
        plLog::Printf("Return Code: '%s'\n", text.c_str());
    }

    const bool memLeaks = pApp->IsMemoryLeakReportingEnabled();
    pApp->~AppClass();
    memset((void*)pApp, 0, sizeof(AppClass));
    if (memLeaks)
      plMemoryTracker::DumpMemoryLeaks();

    return iReturnCode;
  }
} // namespace plApplicationDetails

/// \brief Same as PLASMA_APPLICATION_ENTRY_POINT but should be used for applications that shall always show a console window.
#define PLASMA_CONSOLEAPP_ENTRY_POINT(AppClass, ...)                                                \
  /* Enables that on machines with multiple GPUs the NVIDIA / AMD GPU is preferred */           \
  extern "C"                                                                                    \
  {                                                                                             \
    _declspec(dllexport) plMinWindows::DWORD NvOptimusEnablement = 0x00000001;                  \
    _declspec(dllexport) plMinWindows::DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; \
  }                                                                                             \
  PLASMA_APPLICATION_ENTRY_POINT_CODE_INJECTION                                     \
  int main(int argc, const char** argv) { return plApplicationDetails::ConsoleEntry<AppClass>(argc, argv, __VA_ARGS__); }

// If windows.h is already included use the native types, otherwise use types from plMinWindows
//
// In PLASMA_APPLICATION_ENTRY_POINT we use macro magic to concatenate strings in such a way that depending on whether windows.h has
// been included in the mean time, either the macro is chosen which expands to the proper Windows.h type
// or the macro that expands to our plMinWindows type.
// Unfortunately we cannot do the decision right here, as Windows.h may not yet be included, but may get included later.
#define _PLASMA_APPLICATION_ENTRY_POINT_HINSTANCE HINSTANCE
#define _PLASMA_APPLICATION_ENTRY_POINT_LPSTR LPSTR
#define _PLASMA_APPLICATION_ENTRY_POINT_HINSTANCE_WINDOWS_ plMinWindows::HINSTANCE
#define _PLASMA_APPLICATION_ENTRY_POINT_LPSTR_WINDOWS_ plMinWindows::LPSTR

#ifndef _In_
#  define UndefSAL
#  define _In_
#  define _In_opt_
#endif

/// \brief This macro allows for easy creation of application entry points (since they can't be placed in DLLs)
///
/// Just use the macro in a cpp file of your application and supply your app class (must be derived from plApplication).
/// The additional (optional) parameters are passed to the constructor of your app class.
#define PLASMA_APPLICATION_ENTRY_POINT(AppClass, ...)                                                                          \
  /* Enables that on machines with multiple GPUs the NVIDIA / AMD GPU is preferred */                                      \
  extern "C"                                                                                                               \
  {                                                                                                                        \
    _declspec(dllexport) plMinWindows::DWORD NvOptimusEnablement = 0x00000001;                                             \
    _declspec(dllexport) plMinWindows::DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;                            \
  }                                                                                                                        \
  PLASMA_APPLICATION_ENTRY_POINT_CODE_INJECTION                                                                                \
  int PLASMA_WINDOWS_CALLBACK WinMain(_In_ PLASMA_CONCAT(_PLASMA_, PLASMA_CONCAT(APPLICATION_ENTRY_POINT_HINSTANCE, _WINDOWS_)) hInstance, \
    _In_opt_ PLASMA_CONCAT(_PLASMA_, PLASMA_CONCAT(APPLICATION_ENTRY_POINT_HINSTANCE, _WINDOWS_)) hPrevInstance,                       \
    _In_ PLASMA_CONCAT(_PLASMA_, PLASMA_CONCAT(APPLICATION_ENTRY_POINT_LPSTR, _WINDOWS_)) lpCmdLine, _In_ int nCmdShow)                \
  {                                                                                                                        \
    return plApplicationDetails::ApplicationEntry<AppClass>(__VA_ARGS__);                                                  \
  }

#ifdef UndefSAL
#  undef _In_
#  undef _In_opt_
#endif
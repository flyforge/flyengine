#pragma once

/// \file
#include <Foundation/Application/Application.h>
#include <Foundation/Basics/Platform/Android/AndroidUtils.h>

class plApplication;

extern PLASMA_FOUNDATION_DLL void plAndroidRun(struct android_app* pAndroidApp, plApplication* pApp);

namespace plApplicationDetails
{
  template <typename AppClass, typename... Args>
  void EntryFunc(struct android_app* pAndroidApp, Args&&... arguments)
  {
    alignas(PLASMA_ALIGNMENT_OF(AppClass)) static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.
    plAndroidUtils::SetAndroidApp(pAndroidApp);
    AppClass* pApp = new (appBuffer) AppClass(std::forward<Args>(arguments)...);

    plAndroidRun(pAndroidApp, pApp);

    pApp->~AppClass();
    memset(pApp, 0, sizeof(AppClass));
  }
} // namespace plApplicationDetails


/// \brief Same as PLASMA_APPLICATION_ENTRY_POINT but should be used for applications that shall always show a console window.
#define PLASMA_CONSOLEAPP_ENTRY_POINT(...) PLASMA_APPLICATION_ENTRY_POINT(__VA_ARGS__)

/// \brief This macro allows for easy creation of application entry points (since they can't be placed in DLLs)
///
/// Just use the macro in a cpp file of your application and supply your app class (must be derived from plApplication).
/// The additional (optional) parameters are passed to the constructor of your app class.
#define PLASMA_APPLICATION_ENTRY_POINT(AppClass, ...)                                                                                \
  alignas(PLASMA_ALIGNMENT_OF(AppClass)) static char appBuffer[sizeof(AppClass)]; /* Not on the stack to cope with smaller stacks */ \
  PLASMA_APPLICATION_ENTRY_POINT_CODE_INJECTION                                                                                      \
  extern "C" void android_main(struct android_app* app) { ::plApplicationDetails::EntryFunc<AppClass>(app, ##__VA_ARGS__); }

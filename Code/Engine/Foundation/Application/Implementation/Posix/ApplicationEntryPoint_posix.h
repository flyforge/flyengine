
#pragma once

/// \file


/// \brief Same as PLASMA_APPLICATION_ENTRY_POINT but should be used for applications that shall always show a console window.
#define PLASMA_CONSOLEAPP_ENTRY_POINT PLASMA_APPLICATION_ENTRY_POINT


/// \brief This macro allows for easy creation of application entry points (since they can't be placed in DLLs)
///
/// Just use the macro in a cpp file of your application and supply your app class (must be derived from plApplication).
/// The additional (optional) parameters are passed to the constructor of your app class.
#define PLASMA_APPLICATION_ENTRY_POINT(AppClass, ...)                                                                                \
  alignas(PLASMA_ALIGNMENT_OF(AppClass)) static char appBuffer[sizeof(AppClass)]; /* Not on the stack to cope with smaller stacks */ \
                                                                                                                                 \
  PLASMA_APPLICATION_ENTRY_POINT_CODE_INJECTION                                                                                      \
  int main(int argc, const char** argv)                                                                                          \
  {                                                                                                                              \
                                                                                                                                 \
    AppClass* pApp = new (appBuffer) AppClass(__VA_ARGS__);                                                                      \
    pApp->SetCommandLineArguments((plUInt32)argc, argv);                                                                         \
    plRun(pApp); /* Life cycle & run method calling */                                                                           \
    const int iReturnCode = pApp->GetReturnCode();                                                                               \
    if (iReturnCode != 0)                                                                                                        \
    {                                                                                                                            \
      const char* szReturnCode = pApp->TranslateReturnCode();                                                                    \
      if (szReturnCode != nullptr && szReturnCode[0] != '\0')                                                                    \
        plLog::Printf("Return Code: '%s'\n", szReturnCode);                                                                      \
    }                                                                                                                            \
    pApp->~AppClass();                                                                                                           \
    memset((void*)pApp, 0, sizeof(AppClass));                                                                                    \
    return iReturnCode;                                                                                                          \
  }

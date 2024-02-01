#include <EditorEngineProcess/EditorEngineProcessPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)

#  include <EditorEngineProcess/EngineProcGameAppUWP.h>
PL_APPLICATION_ENTRY_POINT(plEngineProcessGameApplicationUWP);

#else

#  include <EditorEngineProcess/EngineProcGameApp.h>
PL_APPLICATION_ENTRY_POINT(plEngineProcessGameApplication);

#endif

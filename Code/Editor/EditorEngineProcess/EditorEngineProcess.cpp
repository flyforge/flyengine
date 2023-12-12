#include <EditorEngineProcess/EditorEngineProcessPCH.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)

#  include <EditorEngineProcess/EngineProcGameAppUWP.h>
PLASMA_APPLICATION_ENTRY_POINT(PlasmaEngineProcessGameApplicationUWP);

#else

#  include <EditorEngineProcess/EngineProcGameApp.h>
PLASMA_APPLICATION_ENTRY_POINT(PlasmaEngineProcessGameApplication);

#endif

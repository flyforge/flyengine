#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// BEGIN-DOCS-CODE-SNIPPET: dll-export-defines
// Configure the DLL Import/Export Define
#if PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_SAMPLEGAMEPLUGIN_LIB
#    define PLASMA_SAMPLEGAMEPLUGIN_DLL PLASMA_DECL_EXPORT
#  else
#    define PLASMA_SAMPLEGAMEPLUGIN_DLL PLASMA_DECL_IMPORT
#  endif
#else
#  define PLASMA_SAMPLEGAMEPLUGIN_DLL
#endif
// END-DOCS-CODE-SNIPPET

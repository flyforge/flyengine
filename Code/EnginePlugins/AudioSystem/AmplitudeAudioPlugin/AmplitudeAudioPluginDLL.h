#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// BEGIN-DOCS-CODE-SNIPPET: dll-export-defines
// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_AMPLITUDEAUDIOPLUGIN_LIB
#    define PL_AMPLITUDEAUDIOPLUGIN_DLL PL_DECL_EXPORT
#  else
#    define PL_AMPLITUDEAUDIOPLUGIN_DLL PL_DECL_IMPORT
#  endif
#else
#  define PL_AMPLITUDEAUDIOPLUGIN_DLL
#endif
// END-DOCS-CODE-SNIPPET

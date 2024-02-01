#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERCORE_LIB
#    define PL_RENDERERCORE_DLL PL_DECL_EXPORT
#  else
#    define PL_RENDERERCORE_DLL PL_DECL_IMPORT
#  endif
#else
#  define PL_RENDERERCORE_DLL
#endif

#define PL_EMBED_FONT_FILE PL_ON

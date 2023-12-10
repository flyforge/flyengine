#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_TEXTURE_LIB
#    define PLASMA_TEXTURE_DLL PLASMA_DECL_EXPORT
#  else
#    define PLASMA_TEXTURE_DLL PLASMA_DECL_IMPORT
#  endif
#else
#  define PLASMA_TEXTURE_DLL
#endif

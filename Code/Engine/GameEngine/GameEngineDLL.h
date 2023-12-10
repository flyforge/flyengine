#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_GAMEENGINE_LIB
#    define PLASMA_GAMEENGINE_DLL PLASMA_DECL_EXPORT
#  else
#    define PLASMA_GAMEENGINE_DLL PLASMA_DECL_IMPORT
#  endif
#else
#  define PLASMA_GAMEENGINE_DLL
#endif

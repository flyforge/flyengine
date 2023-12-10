#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// Configure the DLL Import/Export Define
#if PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_ENGINEPLUGINJOLT_LIB
#    define PLASMA_ENGINEPLUGINJOLT_DLL PLASMA_DECL_EXPORT
#  else
#    define PLASMA_ENGINEPLUGINJOLT_DLL PLASMA_DECL_IMPORT
#  endif
#else
#  define PLASMA_ENGINEPLUGINJOLT_DLL
#endif

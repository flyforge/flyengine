#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_CORE_LIB
#    define PLASMA_CORE_DLL PLASMA_DECL_EXPORT
#    define PLASMA_CORE_DLL_FRIEND PLASMA_DECL_EXPORT_FRIEND
#  else
#    define PLASMA_CORE_DLL PLASMA_DECL_IMPORT
#    define PLASMA_CORE_DLL_FRIEND PLASMA_DECL_IMPORT_FRIEND
#  endif
#else
#  define PLASMA_CORE_DLL
#  define PLASMA_CORE_DLL_FRIEND
#endif
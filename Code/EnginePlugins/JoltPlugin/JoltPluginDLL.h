#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_JOLTPLUGIN_LIB
#    define PL_JOLTPLUGIN_DLL PL_DECL_EXPORT
#  else
#    define PL_JOLTPLUGIN_DLL PL_DECL_IMPORT
#  endif
#else
#  define PL_JOLTPLUGIN_DLL
#endif

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/System/PlatformFeatures.h>

// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_XBOXCONTROLLERPLUGIN_LIB
#    define PL_XBOXCONTROLLER_DLL PL_DECL_EXPORT
#  else
#    define PL_XBOXCONTROLLER_DLL PL_DECL_IMPORT
#  endif
#else
#  define PL_XBOXCONTROLLER_DLL
#endif

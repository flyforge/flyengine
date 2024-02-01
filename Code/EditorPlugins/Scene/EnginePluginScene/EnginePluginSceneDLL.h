#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_ENGINEPLUGINSCENE_LIB
#    define PL_ENGINEPLUGINSCENE_DLL PL_DECL_EXPORT
#  else
#    define PL_ENGINEPLUGINSCENE_DLL PL_DECL_IMPORT
#  endif
#else
#  define PL_ENGINEPLUGINSCENE_DLL
#endif

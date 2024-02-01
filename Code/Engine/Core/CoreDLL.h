#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_CORE_LIB
#    define PL_CORE_DLL PL_DECL_EXPORT
#    define PL_CORE_DLL_FRIEND PL_DECL_EXPORT_FRIEND
#  else
#    define PL_CORE_DLL PL_DECL_IMPORT
#    define PL_CORE_DLL_FRIEND PL_DECL_IMPORT_FRIEND
#  endif
#else
#  define PL_CORE_DLL
#  define PL_CORE_DLL_FRIEND
#endif

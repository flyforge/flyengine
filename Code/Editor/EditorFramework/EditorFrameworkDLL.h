#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_EDITORFRAMEWORK_LIB
#    define PL_EDITORFRAMEWORK_DLL PL_DECL_EXPORT
#  else
#    define PL_EDITORFRAMEWORK_DLL PL_DECL_IMPORT
#  endif
#else
#  define PL_EDITORFRAMEWORK_DLL
#endif

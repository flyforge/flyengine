#pragma once

// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_JOLTCOOKING_LIB
#    define PL_JOLTCOOKING_DLL PL_DECL_EXPORT
#  else
#    define PL_JOLTCOOKING_DLL PL_DECL_IMPORT
#  endif
#else
#  define PL_JOLTCOOKING_DLL
#endif

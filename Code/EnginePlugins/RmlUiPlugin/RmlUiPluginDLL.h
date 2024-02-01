#pragma once

// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RMLUIPLUGIN_LIB
#    define PL_RMLUIPLUGIN_DLL PL_DECL_EXPORT
#  else
#    define PL_RMLUIPLUGIN_DLL PL_DECL_IMPORT
#  endif
#else
#  define PL_RMLUIPLUGIN_DLL
#endif

#ifndef RMLUI_USE_CUSTOM_RTTI
#  define RMLUI_USE_CUSTOM_RTTI
#endif

#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERDX11_LIB
#    define PL_RENDERERDX11_DLL PL_DECL_EXPORT
#  else
#    define PL_RENDERERDX11_DLL PL_DECL_IMPORT
#  endif
#else
#  define PL_RENDERERDX11_DLL
#endif


#define PL_GAL_DX11_RELEASE(d3dobj)                                                                                                                  \
  do                                                                                                                                                 \
  {                                                                                                                                                  \
    if ((d3dobj) != nullptr)                                                                                                                         \
    {                                                                                                                                                \
      (d3dobj)->Release();                                                                                                                           \
      (d3dobj) = nullptr;                                                                                                                            \
    }                                                                                                                                                \
  } while (0)

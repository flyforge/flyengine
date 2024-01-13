#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERDX12_LIB
#    define PLASMA_RENDERERDX12_DLL PLASMA_DECL_EXPORT
#  else
#    define PLASMA_RENDERERDX12_DLL PLASMA_DECL_IMPORT
#  endif
#else
#  define PLASMA_RENDERERDX11_DLL
#endif

#define PLASMA_GAL_DX12_RELEASE(d3dobj)
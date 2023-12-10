#pragma once

#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct GPUTimingScope;

/// Sets profiling marker and GPU timings for the current scope.
class PLASMA_RENDERERFOUNDATION_DLL plProfilingScopeAndMarker : public plProfilingScope
{
public:
  static GPUTimingScope* Start(plGALCommandEncoder* pCommandEncoder, const char* szName);
  static void Stop(plGALCommandEncoder* pCommandEncoder, GPUTimingScope*& ref_pTimingScope);

  plProfilingScopeAndMarker(plGALCommandEncoder* pCommandEncoder, const char* szName);

  ~plProfilingScopeAndMarker();

protected:
  plGALCommandEncoder* m_pCommandEncoder;
  GPUTimingScope* m_pTimingScope;
};

#if PLASMA_ENABLED(PLASMA_USE_PROFILING) || defined(PLASMA_DOCS)

/// \brief Profiles the current scope using the given name and also inserts a marker with the given GALContext.
#  define PLASMA_PROFILE_AND_MARKER(GALContext, szName) plProfilingScopeAndMarker PLASMA_CONCAT(_plProfilingScope, PLASMA_SOURCE_LINE)(GALContext, szName)

#else

#  define PLASMA_PROFILE_AND_MARKER(GALContext, szName) /*empty*/

#endif

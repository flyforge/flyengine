#pragma once

#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct GPUTimingScope;

/// Sets profiling marker and GPU timings for the current scope.
class PL_RENDERERFOUNDATION_DLL plProfilingScopeAndMarker : public plProfilingScope
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

#if PL_ENABLED(PL_USE_PROFILING) || defined(PL_DOCS)

/// \brief Profiles the current scope using the given name and also inserts a marker with the given GALContext.
#  define PL_PROFILE_AND_MARKER(GALContext, szName) plProfilingScopeAndMarker PL_CONCAT(_plProfilingScope, PL_SOURCE_LINE)(GALContext, szName)

#else

#  define PL_PROFILE_AND_MARKER(GALContext, szName) /*empty*/

#endif

# Brotli-G SDK Support

set(PLASMA_3RDPARTY_BROTLIG_SUPPORT ON CACHE BOOL "Whether support for Brotli-G compression should be enabled")
mark_as_advanced(FORCE PLASMA_3RDPARTY_BROTLIG_SUPPORT)

macro(pl_requires_brotlig)
  pl_requires_one_of(PLASMA_CMAKE_PLATFORM_WINDOWS PLASMA_CMAKE_PLATFORM_LINUX)
  pl_requires(PLASMA_3RDPARTY_BROTLIG_SUPPORT)

  if(PLASMA_CMAKE_PLATFORM_WINDOWS_UWP)
    return()
  endif()
endmacro()

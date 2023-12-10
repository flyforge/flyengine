### RmlUi
set (PLASMA_BUILD_RMLUI ON CACHE BOOL "Whether support for RmlUi should be added")
mark_as_advanced(FORCE PLASMA_BUILD_RMLUI)

macro(pl_requires_rmlui)
  pl_requires_windows()
  pl_requires(PLASMA_BUILD_RMLUI)
  if (PLASMA_CMAKE_PLATFORM_WINDOWS_UWP)
    return()
  endif()
endmacro()
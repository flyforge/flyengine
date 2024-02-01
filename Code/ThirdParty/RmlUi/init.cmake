### RmlUi
set (PL_BUILD_RMLUI ON CACHE BOOL "Whether support for RmlUi should be added")
mark_as_advanced(FORCE PL_BUILD_RMLUI)

macro(pl_requires_rmlui)
  pl_requires_one_of(PL_CMAKE_PLATFORM_WINDOWS PL_CMAKE_PLATFORM_LINUX)
  pl_requires(PL_BUILD_RMLUI)
  if (PL_CMAKE_PLATFORM_WINDOWS_UWP)
    return()
  endif()
endmacro()
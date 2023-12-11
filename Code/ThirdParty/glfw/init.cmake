######################################
### Glfw support
######################################

# On Linux glfw support is required
if(PLASMA_CMAKE_PLATFORM_LINUX)
  # On linux we want to use GLFW by default
  set (PLASMA_3RDPARTY_GLFW_SUPPORT ON CACHE BOOL "Use GLFW to manage windows and input")
  message(STATUS "Using GLFW by default")
else()
  # On all other platforms glfw support is not tested (but might work).
  set (PLASMA_3RDPARTY_GLFW_SUPPORT OFF CACHE BOOL "Use GLFW to manage windows and input")
  if (NOT PLASMA_CMAKE_PLATFORM_WINDOWS_DESKTOP)
    mark_as_advanced(FORCE PLASMA_3RDPARTY_GLFW_SUPPORT)
  endif()
endif()

# Cmake variables which are platform dependent

# #####################################
# ## General settings
# #####################################
if(PLASMA_CMAKE_PLATFORM_WINDOWS OR PLASMA_CMAKE_PLATFORM_LINUX)
	set(PLASMA_COMPILE_ENGINE_AS_DLL ON CACHE BOOL "Whether to compile the code as a shared libraries (DLL).")
	mark_as_advanced(FORCE PLASMA_COMPILE_ENGINE_AS_DLL)
else()
	unset(PLASMA_COMPILE_ENGINE_AS_DLL CACHE)
endif()

# #####################################
# ## Experimental Editor support on Linux
# #####################################
if(PLASMA_CMAKE_PLATFORM_LINUX)
	set (PLASMA_EXPERIMENTAL_EDITOR_ON_LINUX OFF CACHE BOOL "Wether or not to build the editor on linux")
endif()

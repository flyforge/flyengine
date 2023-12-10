# find the folder into which the OpenXR loader has been installed

# early out, if this target has been created before
if(TARGET plOpenXR::Loader)
	return()
endif()

set(PLASMA_OPENXR_LOADER_DIR "PLASMA_OPENXR_LOADER_DIR-NOTFOUND" CACHE PATH "Directory of OpenXR loader installation")
set(PLASMA_OPENXR_HEADERS_DIR "PLASMA_OPENXR_HEADERS_DIR-NOTFOUND" CACHE PATH "Directory of OpenXR headers installation")
set(PLASMA_OPENXR_PREVIEW_DIR "" CACHE PATH "Directory of OpenXR preview include root")
set(PLASMA_OPENXR_REMOTING_DIR "" CACHE PATH "Directory of OpenXR remoting installation")
mark_as_advanced(FORCE PLASMA_OPENXR_LOADER_DIR)
mark_as_advanced(FORCE PLASMA_OPENXR_HEADERS_DIR)
mark_as_advanced(FORCE PLASMA_OPENXR_PREVIEW_DIR)
mark_as_advanced(FORCE PLASMA_OPENXR_REMOTING_DIR)

pl_pull_compiler_and_architecture_vars()

if((PLASMA_OPENXR_LOADER_DIR STREQUAL "PLASMA_OPENXR_LOADER_DIR-NOTFOUND") OR(PLASMA_OPENXR_LOADER_DIR STREQUAL "") OR(PLASMA_OPENXR_HEADERS_DIR STREQUAL "PLASMA_OPENXR_HEADERS_DIR-NOTFOUND") OR(PLASMA_OPENXR_HEADERS_DIR STREQUAL "") OR(PLASMA_OPENXR_REMOTING_DIR STREQUAL "PLASMA_OPENXR_REMOTING_DIR-NOTFOUND") OR(PLASMA_OPENXR_REMOTING_DIR STREQUAL ""))
	pl_nuget_init()
	execute_process(COMMAND ${NUGET} restore ${CMAKE_SOURCE_DIR}/Code/EnginePlugins/OpenXRPlugin/packages.config -PackagesDirectory ${CMAKE_BINARY_DIR}/packages
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
	set(PLASMA_OPENXR_LOADER_DIR "${CMAKE_BINARY_DIR}/packages/OpenXR.Loader.1.0.10.2" CACHE PATH "Directory of OpenXR loader installation" FORCE)
	set(PLASMA_OPENXR_HEADERS_DIR "${CMAKE_BINARY_DIR}/packages/OpenXR.Headers.1.0.10.2" CACHE PATH "Directory of OpenXR headers installation" FORCE)
	set(PLASMA_OPENXR_REMOTING_DIR "${CMAKE_BINARY_DIR}/packages/Microsoft.Holographic.Remoting.OpenXr.2.4.0" CACHE PATH "Directory of OpenXR remoting installation" FORCE)
endif()

if(PLASMA_CMAKE_PLATFORM_WINDOWS_UWP)
	set(OPENXR_DYNAMIC ON)
	find_path(PLASMA_OPENXR_HEADERS_DIR include/openxr/openxr.h)
	set(PLASMAOPENXR_FOUND ON)
	if(PLASMA_CMAKE_ARCHITECTURE_ARM)
		if(PLASMA_CMAKE_ARCHITECTURE_64BIT)
			set(OPENXR_BIN_PREFIX "arm64_uwp")
		else()
			set(OPENXR_BIN_PREFIX "arm_uwp")
		endif()
	else()
		if(PLASMA_CMAKE_ARCHITECTURE_64BIT)
			set(OPENXR_BIN_PREFIX "x64_uwp")
		else()
			set(OPENXR_BIN_PREFIX "Win32_uwp")
		endif()
	endif()

elseif(PLASMA_CMAKE_PLATFORM_WINDOWS_DESKTOP)
	set(OPENXR_DYNAMIC ON)
	find_path(PLASMA_OPENXR_HEADERS_DIR include/openxr/openxr.h)
	set(PLASMAOPENXR_FOUND ON)
	if(PLASMA_CMAKE_ARCHITECTURE_64BIT)
		set(OPENXR_BIN_PREFIX "x64")
		find_path(PLASMA_OPENXR_REMOTING_DIR build/native/include/openxr/openxr_msft_holographic_remoting.h)
	else()
		set(OPENXR_BIN_PREFIX "Win32")
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(plOpenXR DEFAULT_MSG PLASMA_OPENXR_LOADER_DIR)
find_package_handle_standard_args(plOpenXR DEFAULT_MSG PLASMA_OPENXR_HEADERS_DIR)
find_package_handle_standard_args(plOpenXR DEFAULT_MSG PLASMA_OPENXR_REMOTING_DIR)

if(PLASMAOPENXR_FOUND)
	add_library(plOpenXR::Loader SHARED IMPORTED)

	if(OPENXR_DYNAMIC)
		set_target_properties(plOpenXR::Loader PROPERTIES IMPORTED_LOCATION "${PLASMA_OPENXR_LOADER_DIR}/native/${OPENXR_BIN_PREFIX}/release/bin/openxr_loader.dll")
		set_target_properties(plOpenXR::Loader PROPERTIES IMPORTED_LOCATION_DEBUG "${PLASMA_OPENXR_LOADER_DIR}/native/${OPENXR_BIN_PREFIX}/release/bin/openxr_loader.dll")
	endif()

	set_target_properties(plOpenXR::Loader PROPERTIES IMPORTED_IMPLIB "${PLASMA_OPENXR_LOADER_DIR}/native/${OPENXR_BIN_PREFIX}/release/lib/openxr_loader.lib")
	set_target_properties(plOpenXR::Loader PROPERTIES IMPORTED_IMPLIB_DEBUG "${PLASMA_OPENXR_LOADER_DIR}/native/${OPENXR_BIN_PREFIX}/release/lib/openxr_loader.lib")

	set_target_properties(plOpenXR::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PLASMA_OPENXR_HEADERS_DIR}/include")

	if(NOT PLASMA_OPENXR_PREVIEW_DIR STREQUAL "")
		set_target_properties(plOpenXR::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PLASMA_OPENXR_HEADERS_DIR}/include")
	endif()

	if(PLASMA_CMAKE_PLATFORM_WINDOWS_DESKTOP AND PLASMA_CMAKE_ARCHITECTURE_64BIT)
		# As this is a windows only library, we are relying on the .targets file to handle to includes / imports.
		add_library(plOpenXR::Remoting SHARED IMPORTED)
		set_target_properties(plOpenXR::Remoting PROPERTIES IMPORTED_LOCATION ${PLASMA_OPENXR_REMOTING_DIR}/build/native/Microsoft.Holographic.Remoting.OpenXr.targets)
	endif()

	pl_uwp_mark_import_as_content(plOpenXR::Loader)
endif()

unset(OPENXR_DYNAMIC)
unset(OPENXR_BIN_PREFIX)
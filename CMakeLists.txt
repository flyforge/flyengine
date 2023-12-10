cmake_minimum_required(VERSION 3.20)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "In-source builds are not supported. Please choose a different binary directory.")
	return()
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/plCMakeConfig.cmake")
get_property(PLASMA_CMAKE_RELPATH GLOBAL PROPERTY PLASMA_CMAKE_RELPATH)
get_property(PLASMA_CMAKE_RELPATH_CODE GLOBAL PROPERTY PLASMA_CMAKE_RELPATH_CODE)

# Required to find cmake modules like 'FindDirectX.cmake'
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/${PLASMA_CMAKE_RELPATH}")

include("plUtils")

if(PROJECT_IS_PLUGIN)
	file(RELATIVE_PATH PLASMA_SUBMODULE_PREFIX_PATH ${CMAKE_SOURCE_DIR})
else()
	file(RELATIVE_PATH PLASMA_SUBMODULE_PREFIX_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
endif()

set_property(GLOBAL PROPERTY PLASMA_SUBMODULE_PREFIX_PATH ${PLASMA_SUBMODULE_PREFIX_PATH})

if(PLASMA_SUBMODULE_PREFIX_PATH STREQUAL "")
	set(PLASMA_SUBMODULE_MODE FALSE)
else()
	set(PLASMA_SUBMODULE_MODE TRUE)
endif()

set_property(GLOBAL PROPERTY PLASMA_SUBMODULE_MODE ${PLASMA_SUBMODULE_MODE})

# Set a global variable pointing to the root of the PLASMA source tree
set(PLASMA_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set_property(GLOBAL PROPERTY PLASMA_ROOT ${PLASMA_ROOT})

pl_build_filter_init()

pl_detect_project_name(DETECTED_NAME)

if(NOT PLASMA_SUBMODULE_MODE)
	pl_detect_languages()

	set(PLASMA_SOLUTION_NAME ${DETECTED_NAME} CACHE STRING "The name of the solution")
	project(${PLASMA_SOLUTION_NAME}
		VERSION 1.0
		DESCRIPTION "An open-source game engine"
		HOMEPAGE_URL "https://wwww.plasmagameengine.com"
		LANGUAGES ${PLASMA_LANGUAGES})
endif()

pl_set_build_types()
pl_check_build_type()
pl_write_configuration_txt()

if(PLASMA_CUSTOM_TOOLCHAIN_FILE)
	message(STATUS "Including PLASMA_CUSTOM_TOOLCHAIN_FILE: '${PLASMA_CUSTOM_TOOLCHAIN_FILE}'")
	include("${PLASMA_CUSTOM_TOOLCHAIN_FILE}")
endif()

pl_pull_platform_vars()

include("plPlatformVars")

pl_find_qt()
pl_init_projects()

get_property(SELECTED_FILTER_NAME GLOBAL PROPERTY PLASMA_BUILD_FILTER_SELECTED)
if (NOT ${SELECTED_FILTER_NAME} MATCHES "Everything")
	set(CMAKE_SUPPRESS_REGENERATION true)
endif()
add_subdirectory(${PLASMA_CMAKE_RELPATH_CODE})

pl_add_external_projects_folder("_1")
pl_add_external_projects_folder("_2")
pl_add_external_projects_folder("_3")

if(NOT PLASMA_SUBMODULE_MODE)
	if(TARGET Editor)
		# sets the Editor project as the default startup project in Visual Studio
		set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT Editor)
	endif()
endif()

# finalizers may run into problems in split solutions
# however, they aren't needed there, since CI will always build all projects
# and finalizers are typically only needed to set up build dependencies
if (${SELECTED_FILTER_NAME} MATCHES "Everything")
pl_finalize_projects()
endif()

get_property(EXPORT_PROJECTS GLOBAL PROPERTY "EXPORT_PROJECTS")
export(TARGETS ${EXPORT_PROJECTS} FILE "${PLASMA_OUTPUT_DIRECTORY_DLL}/plExport.cmake")

set(EXPORT_INFO "
set(EXPINP_OUTPUT_DIRECTORY_DLL ${PLASMA_OUTPUT_DIRECTORY_DLL})
set(EXPINP_OUTPUT_DIRECTORY_LIB ${PLASMA_OUTPUT_DIRECTORY_LIB})
set(EXPINP_BINARY_DIR ${CMAKE_BINARY_DIR})
set(EXPINP_SOURCE_DIR ${CMAKE_SOURCE_DIR})
")

file(WRITE "${PLASMA_OUTPUT_DIRECTORY_DLL}/plExportInfo.cmake" ${EXPORT_INFO})

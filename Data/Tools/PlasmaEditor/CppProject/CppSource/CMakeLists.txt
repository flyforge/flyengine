cmake_minimum_required(VERSION 3.21)

project (CppProject VERSION 1.0 LANGUAGES C CXX)

if(PROJECT_IS_TOP_LEVEL)

	set (PLASMA_SDK_DIR "" CACHE PATH "The plEngine SDK root directory.")

	message(STATUS "PLASMA_SDK_DIR is set to '${PLASMA_SDK_DIR}'")

	# fix output directory
	set (PLASMA_OUTPUT_DIRECTORY_LIB "${PLASMA_SDK_DIR}/Output/Lib" CACHE PATH "Where to store the compiled .lib files." FORCE)
	set (PLASMA_OUTPUT_DIRECTORY_DLL "${PLASMA_SDK_DIR}/Output/Bin" CACHE PATH "Where to store the compiled .dll files." FORCE)

	include("${PLASMA_SDK_DIR}/plCMakeConfig.cmake")
	get_property(PLASMA_CMAKE_RELPATH GLOBAL PROPERTY PLASMA_CMAKE_RELPATH)
	get_property(PLASMA_CMAKE_RELPATH_CODE GLOBAL PROPERTY PLASMA_CMAKE_RELPATH_CODE)

	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PLASMA_SDK_DIR}/${PLASMA_CMAKE_RELPATH}")

	set(PLASMA_ROOT ${PLASMA_SDK_DIR})
	set_property(GLOBAL PROPERTY PLASMA_ROOT ${PLASMA_ROOT})

	include("plUtils")
	include("plPlatformVars")

	pl_include_plExport()

	pl_configure_external_project()

endif()

# include all projects in the sub-folders
pl_add_all_subdirs()
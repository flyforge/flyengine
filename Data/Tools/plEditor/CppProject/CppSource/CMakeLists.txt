cmake_minimum_required(VERSION 3.21)

project (CppProject VERSION 1.0 LANGUAGES C CXX)

if(PROJECT_IS_TOP_LEVEL)

	set (PL_SDK_DIR "" CACHE PATH "The plEngine SDK root directory.")

	message(STATUS "PL_SDK_DIR is set to '${PL_SDK_DIR}'")

	# fix output directory
	set (PL_OUTPUT_DIRECTORY_LIB "${PL_SDK_DIR}/Output/Lib" CACHE PATH "Where to store the compiled .lib files." FORCE)
	set (PL_OUTPUT_DIRECTORY_DLL "${PL_SDK_DIR}/Output/Bin" CACHE PATH "Where to store the compiled .dll files." FORCE)

	include("${PL_SDK_DIR}/plCMakeConfig.cmake")
	get_property(PL_CMAKE_RELPATH GLOBAL PROPERTY PL_CMAKE_RELPATH)
	get_property(PL_CMAKE_RELPATH_CODE GLOBAL PROPERTY PL_CMAKE_RELPATH_CODE)

	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PL_SDK_DIR}/${PL_CMAKE_RELPATH}")
	
	set(PL_ROOT ${PL_SDK_DIR})
	set_property(GLOBAL PROPERTY PL_ROOT ${PL_ROOT})

	include("plUtils")
	include("plPlatform")

	pl_include_plExport()

	pl_configure_external_project()

endif()

# include all projects in the sub-folders
pl_add_all_subdirs()

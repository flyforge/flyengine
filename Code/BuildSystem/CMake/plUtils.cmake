include(CheckIncludeFileCXX)

file(GLOB UTILS_FILES "${CMAKE_CURRENT_LIST_DIR}/CMakeUtils/*.cmake")

# automatically include all files in the CMakeUtils subfolder
foreach(UTILS_FILE ${UTILS_FILES})
	include("${UTILS_FILE}")
endforeach()

# #####################################
# ## pl_pull_config_vars()
# #####################################
macro(pl_pull_config_vars)
	get_property(PL_BUILDTYPENAME_DEBUG GLOBAL PROPERTY PL_BUILDTYPENAME_DEBUG)
	get_property(PL_BUILDTYPENAME_DEV GLOBAL PROPERTY PL_BUILDTYPENAME_DEV)
	get_property(PL_BUILDTYPENAME_RELEASE GLOBAL PROPERTY PL_BUILDTYPENAME_RELEASE)

	get_property(PL_BUILDTYPENAME_DEBUG_UPPER GLOBAL PROPERTY PL_BUILDTYPENAME_DEBUG_UPPER)
	get_property(PL_BUILDTYPENAME_DEV_UPPER GLOBAL PROPERTY PL_BUILDTYPENAME_DEV_UPPER)
	get_property(PL_BUILDTYPENAME_RELEASE_UPPER GLOBAL PROPERTY PL_BUILDTYPENAME_RELEASE_UPPER)

	get_property(PL_DEV_BUILD_LINKERFLAGS GLOBAL PROPERTY PL_DEV_BUILD_LINKERFLAGS)

	get_property(PL_CMAKE_RELPATH GLOBAL PROPERTY PL_CMAKE_RELPATH)
	get_property(PL_CMAKE_RELPATH_CODE GLOBAL PROPERTY PL_CMAKE_RELPATH_CODE)
	get_property(PL_CONFIG_PATH_7ZA GLOBAL PROPERTY PL_CONFIG_PATH_7ZA)

	get_property(PL_CONFIG_QT_WINX64_URL GLOBAL PROPERTY PL_CONFIG_QT_WINX64_URL)
	get_property(PL_CONFIG_QT_WINX64_VERSION GLOBAL PROPERTY PL_CONFIG_QT_WINX64_VERSION)

	get_property(PL_CONFIG_VULKAN_SDK_LINUXX64_VERSION GLOBAL PROPERTY PL_CONFIG_VULKAN_SDK_LINUXX64_VERSION)
	get_property(PL_CONFIG_VULKAN_SDK_LINUXX64_URL GLOBAL PROPERTY PL_CONFIG_VULKAN_SDK_LINUXX64_URL)
endmacro()

# #####################################
# ## pl_pull_output_vars(LIB_OUTPUT_DIR DLL_OUTPUT_DIR)
# #####################################
macro(pl_pull_output_vars LIB_OUTPUT_DIR DLL_OUTPUT_DIR)
	pl_pull_all_vars()
	pl_pull_config_vars()

	set(SUB_DIR "")
	set(PLATFORM_PREFIX "")
	set(PLATFORM_POSTFIX "")
	set(ARCH "x${PL_CMAKE_ARCHITECTURE_POSTFIX}")

	# PLATFORM-TODO (build output path hook? add more variables?)
	if(PL_CMAKE_PLATFORM_WINDOWS_UWP)
		# UWP has deployment problems if all applications output to the same path.
		set(SUB_DIR "/${TARGET_NAME}")
		set(PLATFORM_PREFIX "uwp_")

		if(${ARCH} STREQUAL "x32")
			set(ARCH "x86")
		endif()

		if(${ARCH} STREQUAL "xArm32")
			set(ARCH "arm")
		endif()

		if(${ARCH} STREQUAL "xArm64")
			set(ARCH "arm64")
		endif()

	elseif(PL_CMAKE_PLATFORM_WINDOWS_DESKTOP)
		set(PLATFORM_POSTFIX "_win10")

	elseif(PL_CMAKE_PLATFORM_EMSCRIPTEN)
		set(PLATFORM_POSTFIX "_wasm")

	elseif(PL_CMAKE_PLATFORM_ANDROID)
		set(PLATFORM_POSTFIX "_android")
	endif()

	string(TOLOWER ${PL_CMAKE_GENERATOR_PREFIX} LOWER_GENERATOR_PREFIX)

	set(PRE_PATH "${PL_CMAKE_PLATFORM_PREFIX}${PL_CMAKE_GENERATOR_PREFIX}${PL_CMAKE_COMPILER_POSTFIX}")
	set(OUTPUT_DEBUG "${PRE_PATH}${PL_BUILDTYPENAME_DEBUG}${PL_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")
	set(OUTPUT_RELEASE "${PRE_PATH}${PL_BUILDTYPENAME_RELEASE}${PL_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")
	set(OUTPUT_DEV "${PRE_PATH}${PL_BUILDTYPENAME_DEV}${PL_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")

	set(OUTPUT_DLL_DEBUG "${DLL_OUTPUT_DIR}/${OUTPUT_DEBUG}")
	set(OUTPUT_LIB_DEBUG "${LIB_OUTPUT_DIR}/${OUTPUT_DEBUG}")

	set(OUTPUT_DLL_RELEASE "${DLL_OUTPUT_DIR}/${OUTPUT_RELEASE}")
	set(OUTPUT_LIB_RELEASE "${LIB_OUTPUT_DIR}/${OUTPUT_RELEASE}")

	set(OUTPUT_DLL_DEV "${DLL_OUTPUT_DIR}/${OUTPUT_DEV}")
	set(OUTPUT_LIB_DEV "${LIB_OUTPUT_DIR}/${OUTPUT_DEV}")

endmacro()

# #####################################
# ## pl_set_target_output_dirs(<target> <lib-output-dir> <dll-output-dir>)
# #####################################
function(pl_set_target_output_dirs TARGET_NAME LIB_OUTPUT_DIR DLL_OUTPUT_DIR)
	if(PL_DO_NOT_SET_OUTPUT_DIRS)
		return()
	endif()

	pl_pull_output_vars("${LIB_OUTPUT_DIR}" "${DLL_OUTPUT_DIR}")

	# If we can't use generator expressions the non-generator expression version of the
	# output directory should point to the version matching CMAKE_BUILD_TYPE. This is the case for
	# add_custom_command BYPRODUCTS for example needed by Ninja.
	if("${CMAKE_BUILD_TYPE}" STREQUAL ${PL_BUILDTYPENAME_DEBUG})
		set_target_properties(${TARGET_NAME} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEBUG}"
			LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEBUG}"
			ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_LIB_DEBUG}"
		)
	elseif("${CMAKE_BUILD_TYPE}" STREQUAL ${PL_BUILDTYPENAME_RELEASE})
		set_target_properties(${TARGET_NAME} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DLL_RELEASE}"
			LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DLL_RELEASE}"
			ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_LIB_RELEASE}"
		)
	elseif("${CMAKE_BUILD_TYPE}" STREQUAL ${PL_BUILDTYPENAME_DEV})
		set_target_properties(${TARGET_NAME} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEV}"
			LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEV}"
			ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_LIB_DEV}"
		)
	else()
		message(FATAL_ERROR "Unknown CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")
	endif()

	set_target_properties(${TARGET_NAME} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY_${PL_BUILDTYPENAME_DEBUG_UPPER} "${OUTPUT_DLL_DEBUG}"
		LIBRARY_OUTPUT_DIRECTORY_${PL_BUILDTYPENAME_DEBUG_UPPER} "${OUTPUT_DLL_DEBUG}"
		ARCHIVE_OUTPUT_DIRECTORY_${PL_BUILDTYPENAME_DEBUG_UPPER} "${OUTPUT_LIB_DEBUG}"
	)

	set_target_properties(${TARGET_NAME} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY_${PL_BUILDTYPENAME_RELEASE_UPPER} "${OUTPUT_DLL_RELEASE}"
		LIBRARY_OUTPUT_DIRECTORY_${PL_BUILDTYPENAME_RELEASE_UPPER} "${OUTPUT_DLL_RELEASE}"
		ARCHIVE_OUTPUT_DIRECTORY_${PL_BUILDTYPENAME_RELEASE_UPPER} "${OUTPUT_LIB_RELEASE}"
	)

	set_target_properties(${TARGET_NAME} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY_${PL_BUILDTYPENAME_DEV_UPPER} "${OUTPUT_DLL_DEV}"
		LIBRARY_OUTPUT_DIRECTORY_${PL_BUILDTYPENAME_DEV_UPPER} "${OUTPUT_DLL_DEV}"
		ARCHIVE_OUTPUT_DIRECTORY_${PL_BUILDTYPENAME_DEV_UPPER} "${OUTPUT_LIB_DEV}"
	)
endfunction()

# #####################################
# ## pl_set_default_target_output_dirs(<target>)
# #####################################
function(pl_set_default_target_output_dirs TARGET_NAME)
	pl_set_target_output_dirs("${TARGET_NAME}" "${PL_OUTPUT_DIRECTORY_LIB}" "${PL_OUTPUT_DIRECTORY_DLL}")
endfunction()

# #####################################
# ## pl_write_configuration_txt()
# #####################################
function(pl_write_configuration_txt)
	if(PL_NO_TXT_FILES)
		return()
	endif()
	# Clear Targets.txt and Tests.txt
	file(WRITE ${CMAKE_BINARY_DIR}/Targets.txt "")
	file(WRITE ${CMAKE_BINARY_DIR}/Tests.txt "")

	pl_pull_all_vars()
	pl_pull_config_vars()

	# Write configuration to file, as this is done at configure time we must pin the configuration in place (Dev is used because all build machines use this).
	file(WRITE ${CMAKE_BINARY_DIR}/Configuration.txt "")
	set(CONFIGURATION_DESC "${PL_CMAKE_PLATFORM_PREFIX}${PL_CMAKE_GENERATOR_PREFIX}${PL_CMAKE_COMPILER_POSTFIX}${PL_BUILDTYPENAME_DEV}${PL_CMAKE_ARCHITECTURE_POSTFIX}")
	file(APPEND ${CMAKE_BINARY_DIR}/Configuration.txt ${CONFIGURATION_DESC})
endfunction()

# #####################################
# ## pl_add_target_folder_as_include_dir(<target> <path-to-target>)
# #####################################
function(pl_add_target_folder_as_include_dir TARGET_NAME TARGET_FOLDER)
	get_filename_component(PARENT_DIR ${TARGET_FOLDER} DIRECTORY)

	# target_include_directories(${TARGET_NAME} PRIVATE "${TARGET_FOLDER}")
	target_include_directories(${TARGET_NAME} PUBLIC "${PARENT_DIR}")
endfunction()

# #####################################
# ## pl_set_common_target_definitions(<target>)
# #####################################
function(pl_set_common_target_definitions TARGET_NAME)
	pl_pull_all_vars()
	pl_pull_config_vars()

	# set the BUILDSYSTEM_COMPILE_ENGINE_AS_DLL definition
	if(PL_COMPILE_ENGINE_AS_DLL)
		target_compile_definitions(${TARGET_NAME} PUBLIC BUILDSYSTEM_COMPILE_ENGINE_AS_DLL)
	endif()

	target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_SDKVERSION_MAJOR=${PL_CMAKE_SDKVERSION_MAJOR})
	target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_SDKVERSION_MINOR=${PL_CMAKE_SDKVERSION_MINOR})
	target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_SDKVERSION_PATCH=${PL_CMAKE_SDKVERSION_PATCH})

	set(ORIGINAL_BUILD_TYPE "$<IF:$<STREQUAL:${PL_CMAKE_GENERATOR_CONFIGURATION},${PL_BUILDTYPENAME_DEBUG}>,Debug,$<IF:$<STREQUAL:${PL_CMAKE_GENERATOR_CONFIGURATION},${PL_BUILDTYPENAME_DEV}>,Dev,Shipping>>")

	# set the BUILDSYSTEM_BUILDTYPE definition
	target_compile_definitions(${TARGET_NAME} PRIVATE "BUILDSYSTEM_BUILDTYPE=\"${ORIGINAL_BUILD_TYPE}\"")
	target_compile_definitions(${TARGET_NAME} PUBLIC "BUILDSYSTEM_BUILDTYPE_${ORIGINAL_BUILD_TYPE}")

	# set the BUILDSYSTEM_BUILDING_XYZ_LIB definition
	string(TOUPPER ${TARGET_NAME} PROJECT_NAME_UPPER)
	target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_BUILDING_${PROJECT_NAME_UPPER}_LIB)

	if(PL_BUILD_EXPERIMENTAL_VULKAN)
		target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_ENABLE_VULKAN_SUPPORT)
	endif()

	# on Windows, make sure to use the Unicode API
	target_compile_definitions(${TARGET_NAME} PUBLIC UNICODE _UNICODE)
endfunction()

# #####################################
# ## pl_set_project_ide_folder(<target> <path-to-target>)
# #####################################
function(pl_set_project_ide_folder TARGET_NAME PROJECT_SOURCE_DIR)
	# globally enable sorting targets into folders in IDEs
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	get_filename_component(PARENT_FOLDER ${PROJECT_SOURCE_DIR} PATH)
	get_filename_component(FOLDER_NAME ${PARENT_FOLDER} NAME)

	set(IDE_FOLDER "${FOLDER_NAME}")

	set(CMAKE_SOURCE_DIR_PREFIX "${CMAKE_SOURCE_DIR}/")
	cmake_path(IS_PREFIX CMAKE_SOURCE_DIR_PREFIX ${PROJECT_SOURCE_DIR} NORMALIZE FOLDER_IN_TREE)
	if(FOLDER_IN_TREE)
		set(IDE_FOLDER "")
		string(REPLACE ${CMAKE_SOURCE_DIR_PREFIX} "" PARENT_FOLDER ${PROJECT_SOURCE_DIR})

		get_filename_component(PARENT_FOLDER "${PARENT_FOLDER}" PATH)
		get_filename_component(FOLDER_NAME "${PARENT_FOLDER}" NAME)

		get_filename_component(PARENT_FOLDER2 "${PARENT_FOLDER}" PATH)

		while(NOT ${PARENT_FOLDER2} STREQUAL "")
			set(IDE_FOLDER "${FOLDER_NAME}/${IDE_FOLDER}")

			get_filename_component(PARENT_FOLDER "${PARENT_FOLDER}" PATH)
			get_filename_component(FOLDER_NAME "${PARENT_FOLDER}" NAME)

			get_filename_component(PARENT_FOLDER2 "${PARENT_FOLDER}" PATH)
		endwhile()
	endif()

	get_property(PL_SUBMODULE_MODE GLOBAL PROPERTY PL_SUBMODULE_MODE)

	if(PL_SUBMODULE_MODE)
		set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "plEngine/${IDE_FOLDER}")
	else()
		set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER ${IDE_FOLDER})
	endif()
endfunction()

# #####################################
# ## pl_add_output_pl_prefix(<target>)
# #####################################
function(pl_add_output_pl_prefix TARGET_NAME)
	set_target_properties(${TARGET_NAME} PROPERTIES IMPORT_PREFIX "pl")
	set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "pl")
endfunction()

# #####################################
# ## pl_make_winmain_executable(<target>)
# #####################################
function(pl_make_winmain_executable TARGET_NAME)
	set_property(TARGET ${TARGET_NAME} PROPERTY WIN32_EXECUTABLE ON)
endfunction()

# #####################################
# ## pl_gather_subfolders(<abs-path-to-folder> <out-sub-folders>)
# #####################################
function(pl_gather_subfolders START_FOLDER RESULT_FOLDERS)
	set(ALL_FILES "")
	set(ALL_DIRS "")

	file(GLOB_RECURSE ALL_FILES RELATIVE "${START_FOLDER}" "${START_FOLDER}/*")

	foreach(FILE ${ALL_FILES})
		get_filename_component(FILE_PATH ${FILE} DIRECTORY)

		list(APPEND ALL_DIRS ${FILE_PATH})
	endforeach()

	list(REMOVE_DUPLICATES ALL_DIRS)

	set(${RESULT_FOLDERS} ${ALL_DIRS} PARENT_SCOPE)
endfunction()

# #####################################
# ## pl_glob_source_files(<path-to-folder> <out-files>)
# #####################################
function(pl_glob_source_files ROOT_DIR RESULT_ALL_SOURCES)
	file(GLOB_RECURSE RELEVANT_FILES
		"${ROOT_DIR}/*.cpp"
		"${ROOT_DIR}/*.cc"
		"${ROOT_DIR}/*.h"
		"${ROOT_DIR}/*.hpp"
		"${ROOT_DIR}/*.inl"
		"${ROOT_DIR}/*.c"
		"${ROOT_DIR}/*.cs"
		"${ROOT_DIR}/*.ui"
		"${ROOT_DIR}/*.qrc"
		"${ROOT_DIR}/*.def"
		"${ROOT_DIR}/*.ico"
		"${ROOT_DIR}/*.rc"
		"${ROOT_DIR}/*.s"
		"${ROOT_DIR}/*.cmake"
		"${ROOT_DIR}/*.natvis"
		"${ROOT_DIR}/*.txt"
		"${ROOT_DIR}/*.ddl"
		"${ROOT_DIR}/*.plPermVar"
		"${ROOT_DIR}/*.plShader"
		"${ROOT_DIR}/*.plShaderTemplate"
	)

	set(${RESULT_ALL_SOURCES} ${RELEVANT_FILES} PARENT_SCOPE)
endfunction()

# #####################################
# ## pl_add_all_subdirs()
# #####################################
function(pl_add_all_subdirs)
	# find all cmake files below this directory
	file(GLOB SUB_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")

	foreach(VAR ${SUB_DIRS})
		get_filename_component(RES ${VAR} DIRECTORY)

		add_subdirectory(${RES})
	endforeach()
endfunction()

# #####################################
# ## pl_cmake_init()
# #####################################
macro(pl_cmake_init)
	pl_pull_all_vars()
endmacro()

# #####################################
# ## pl_requires(<variable>)
# #####################################
macro(pl_requires)
	if(${ARGC} EQUAL 0)
		return()
	endif()

	set(ALL_ARGS "${ARGN}")

	foreach(arg IN LISTS ALL_ARGS)
		if(NOT ${arg})
			return()
		endif()
	endforeach()
endmacro()

# #####################################
# ## pl_requires_one_of(<variable1> (<variable2>) (<variable3>) ...)
# #####################################
macro(pl_requires_one_of)
	if(${ARGC} EQUAL 0)
		message(FATAL_ERROR "pl_requires_one_of needs at least one argument")
	endif()

	set(ALL_ARGS "${ARGN}")

	set(VALID 0)

	foreach(arg IN LISTS ALL_ARGS)
		if(${arg})
			set(VALID 1)
		endif()
	endforeach()

	if(NOT VALID)
		return()
	endif()
endmacro()

# #####################################
# ## pl_requires_editor()
# #####################################
macro(pl_requires_editor)
	pl_requires_qt()
	pl_requires_renderer()
	if(NOT PL_CMAKE_PLATFORM_SUPPORTS_EDITOR)
		return()
	endif()
endmacro()

# #####################################
# ## pl_add_external_folder(<project-number>)
# #####################################
function(pl_add_external_projects_folder PROJECT_NUMBER)
	set(CACHE_VAR_NAME "PL_EXTERNAL_PROJECT${PROJECT_NUMBER}")

	set(${CACHE_VAR_NAME} "" CACHE PATH "A folder outside the pl repository that should be parsed for CMakeLists.txt files to include projects into the pl solution.")

	set(CACHE_VAR_VALUE ${${CACHE_VAR_NAME}})

	if(NOT CACHE_VAR_VALUE)
		return()
	endif()

	set_property(GLOBAL PROPERTY "GATHER_EXTERNAL_PROJECTS" TRUE)
	add_subdirectory(${CACHE_VAR_VALUE} "${CMAKE_BINARY_DIR}/ExternalProject${PROJECT_NUMBER}")
	set_property(GLOBAL PROPERTY "GATHER_EXTERNAL_PROJECTS" FALSE)
endfunction()

# #####################################
# ## pl_init_projects()
# #####################################
# By defining PL_SOURCE_DIR before calling this function
# you can change the location that will be scanned for projects.
function(pl_init_projects)
	# find all init.cmake files below this directory or the given source directory if any.
	if(PL_SOURCE_DIR)
		file(GLOB_RECURSE INIT_FILES "${PL_SOURCE_DIR}/init.cmake")
	else()
		file(GLOB_RECURSE INIT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/init.cmake")
	endif()

	foreach(INIT_FILE ${INIT_FILES})
		message(STATUS "Including '${INIT_FILE}'")
		include("${INIT_FILE}")
	endforeach()
endfunction()

# #####################################
# ## pl_finalize_projects()
# #####################################
# By defining PL_SOURCE_DIR before calling this function
# you can change the location that will be scanned for projects.
function(pl_finalize_projects)
	# find all finalize.cmake files below this directory or the given source directory if any.
	if(PL_SOURCE_DIR)
		file(GLOB_RECURSE FINALIZE_FILES "${PL_SOURCE_DIR}/finalize.cmake")
	else()
		file(GLOB_RECURSE FINALIZE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/finalize.cmake")
	endif()

	# TODO: also finalize external projects
	foreach(FINALIZE_FILE ${FINALIZE_FILES})
		message(STATUS "Including '${FINALIZE_FILE}'")
		include("${FINALIZE_FILE}")
	endforeach()
endfunction()

# #####################################
# ## pl_build_filter_init()
# #####################################

# The build filter is intended to only build a subset of plEngine.
# The build filters are configured through cmake files in the 'BuildFilters' directory.
function(pl_build_filter_init)
	file(GLOB_RECURSE FILTER_FILES "${PL_ROOT}/Code/BuildSystem/CMake/BuildFilters/*.BuildFilter")

	get_property(PL_BUILD_FILTER_NAMES GLOBAL PROPERTY PL_BUILD_FILTER_NAMES)

	foreach(VAR ${FILTER_FILES})
		cmake_path(GET VAR STEM FILTER_NAME)
		list(APPEND PL_BUILD_FILTER_NAMES "${FILTER_NAME}")

		message(STATUS "Reading build filter '${FILTER_NAME}'")
		include(${VAR})
	endforeach()

	list(REMOVE_DUPLICATES PL_BUILD_FILTER_NAMES)
	set_property(GLOBAL PROPERTY PL_BUILD_FILTER_NAMES ${PL_BUILD_FILTER_NAMES})

	set(PL_BUILD_FILTER "Everything" CACHE STRING "Which projects to include in the solution.")

	get_property(PL_BUILD_FILTER_NAMES GLOBAL PROPERTY PL_BUILD_FILTER_NAMES)
	set_property(CACHE PL_BUILD_FILTER PROPERTY STRINGS ${PL_BUILD_FILTER_NAMES})
	set_property(GLOBAL PROPERTY PL_BUILD_FILTER_SELECTED ${PL_BUILD_FILTER})
endfunction()

# #####################################
# ## pl_project_build_filter_index(<PROJECT_NAME> <OUT_INDEX>)
# #####################################
function(pl_project_build_filter_index PROJECT_NAME OUT_INDEX)
	get_property(SELECTED_FILTER_NAME GLOBAL PROPERTY PL_BUILD_FILTER_SELECTED)
	set(FILTER_VAR_NAME "PL_BUILD_FILTER_${SELECTED_FILTER_NAME}")
	get_property(FILTER_PROJECTS GLOBAL PROPERTY ${FILTER_VAR_NAME})

	list(LENGTH FILTER_PROJECTS LIST_LENGTH)

	if(${LIST_LENGTH} GREATER 1)
		list(FIND FILTER_PROJECTS ${PROJECT_NAME} FOUND_INDEX)
		set(${OUT_INDEX} ${FOUND_INDEX} PARENT_SCOPE)
	else()
		set(${OUT_INDEX} 0 PARENT_SCOPE)
	endif()
endfunction()

# #####################################
# ## pl_apply_build_filter(<PROJECT_NAME>)
# #####################################
macro(pl_apply_build_filter PROJECT_NAME)
	pl_project_build_filter_index(${PROJECT_NAME} PROJECT_INDEX)

	if(${PROJECT_INDEX} EQUAL -1)
		get_property(SELECTED_FILTER_NAME GLOBAL PROPERTY PL_BUILD_FILTER_SELECTED)
		message(STATUS "Project '${PROJECT_NAME}' excluded by build filter '${SELECTED_FILTER_NAME}'.")
		return()
	endif()
endmacro()

# #####################################
# ## pl_set_build_types()
# #####################################
function(pl_set_build_types)
	pl_pull_config_vars()

	set(CMAKE_CONFIGURATION_TYPES "${PL_BUILDTYPENAME_DEBUG};${PL_BUILDTYPENAME_DEV};${PL_BUILDTYPENAME_RELEASE}" CACHE STRING "" FORCE)

	if (PL_BUILDTYPE_ONLY)
		set(CMAKE_CONFIGURATION_TYPES "${PL_BUILDTYPE_ONLY}" CACHE STRING "" FORCE)
	endif()

	set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "PL build config types" FORCE)

	set(CMAKE_BUILD_TYPE ${PL_BUILDTYPENAME_DEV} CACHE STRING "The default build type")
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES})

	set(CMAKE_EXE_LINKER_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_EXE_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_EXE_LINKER_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER} ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_EXE_LINKER_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_EXE_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

	set(CMAKE_SHARED_LINKER_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_SHARED_LINKER_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER} ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_SHARED_LINKER_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_SHARED_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

	set(CMAKE_STATIC_LINKER_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_STATIC_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_STATIC_LINKER_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER} ${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_STATIC_LINKER_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_STATIC_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

	set(CMAKE_MODULE_LINKER_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_MODULE_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_MODULE_LINKER_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER} ${CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_MODULE_LINKER_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_MODULE_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

	# Fix for cl : Command line warning D9025 : overriding '/Ob0' with '/Ob1'
	# We are adding /Ob1 to debug inside ./CMakeUtils/plUtilsCppFlags.cmake
	string(REPLACE "/Ob0" "/Ob1" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
	string(REPLACE "/Ob0" "/Ob1" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

	set(CMAKE_CXX_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_CXX_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_CXX_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER} ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_CXX_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_CXX_FLAGS_RELEASE} CACHE STRING "" FORCE)

	set(CMAKE_C_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_C_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_C_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER} ${CMAKE_C_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_C_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_C_FLAGS_RELEASE} CACHE STRING "" FORCE)

	set(CMAKE_CSharp_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_CSharp_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_CSharp_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER} ${CMAKE_CSharp_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_CSharp_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_CSharp_FLAGS_RELEASE} CACHE STRING "" FORCE)

	set(CMAKE_RC_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER} ${CMAKE_RC_FLAGS_DEBUG} CACHE STRING "" FORCE)
	set(CMAKE_RC_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER} ${CMAKE_RC_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
	set(CMAKE_RC_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER} ${CMAKE_RC_FLAGS_RELEASE} CACHE STRING "" FORCE)

	mark_as_advanced(FORCE CMAKE_EXE_LINKER_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_EXE_LINKER_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_EXE_LINKER_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER})
	mark_as_advanced(FORCE CMAKE_SHARED_LINKER_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_SHARED_LINKER_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_SHARED_LINKER_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER})
	mark_as_advanced(FORCE CMAKE_STATIC_LINKER_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_STATIC_LINKER_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_STATIC_LINKER_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER})
	mark_as_advanced(FORCE CMAKE_MODULE_LINKER_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_MODULE_LINKER_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_MODULE_LINKER_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER})
	mark_as_advanced(FORCE CMAKE_CXX_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_CXX_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_CXX_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER})
	mark_as_advanced(FORCE CMAKE_C_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_C_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_C_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER})
	mark_as_advanced(FORCE CMAKE_CSharp_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_CSharp_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_CSharp_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER})
	mark_as_advanced(FORCE CMAKE_RC_FLAGS_${PL_BUILDTYPENAME_DEBUG_UPPER})
	mark_as_advanced(FORCE CMAKE_RC_FLAGS_${PL_BUILDTYPENAME_DEV_UPPER})
	mark_as_advanced(FORCE CMAKE_RC_FLAGS_${PL_BUILDTYPENAME_RELEASE_UPPER})
endfunction()

# #####################################
# ## pl_download_and_extract(<url-to-download> <dest-folder-path> <dest-filename-without-extension>)
# #####################################
function(pl_download_and_extract URL DEST_FOLDER DEST_FILENAME)
	if(${URL} MATCHES ".tar.gz$")
		set(PKG_TYPE "tar.gz")
	elseif(${URL} MATCHES ".tar.xz$")
		set(PKG_TYPE "tar.xz")
	else()
		get_filename_component(PKG_TYPE ${URL} LAST_EXT)
	endif()

	set(FULL_FILENAME "${DEST_FILENAME}.${PKG_TYPE}")
	set(PKG_FILE "${DEST_FOLDER}/${FULL_FILENAME}")
	set(EXTRACT_MARKER "${PKG_FILE}.extracted")

	if(EXISTS "${EXTRACT_MARKER}")
		return()
	endif()

	# if the "URL" is actually a file path
	if(NOT "${URL}" MATCHES "http*")
		set(PKG_FILE "${URL}")
	endif()

	if(NOT EXISTS "${PKG_FILE}")
		message(STATUS "Downloading '${FULL_FILENAME}'...")
		file(DOWNLOAD ${URL} "${PKG_FILE}" SHOW_PROGRESS STATUS DOWNLOAD_STATUS)

		list(GET DOWNLOAD_STATUS 0 DOWNLOAD_STATUS_CODE)

		if(NOT DOWNLOAD_STATUS_CODE EQUAL 0)
			message(FATAL_ERROR "Download failed: ${DOWNLOAD_STATUS}")
			return()
		endif()
	endif()

	pl_pull_config_vars()

	message(STATUS "Extracting '${FULL_FILENAME}'...")

	if(${PKG_TYPE} MATCHES "7z")
		set(FULL_7ZA_PATH "${PL_ROOT}/${PL_CONFIG_PATH_7ZA}")
		execute_process(COMMAND "${FULL_7ZA_PATH}"
			x "${PKG_FILE}"
			-aoa
			WORKING_DIRECTORY "${DEST_FOLDER}"
			COMMAND_ERROR_IS_FATAL ANY
			RESULT_VARIABLE CMD_STATUS)
	else()
		execute_process(COMMAND ${CMAKE_COMMAND}
			-E tar -xf "${PKG_FILE}"
			WORKING_DIRECTORY "${DEST_FOLDER}"
			COMMAND_ERROR_IS_FATAL ANY
			RESULT_VARIABLE CMD_STATUS)
	endif()

	if(NOT CMD_STATUS EQUAL 0)
		message(FATAL_ERROR "Extracting package '${FULL_FILENAME}' failed.")
		return()
	endif()

	file(TOUCH ${EXTRACT_MARKER})
endfunction()

function(pl_get_export_location DST_VAR)
	pl_pull_config_vars()
	pl_pull_output_vars("" "${PL_OUTPUT_DIRECTORY_DLL}")

	if(GENERATOR_IS_MULTI_CONFIG OR (CMAKE_GENERATOR MATCHES "Visual Studio"))
		set("${DST_VAR}" "${PL_OUTPUT_DIRECTORY_DLL}/plExport.cmake" PARENT_SCOPE)
	else()
		if("${CMAKE_BUILD_TYPE}" STREQUAL ${PL_BUILDTYPENAME_DEBUG})
			set("${DST_VAR}" "${PL_OUTPUT_DIRECTORY_DLL}/${OUTPUT_DEBUG}/plExport.cmake" PARENT_SCOPE)
		elseif("${CMAKE_BUILD_TYPE}" STREQUAL ${PL_BUILDTYPENAME_RELEASE})
			set("${DST_VAR}" "${PL_OUTPUT_DIRECTORY_DLL}/${OUTPUT_RELEASE}/plExport.cmake" PARENT_SCOPE)
		elseif("${CMAKE_BUILD_TYPE}" STREQUAL ${PL_BUILDTYPENAME_DEV})
			set("${DST_VAR}" "${PL_OUTPUT_DIRECTORY_DLL}/${OUTPUT_DEV}/plExport.cmake" PARENT_SCOPE)
		else()
			message(FATAL_ERROR "Unknown CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")
		endif()
	endif()
endfunction()
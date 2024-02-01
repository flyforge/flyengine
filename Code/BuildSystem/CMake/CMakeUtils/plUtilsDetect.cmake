# #####################################
# ## pl_detect_project_name(<out-name>)
# #####################################

function(pl_detect_project_name OUT_NAME)
	# unfortunately this has to be known before the PROJECT command,
	# but platform and compiler settings are only detected by CMake AFTER the project command
	# CMAKE_GENERATOR is the only value available before that, so we have to regex this a bit to
	# generate a useful name
	# thus, only VS solutions currently get nice names
	cmake_path(IS_PREFIX CMAKE_SOURCE_DIR ${CMAKE_BINARY_DIR} NORMALIZE IS_IN_SOURCE_BUILD)

	get_filename_component(NAME_REPO ${CMAKE_SOURCE_DIR} NAME)
	get_filename_component(NAME_DEST ${CMAKE_BINARY_DIR} NAME)

	set(DETECTED_NAME "${NAME_REPO}")

	if(NOT ${NAME_REPO} STREQUAL ${NAME_DEST})
		set(DETECTED_NAME "${DETECTED_NAME}_${NAME_DEST}")
	endif()

	set(${OUT_NAME} "${DETECTED_NAME}" PARENT_SCOPE)

	message(STATUS "Auto-detected solution name: ${DETECTED_NAME} (Generator = ${CMAKE_GENERATOR})")
endfunction()

# #####################################
# ## pl_pull_platform_vars()
# #####################################
macro(pl_pull_platform_vars)

	get_property(PL_CMAKE_PLATFORM_NAME GLOBAL PROPERTY PL_CMAKE_PLATFORM_NAME)
	get_property(PL_CMAKE_PLATFORM_PREFIX GLOBAL PROPERTY PL_CMAKE_PLATFORM_PREFIX)
	get_property(PL_CMAKE_PLATFORM_POSTFIX GLOBAL PROPERTY PL_CMAKE_PLATFORM_POSTFIX)
	get_property(PL_CMAKE_PLATFORM_POSIX GLOBAL PROPERTY PL_CMAKE_PLATFORM_POSIX)
	get_property(PL_CMAKE_PLATFORM_SUPPORTS_VULKAN GLOBAL PROPERTY PL_CMAKE_PLATFORM_SUPPORTS_VULKAN)
	get_property(PL_CMAKE_PLATFORM_SUPPORTS_EDITOR GLOBAL PROPERTY PL_CMAKE_PLATFORM_SUPPORTS_EDITOR)

	pl_platform_pull_properties()
endmacro()

# #####################################
# ## pl_detect_generator()
# #####################################
function(pl_detect_generator)
	get_property(PREFIX GLOBAL PROPERTY PL_CMAKE_GENERATOR_PREFIX)

	if(PREFIX)
		# has already run before and PL_CMAKE_GENERATOR_PREFIX is already set
		# message (STATUS "Redundant call to pl_detect_generator()")
		return()
	endif()

	pl_pull_platform_vars()

	set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_PREFIX "")
	set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_CONFIGURATION "undefined")
	set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_MSVC OFF)
	set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_XCODE OFF)
	set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_MAKE OFF)
	set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_NINJA OFF)
	set_property(GLOBAL PROPERTY PL_CMAKE_INSIDE_VS OFF) # if cmake is called through the visual studio open folder workflow

	message(STATUS "CMAKE_VERSION is '${CMAKE_VERSION}'")
	message(STATUS "CMAKE_BUILD_TYPE is '${CMAKE_BUILD_TYPE}'")
	message(STATUS "CMAKE_GENERATOR is '${CMAKE_GENERATOR}'")

	pl_platform_detect_generator()

endfunction()

# #####################################
# ## pl_pull_generator_vars()
# #####################################
macro(pl_pull_generator_vars)
	pl_detect_generator()

	get_property(PL_CMAKE_GENERATOR_PREFIX GLOBAL PROPERTY PL_CMAKE_GENERATOR_PREFIX)
	get_property(PL_CMAKE_GENERATOR_CONFIGURATION GLOBAL PROPERTY PL_CMAKE_GENERATOR_CONFIGURATION)
	get_property(PL_CMAKE_GENERATOR_MSVC GLOBAL PROPERTY PL_CMAKE_GENERATOR_MSVC)
	get_property(PL_CMAKE_GENERATOR_XCODE GLOBAL PROPERTY PL_CMAKE_GENERATOR_XCODE)
	get_property(PL_CMAKE_GENERATOR_MAKE GLOBAL PROPERTY PL_CMAKE_GENERATOR_MAKE)
	get_property(PL_CMAKE_GENERATOR_NINJA GLOBAL PROPERTY PL_CMAKE_GENERATOR_NINJA)
	get_property(PL_CMAKE_INSIDE_VS GLOBAL PROPERTY PL_CMAKE_INSIDE_VS)
endmacro()

# #####################################
# ## pl_detect_compiler_and_architecture()
# #####################################
function(pl_detect_compiler_and_architecture)
	get_property(PREFIX GLOBAL PROPERTY PL_CMAKE_COMPILER_POSTFIX)

	if(PREFIX)
		# has already run before and PL_CMAKE_COMPILER_POSTFIX is already set
		# message (STATUS "Redundant call to pl_detect_compiler()")
		return()
	endif()

	pl_pull_platform_vars()
	pl_pull_generator_vars()
	pl_pull_config_vars()
	get_property(GENERATOR_MSVC GLOBAL PROPERTY PL_CMAKE_GENERATOR_MSVC)

	set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_POSTFIX "")
	set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC OFF)
	set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC_140 OFF)
	set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC_141 OFF)
	set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC_142 OFF)
	set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC_143 OFF)
	set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_CLANG OFF)
	set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_GCC OFF)

	set(FILE_TO_COMPILE "${PL_ROOT}/${PL_CMAKE_RELPATH}/ProbingSrc/ArchitectureDetect.c")
	
	if (PL_SDK_DIR)
		set(FILE_TO_COMPILE "${PL_SDK_DIR}/${PL_CMAKE_RELPATH}/ProbingSrc/ArchitectureDetect.c")
	endif()

	# Only compile the detect file if we don't have a cached result from the last run
	if((NOT PL_DETECTED_COMPILER) OR (NOT PL_DETECTED_ARCH) OR (NOT PL_DETECTED_MSVC_VER))
		set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
		try_compile(COMPILE_RESULT
			${CMAKE_CURRENT_BINARY_DIR}
			${FILE_TO_COMPILE}
			OUTPUT_VARIABLE COMPILE_OUTPUT
		)

		if(NOT COMPILE_RESULT)
			message(FATAL_ERROR "Failed to detect compiler / target architecture. Compiler output: ${COMPILE_OUTPUT}")
		endif()

		if(${COMPILE_OUTPUT} MATCHES "ARCH:'([^']*)'")
			set(PL_DETECTED_ARCH ${CMAKE_MATCH_1} CACHE INTERNAL "")
		else()
			message(FATAL_ERROR "The compile test did not output the architecture. Compiler broken? Compiler output: ${COMPILE_OUTPUT}")
		endif()

		if(${COMPILE_OUTPUT} MATCHES "COMPILER:'([^']*)'")
			set(PL_DETECTED_COMPILER ${CMAKE_MATCH_1} CACHE INTERNAL "")
		else()
			message(FATAL_ERROR "The compile test did not output the compiler. Compiler broken? Compiler output: ${COMPILE_OUTPUT}")
		endif()
		
		if(PL_DETECTED_COMPILER STREQUAL "msvc")
			if(${COMPILE_OUTPUT} MATCHES "MSC_VER:'([^']*)'")
				set(PL_DETECTED_MSVC_VER ${CMAKE_MATCH_1} CACHE INTERNAL "")
			else()
				message(FATAL_ERROR "The compile test did not output the MSC_VER. Compiler broken? Compiler output: ${COMPILE_OUTPUT}")
			endif()
		else()
			set(PL_DETECTED_MSVC_VER "<NOT USING MSVC>" CACHE INTERNAL "")
		endif()
	endif()

	if(PL_DETECTED_COMPILER STREQUAL "msvc") # Visual Studio Compiler
		message(STATUS "Compiler is MSVC (PL_CMAKE_COMPILER_MSVC) version ${PL_DETECTED_MSVC_VER}")

		set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC ON)

		if(PL_DETECTED_MSVC_VER GREATER_EQUAL 1930)
			message(STATUS "Compiler is Visual Studio 2022 (PL_CMAKE_COMPILER_MSVC_143)")
			set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC_143 ON)
			set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_POSTFIX "2022")

		elseif(PL_DETECTED_MSVC_VER GREATER_EQUAL 1920)
			message(STATUS "Compiler is Visual Studio 2019 (PL_CMAKE_COMPILER_MSVC_142)")
			set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC_142 ON)
			set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_POSTFIX "2019")

		elseif(PL_DETECTED_MSVC_VER GREATER_EQUAL 1910)
			message(STATUS "Compiler is Visual Studio 2017 (PL_CMAKE_COMPILER_MSVC_141)")
			set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC_141 ON)
			set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_POSTFIX "2017")

		elseif(MSVC_VERSION GREATER_EQUAL 1900)
			message(STATUS "Compiler is Visual Studio 2015 (PL_CMAKE_COMPILER_MSVC_140)")
			set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC_140 ON)
			set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_POSTFIX "2015")

		else()
			message(FATAL_ERROR "Compiler for generator '${CMAKE_GENERATOR}' is not supported on MSVC! Please extend pl_detect_compiler()")
		endif()

	elseif(PL_DETECTED_COMPILER STREQUAL "clang")
		message(STATUS "Compiler is clang (PL_CMAKE_COMPILER_CLANG)")
		set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_CLANG ON)
		set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_POSTFIX "Clang")

	elseif(PL_DETECTED_COMPILER STREQUAL "gcc")
		message(STATUS "Compiler is gcc (PL_CMAKE_COMPILER_GCC)")
		set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_GCC ON)
		set_property(GLOBAL PROPERTY PL_CMAKE_COMPILER_POSTFIX "Gcc")

	else()
		message(FATAL_ERROR "Unhandled compiler ${PL_DETECTED_COMPILER}")
	endif()

	set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_POSTFIX "")
	set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_32BIT OFF)
	set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_64BIT OFF)
	set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_X86 OFF)
	set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_ARM OFF)
	set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_EMSCRIPTEN OFF)

	if(PL_DETECTED_ARCH STREQUAL "x86")
		message(STATUS "Architecture is X86 (PL_CMAKE_ARCHITECTURE_X86)")
		set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_X86 ON)

		message(STATUS "Architecture is 32-Bit (PL_CMAKE_ARCHITECTURE_32BIT)")
		set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_32BIT ON)

	elseif(PL_DETECTED_ARCH STREQUAL "x64")
		message(STATUS "Architecture is X86 (PL_CMAKE_ARCHITECTURE_X86)")
		set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_X86 ON)

		message(STATUS "Architecture is 64-Bit (PL_CMAKE_ARCHITECTURE_64BIT)")
		set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_64BIT ON)

	elseif(PL_DETECTED_ARCH STREQUAL "arm32")
		message(STATUS "Architecture is ARM (PL_CMAKE_ARCHITECTURE_ARM)")
		set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_ARM ON)

		message(STATUS "Architecture is 32-Bit (PL_CMAKE_ARCHITECTURE_32BIT)")
		set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_32BIT ON)

	elseif(PL_DETECTED_ARCH STREQUAL "arm64")
		message(STATUS "Architecture is ARM (PL_CMAKE_ARCHITECTURE_ARM)")
		set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_ARM ON)

		message(STATUS "Architecture is 64-Bit (PL_CMAKE_ARCHITECTURE_64BIT)")
		set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_64BIT ON)

	elseif(PL_DETECTED_ARCH STREQUAL "emscripten")
		message(STATUS "Architecture is WEBASSEMBLY (PL_CMAKE_ARCHITECTURE_WEBASSEMBLY)")
		set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_WEBASSEMBLY ON)

		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			message(STATUS "Architecture is 64-Bit (PL_CMAKE_ARCHITECTURE_64BIT)")
			set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_64BIT ON)
		else()
			message(STATUS "Architecture is 32-Bit (PL_CMAKE_ARCHITECTURE_32BIT)")
			set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_32BIT ON)
		endif()

	else()
		message(FATAL_ERROR "Unhandled target architecture ${PL_DETECTED_ARCH}")
	endif()

	get_property(PL_CMAKE_ARCHITECTURE_32BIT GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_32BIT)
	get_property(PL_CMAKE_ARCHITECTURE_ARM GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_ARM)

	if(PL_CMAKE_ARCHITECTURE_ARM)
		if(PL_CMAKE_ARCHITECTURE_32BIT)
			set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_POSTFIX "Arm32")
		else()
			set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_POSTFIX "Arm64")
		endif()
	else()
		if(PL_CMAKE_ARCHITECTURE_32BIT)
			set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_POSTFIX "32")
		else()
			set_property(GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_POSTFIX "64")
		endif()
	endif()
endfunction()

# #####################################
# ## pl_pull_compiler_vars()
# #####################################
macro(pl_pull_compiler_and_architecture_vars)
	pl_detect_compiler_and_architecture()

	get_property(PL_CMAKE_COMPILER_POSTFIX GLOBAL PROPERTY PL_CMAKE_COMPILER_POSTFIX)
	get_property(PL_CMAKE_COMPILER_MSVC GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC)
	get_property(PL_CMAKE_COMPILER_MSVC_140 GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC_140)
	get_property(PL_CMAKE_COMPILER_MSVC_141 GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC_141)
	get_property(PL_CMAKE_COMPILER_MSVC_142 GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC_142)
	get_property(PL_CMAKE_COMPILER_MSVC_143 GLOBAL PROPERTY PL_CMAKE_COMPILER_MSVC_143)
	get_property(PL_CMAKE_COMPILER_CLANG GLOBAL PROPERTY PL_CMAKE_COMPILER_CLANG)
	get_property(PL_CMAKE_COMPILER_GCC GLOBAL PROPERTY PL_CMAKE_COMPILER_GCC)

	get_property(PL_CMAKE_ARCHITECTURE_POSTFIX GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_POSTFIX)
	get_property(PL_CMAKE_ARCHITECTURE_32BIT GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_32BIT)
	get_property(PL_CMAKE_ARCHITECTURE_64BIT GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_64BIT)
	get_property(PL_CMAKE_ARCHITECTURE_X86 GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_X86)
	get_property(PL_CMAKE_ARCHITECTURE_ARM GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_ARM)
	get_property(PL_CMAKE_ARCHITECTURE_WEBASSEMBLY GLOBAL PROPERTY PL_CMAKE_ARCHITECTURE_WEBASSEMBLY)
endmacro()

# #####################################
# ## pl_pull_all_vars()
# #####################################
macro(pl_pull_all_vars)
	get_property(PL_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY PL_SUBMODULE_PREFIX_PATH)
	get_property(PL_ROOT GLOBAL PROPERTY PL_ROOT)

	pl_pull_version()
	pl_pull_compiler_and_architecture_vars()
	pl_pull_generator_vars()
	pl_pull_platform_vars()
endmacro()

# #####################################
# ## pl_get_version(<VERSIONFILE> <OUT_MAJOR> <OUT_MINOR> <OUT_PATCH>)
# #####################################
function(pl_get_version VERSIONFILE OUT_MAJOR OUT_MINOR OUT_PATCH)
	file(READ ${VERSIONFILE} VERSION_STRING)

	string(STRIP ${VERSION_STRING} VERSION_STRING)

	if(VERSION_STRING MATCHES "([0-9]+).([0-9]+).([0-9+])")
		STRING(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" VERSION_MAJOR "${VERSION_STRING}")
		STRING(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" VERSION_MINOR "${VERSION_STRING}")
		STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" VERSION_PATCH "${VERSION_STRING}")

		string(STRIP ${VERSION_MAJOR} VERSION_MAJOR)
		string(STRIP ${VERSION_MINOR} VERSION_MINOR)
		string(STRIP ${VERSION_PATCH} VERSION_PATCH)

		set(${OUT_MAJOR} ${VERSION_MAJOR} PARENT_SCOPE)
		set(${OUT_MINOR} ${VERSION_MINOR} PARENT_SCOPE)
		set(${OUT_PATCH} ${VERSION_PATCH} PARENT_SCOPE)

	else()
		message(FATAL_ERROR "Invalid version string '${VERSION_STRING}'")
	endif()
endfunction()

# #####################################
# ## pl_detect_version()
# #####################################
function(pl_detect_version)
	get_property(VERSION_MAJOR GLOBAL PROPERTY PL_CMAKE_SDKVERSION_MAJOR)

	if(VERSION_MAJOR)
		# has already run before and PL_CMAKE_SDKVERSION_MAJOR is already set
		return()
	endif()

	pl_get_version("${PL_ROOT}/version.txt" VERSION_MAJOR VERSION_MINOR VERSION_PATCH)

	set_property(GLOBAL PROPERTY PL_CMAKE_SDKVERSION_MAJOR "${VERSION_MAJOR}")
	set_property(GLOBAL PROPERTY PL_CMAKE_SDKVERSION_MINOR "${VERSION_MINOR}")
	set_property(GLOBAL PROPERTY PL_CMAKE_SDKVERSION_PATCH "${VERSION_PATCH}")

	message(STATUS "SDK version: Major = '${VERSION_MAJOR}', Minor = '${VERSION_MINOR}', Patch = '${VERSION_PATCH}'")
endfunction()

# #####################################
# ## pl_pull_version()
# #####################################
macro(pl_pull_version)
	pl_detect_version()

	get_property(PL_CMAKE_SDKVERSION_MAJOR GLOBAL PROPERTY PL_CMAKE_SDKVERSION_MAJOR)
	get_property(PL_CMAKE_SDKVERSION_MINOR GLOBAL PROPERTY PL_CMAKE_SDKVERSION_MINOR)
	get_property(PL_CMAKE_SDKVERSION_PATCH GLOBAL PROPERTY PL_CMAKE_SDKVERSION_PATCH)
endmacro()
include("${CMAKE_CURRENT_LIST_DIR}/Configure_Default.cmake")

message(STATUS "Configuring Platform: OSX")

set_property(GLOBAL PROPERTY PL_CMAKE_PLATFORM_OSX ON)
set_property(GLOBAL PROPERTY PL_CMAKE_PLATFORM_POSIX ON)

macro(pl_platform_pull_properties)

	get_property(PL_CMAKE_PLATFORM_OSX GLOBAL PROPERTY PL_CMAKE_PLATFORM_OSX)

endmacro()

macro (pl_platformhook_set_build_flags_clang)

    target_compile_options(${TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
	target_link_options(${TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)

endmacro()

macro(pl_platform_detect_generator)
	if(CMAKE_GENERATOR MATCHES "Xcode") # XCODE
		message(STATUS "Buildsystem is Xcode (PL_CMAKE_GENERATOR_XCODE)")

		set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_XCODE ON)
		set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_PREFIX "Xcode")
		set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_CONFIGURATION $<CONFIGURATION>)

	elseif(CMAKE_GENERATOR MATCHES "Unix Makefiles") # Unix Makefiles (for QtCreator etc.)
		message(STATUS "Buildsystem is Make (PL_CMAKE_GENERATOR_MAKE)")

		set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_MAKE ON)
		set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_PREFIX "Make")
		set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})

	else()
		message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on OS X! Please extend pl_platform_detect_generator()")
	endif()
endmacro()
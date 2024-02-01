include("${CMAKE_CURRENT_LIST_DIR}/Configure_Default.cmake")

message(STATUS "Configuring Platform: Linux")

set_property(GLOBAL PROPERTY PL_CMAKE_PLATFORM_LINUX ON)
set_property(GLOBAL PROPERTY PL_CMAKE_PLATFORM_POSIX ON)
set_property(GLOBAL PROPERTY PL_CMAKE_PLATFORM_SUPPORTS_VULKAN ON)

# #####################################
# ## General settings
# #####################################
set(PL_COMPILE_ENGINE_AS_DLL ON CACHE BOOL "Whether to compile the code as a shared libraries (DLL).")
mark_as_advanced(FORCE PL_COMPILE_ENGINE_AS_DLL)

# #####################################
# ## Experimental Editor support on Linux
# #####################################
set (PL_EXPERIMENTAL_EDITOR_ON_LINUX OFF CACHE BOOL "Wether or not to build the editor on linux")

if (PL_EXPERIMENTAL_EDITOR_ON_LINUX)
    set_property(GLOBAL PROPERTY PL_CMAKE_PLATFORM_SUPPORTS_EDITOR ON)
endif()

macro(pl_platform_pull_properties)

    get_property(PL_CMAKE_PLATFORM_LINUX GLOBAL PROPERTY PL_CMAKE_PLATFORM_LINUX)

endmacro()

macro(pl_platformhook_link_target_vulkan)

    # on linux is the loader a dll
    get_target_property(_dll_location PlVulkan::Loader IMPORTED_LOCATION)

    if(NOT _dll_location STREQUAL "")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:PlVulkan::Loader> $<TARGET_FILE_DIR:${TARGET_NAME}>)
    endif()

    unset(_dll_location)

endmacro()

macro(pl_platformhook_set_build_flags_clang)
	target_compile_options(${TARGET_NAME} PRIVATE -fPIC)

	# Look for the super fast ld compatible linker called "mold". If present we want to use it.
	find_program(MOLD_PATH "mold")

	# We want to use the llvm linker lld by default
	# Unless the user has specified a different linker
	get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)

	if("${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY")
		if(NOT("${CMAKE_EXE_LINKER_FLAGS}" MATCHES "fuse-ld="))
			if(MOLD_PATH)
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=${MOLD_PATH}")
			else()
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=lld")
			endif()
		endif()

		# Reporting missing symbols at linktime
		target_link_options(${TARGET_NAME} PRIVATE "-Wl,-z,defs")
	elseif("${TARGET_TYPE}" STREQUAL "EXECUTABLE")
		if(NOT("${CMAKE_SHARED_LINKER_FLAGS}" MATCHES "fuse-ld="))
			if(MOLD_PATH)
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=${MOLD_PATH}")
			else()
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=lld")
			endif()
		endif()

		# Reporting missing symbols at linktime
		target_link_options(${TARGET_NAME} PRIVATE "-Wl,-z,defs")
	endif()
endmacro()

macro(pl_platformhook_set_application_properties TARGET_NAME)

    # We need to link against pthread and rt last or linker errors will occur.
	target_link_libraries(${TARGET_NAME} PRIVATE pthread rt)

endmacro()

macro(pl_platform_detect_generator)
    if(CMAKE_GENERATOR MATCHES "Unix Makefiles") # Unix Makefiles (for QtCreator etc.)
        message(STATUS "Buildsystem is Make (PL_CMAKE_GENERATOR_MAKE)")

        set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_MAKE ON)
        set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_PREFIX "Make")
        set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})

    elseif(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
        message(STATUS "Buildsystem is Ninja (PL_CMAKE_GENERATOR_NINJA)")

        set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_NINJA ON)
        set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_PREFIX "Ninja")
        set_property(GLOBAL PROPERTY PL_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})
    else()
        message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Linux! Please extend pl_platform_detect_generator()")
    endif()
endmacro()


macro(pl_platformhook_set_library_properties TARGET_NAME)
    # c = libc.so (the C standard library)
    # m = libm.so (the C standard library math portion)
    # pthread = libpthread.so (thread support)
    # rt = librt.so (compiler runtime functions)
    target_link_libraries(${TARGET_NAME} PRIVATE pthread rt c m)

    get_property(PL_CMAKE_COMPILER_GCC GLOBAL PROPERTY PL_CMAKE_COMPILER_GCC)
    if(PL_CMAKE_COMPILER_GCC)
        # Workaround for: https://bugs.launchpad.net/ubuntu/+source/gcc-5/+bug/1568899
        target_link_libraries(${TARGET_NAME} PRIVATE -lgcc_s -lgcc)
    endif()
endmacro()

macro(pl_platformhook_find_vulkan)
    if(PL_CMAKE_ARCHITECTURE_64BIT)
        if((PL_VULKAN_DIR STREQUAL "PL_VULKAN_DIR-NOTFOUND") OR(PL_VULKAN_DIR STREQUAL ""))
            # set(CMAKE_FIND_DEBUG_MODE TRUE)
            unset(PL_VULKAN_DIR CACHE)
            unset(PlVulkan_DIR CACHE)
            find_path(PL_VULKAN_DIR config/vk_layer_settings.txt
                PATHS
                ${PL_VULKAN_DIR}
                $ENV{VULKAN_SDK}
            )

            if(PL_CMAKE_ARCHITECTURE_X86)
                if((PL_VULKAN_DIR STREQUAL "PL_VULKAN_DIR-NOTFOUND") OR (PL_VULKAN_DIR STREQUAL ""))
                    pl_download_and_extract("${PL_CONFIG_VULKAN_SDK_LINUXX64_URL}" "${CMAKE_BINARY_DIR}/vulkan-sdk" "vulkan-sdk-${PL_CONFIG_VULKAN_SDK_LINUXX64_VERSION}")
                    set(PL_VULKAN_DIR "${CMAKE_BINARY_DIR}/vulkan-sdk/${PL_CONFIG_VULKAN_SDK_LINUXX64_VERSION}" CACHE PATH "Directory of the Vulkan SDK" FORCE)

                    find_path(PL_VULKAN_DIR config/vk_layer_settings.txt
                        PATHS
                        ${PL_VULKAN_DIR}
                        $ENV{VULKAN_SDK}
                    )
                endif()
            endif()

            if((PL_VULKAN_DIR STREQUAL "PL_VULKAN_DIR-NOTFOUND") OR (PL_VULKAN_DIR STREQUAL ""))
                message(FATAL_ERROR "Failed to find vulkan SDK. Pl requires the vulkan sdk ${PL_CONFIG_VULKAN_SDK_LINUXX64_VERSION}. Please set the environment variable VULKAN_SDK to the vulkan sdk location.")
            endif()

            # set(CMAKE_FIND_DEBUG_MODE FALSE)
        endif()
    else()
        message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(PlVulkan DEFAULT_MSG PL_VULKAN_DIR)
    
	if(PL_CMAKE_ARCHITECTURE_64BIT)
		add_library(PlVulkan::Loader SHARED IMPORTED)
		set_target_properties(PlVulkan::Loader PROPERTIES IMPORTED_LOCATION "${PL_VULKAN_DIR}/x86_64/lib/libvulkan.so")
		set_target_properties(PlVulkan::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PL_VULKAN_DIR}/x86_64/include")

		add_library(PlVulkan::DXC SHARED IMPORTED)
		set_target_properties(PlVulkan::DXC PROPERTIES IMPORTED_LOCATION "${PL_VULKAN_DIR}/x86_64/lib/libdxcompiler.so")
		set_target_properties(PlVulkan::DXC PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PL_VULKAN_DIR}/x86_64/include")
	else()
		message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
	endif()
    
endmacro()
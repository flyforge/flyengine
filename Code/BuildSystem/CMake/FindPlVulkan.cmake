# find the folder into which the Vulkan SDK has been installed

# early out, if this target has been created before
if((TARGET PlVulkan::Loader) AND(TARGET PlVulkan::DXC))
	return()
endif()

set(PLASMA_VULKAN_DIR $ENV{VULKAN_SDK} CACHE PATH "Directory of the Vulkan SDK")

pl_pull_compiler_and_architecture_vars()
pl_pull_config_vars()

get_property(PLASMA_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY PLASMA_SUBMODULE_PREFIX_PATH)

if(PLASMA_CMAKE_PLATFORM_WINDOWS_DESKTOP AND PLASMA_CMAKE_ARCHITECTURE_64BIT)
	if((PLASMA_VULKAN_DIR STREQUAL "PLASMA_VULKAN_DIR-NOTFOUND") OR(PLASMA_VULKAN_DIR STREQUAL ""))
		# set(CMAKE_FIND_DEBUG_MODE TRUE)
		unset(PLASMA_VULKAN_DIR CACHE)
		unset(PlVulkan_DIR CACHE)
		find_path(PLASMA_VULKAN_DIR Config/vk_layer_settings.txt
			PATHS
			${PLASMA_VULKAN_DIR}
			$ENV{VULKAN_SDK}
			REQUIRED
		)

		# set(CMAKE_FIND_DEBUG_MODE FALSE)
	endif()
elseif(PLASMA_CMAKE_PLATFORM_LINUX AND PLASMA_CMAKE_ARCHITECTURE_64BIT)
	if((PLASMA_VULKAN_DIR STREQUAL "PLASMA_VULKAN_DIR-NOTFOUND") OR(PLASMA_VULKAN_DIR STREQUAL ""))
		# set(CMAKE_FIND_DEBUG_MODE TRUE)
		unset(PLASMA_VULKAN_DIR CACHE)
		unset(PlVulkan_DIR CACHE)
		find_path(PLASMA_VULKAN_DIR config/vk_layer_settings.txt
			PATHS
			${PLASMA_VULKAN_DIR}
			$ENV{VULKAN_SDK}
		)

		if(PLASMA_CMAKE_ARCHITECTURE_X86)
			if((PLASMA_VULKAN_DIR STREQUAL "PLASMA_VULKAN_DIR-NOTFOUND") OR (PLASMA_VULKAN_DIR STREQUAL ""))
				pl_download_and_extract("${PLASMA_CONFIG_VULKAN_SDK_LINUXX64_URL}" "${CMAKE_BINARY_DIR}/vulkan-sdk" "vulkan-sdk-${PLASMA_CONFIG_VULKAN_SDK_LINUXX64_VERSION}")
				set(PLASMA_VULKAN_DIR "${CMAKE_BINARY_DIR}/vulkan-sdk/${PLASMA_CONFIG_VULKAN_SDK_LINUXX64_VERSION}" CACHE PATH "Directory of the Vulkan SDK" FORCE)

				find_path(PLASMA_VULKAN_DIR config/vk_layer_settings.txt
					PATHS
					${PLASMA_VULKAN_DIR}
					$ENV{VULKAN_SDK}
				)
			endif()
		endif()

		if((PLASMA_VULKAN_DIR STREQUAL "PLASMA_VULKAN_DIR-NOTFOUND") OR (PLASMA_VULKAN_DIR STREQUAL ""))
			message(FATAL_ERROR "Failed to find vulkan SDK. Pl requires the vulkan sdk ${PLASMA_CONFIG_VULKAN_SDK_LINUXX64_VERSION}. Please set the environment variable VULKAN_SDK to the vulkan sdk location.")
		endif()

		# set(CMAKE_FIND_DEBUG_MODE FALSE)
	endif()
else()
	message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PlVulkan DEFAULT_MSG PLASMA_VULKAN_DIR)

if(NOT(PLASMA_VULKAN_DIR STREQUAL "PLASMA_VULKAN_DIR-NOTFOUND") AND NOT(PLASMA_VULKAN_DIR STREQUAL ""))
	set(PLASMAVULKAN_FOUND TRUE)
endif()

if(PLASMAVULKAN_FOUND)
	if(PLASMA_CMAKE_PLATFORM_WINDOWS_DESKTOP AND PLASMA_CMAKE_ARCHITECTURE_64BIT)
		add_library(PlVulkan::Loader STATIC IMPORTED)
		set_target_properties(PlVulkan::Loader PROPERTIES IMPORTED_LOCATION "${PLASMA_VULKAN_DIR}/Lib/vulkan-1.lib")
		set_target_properties(PlVulkan::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PLASMA_VULKAN_DIR}/Include")

		add_library(PlVulkan::DXC SHARED IMPORTED)
		set_target_properties(PlVulkan::DXC PROPERTIES IMPORTED_LOCATION "${PLASMA_VULKAN_DIR}/Bin/dxcompiler.dll")
		set_target_properties(PlVulkan::DXC PROPERTIES IMPORTED_IMPLIB "${PLASMA_VULKAN_DIR}/Lib/dxcompiler.lib")
		set_target_properties(PlVulkan::DXC PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PLASMA_VULKAN_DIR}/Include")

	elseif(PLASMA_CMAKE_PLATFORM_LINUX AND PLASMA_CMAKE_ARCHITECTURE_64BIT)
		add_library(PlVulkan::Loader SHARED IMPORTED)
		set_target_properties(PlVulkan::Loader PROPERTIES IMPORTED_LOCATION "${PLASMA_VULKAN_DIR}/x86_64/lib/libvulkan.so.1.3.216")
		set_target_properties(PlVulkan::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PLASMA_VULKAN_DIR}/x86_64/include")

		add_library(PlVulkan::DXC SHARED IMPORTED)
		set_target_properties(PlVulkan::DXC PROPERTIES IMPORTED_LOCATION "${PLASMA_VULKAN_DIR}/x86_64/lib/libdxcompiler.so.3.7")
		set_target_properties(PlVulkan::DXC PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PLASMA_VULKAN_DIR}/x86_64/include")
	else()
		message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
	endif()
endif()

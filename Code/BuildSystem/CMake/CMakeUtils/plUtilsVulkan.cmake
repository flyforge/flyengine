# #####################################
# ## Vulkan support
# #####################################

set(PL_BUILD_EXPERIMENTAL_VULKAN ON CACHE BOOL "Whether to enable experimental / work-in-progress Vulkan code")

# #####################################
# ## pl_requires_vulkan()
# #####################################
macro(pl_requires_vulkan)
	pl_requires(PL_CMAKE_PLATFORM_SUPPORTS_VULKAN)
	pl_requires(PL_BUILD_EXPERIMENTAL_VULKAN)
	find_package(PlVulkan REQUIRED)
endmacro()

# #####################################
# ## pl_link_target_vulkan(<target>)
# #####################################
function(pl_link_target_vulkan TARGET_NAME)
	pl_requires_vulkan()

	find_package(PlVulkan REQUIRED)

	if(PLVULKAN_FOUND)
		target_link_libraries(${TARGET_NAME} PRIVATE PlVulkan::Loader)

		if (COMMAND pl_platformhook_link_target_vulkan)
			# call platform-specific hook for linking with Vulkan
			pl_platformhook_link_target_vulkan()
		endif()
	endif()
endfunction()

# #####################################
# ## pl_link_target_dxc(<target>)
# #####################################
function(pl_link_target_dxc TARGET_NAME)
	pl_requires_vulkan()

	find_package(PlVulkan REQUIRED)

	if(PLVULKAN_FOUND)
		target_link_libraries(${TARGET_NAME} PRIVATE PlVulkan::DXC)

		get_target_property(_dll_location PlVulkan::DXC IMPORTED_LOCATION)

		if(NOT _dll_location STREQUAL "")
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:PlVulkan::DXC> $<TARGET_FILE_DIR:${TARGET_NAME}>)
		endif()

		unset(_dll_location)
	endif()
endfunction()

# #####################################
# ## pl_sources_target_spirv_reflect(<target>)
# #####################################
function(pl_sources_target_spirv_reflect TARGET_NAME)
	pl_requires_vulkan()

	find_package(PlVulkan REQUIRED)

	if(PLVULKAN_FOUND)
		if(PL_CMAKE_PLATFORM_WINDOWS_DESKTOP AND PL_CMAKE_ARCHITECTURE_64BIT)
			target_include_directories(${TARGET_NAME} PRIVATE "${PL_VULKAN_DIR}/Source/SPIRV-Reflect")
			target_sources(${TARGET_NAME} PRIVATE "${PL_VULKAN_DIR}/Source/SPIRV-Reflect/spirv_reflect.h")
			target_sources(${TARGET_NAME} PRIVATE "${PL_VULKAN_DIR}/Source/SPIRV-Reflect/spirv_reflect.c")
			source_group("SPIRV-Reflect" FILES "${PL_VULKAN_DIR}/Source/SPIRV-Reflect/spirv_reflect.h" "${PL_VULKAN_DIR}/x86_64/include/SPIRV-Reflect/spirv_reflect.c")
		elseif(PL_CMAKE_PLATFORM_LINUX AND PL_CMAKE_ARCHITECTURE_64BIT)
			target_include_directories(${TARGET_NAME} PRIVATE "${PL_VULKAN_DIR}/x86_64/include/SPIRV-Reflect")
			target_sources(${TARGET_NAME} PRIVATE "${PL_VULKAN_DIR}/x86_64/include/SPIRV-Reflect/spirv_reflect.h")
			target_sources(${TARGET_NAME} PRIVATE "${PL_VULKAN_DIR}/x86_64/include/SPIRV-Reflect/spirv_reflect.c")
			source_group("SPIRV-Reflect" FILES "${PL_VULKAN_DIR}/x86_64/include/SPIRV-Reflect/spirv_reflect.h" "${PL_VULKAN_DIR}/x86_64/include/SPIRV-Reflect/spirv_reflect.c")
		else()
			message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
		endif()
	endif()
endfunction()
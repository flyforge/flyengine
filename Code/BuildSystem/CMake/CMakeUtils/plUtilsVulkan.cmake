# #####################################
# ## Vulkan support
# #####################################

set(PLASMA_BUILD_VULKAN ON CACHE BOOL "Whether to enable Vulkan code")

# #####################################
# ## pl_requires_vulkan()
# #####################################
macro(pl_requires_vulkan)
	pl_requires_one_of(PLASMA_CMAKE_PLATFORM_LINUX PLASMA_CMAKE_PLATFORM_WINDOWS)
	pl_requires(PLASMA_BUILD_VULKAN)
	find_package(PlVulkan REQUIRED)
endmacro()

# #####################################
# ## pl_link_target_vulkan(<target>)
# #####################################
function(pl_link_target_vulkan TARGET_NAME)
	pl_requires_vulkan()

	find_package(PlVulkan REQUIRED)

	if(PLASMAVULKAN_FOUND)
		target_link_libraries(${TARGET_NAME} PRIVATE PlVulkan::Loader)

		# Only on linux is the loader a dll.
		if(PLASMA_CMAKE_PLATFORM_LINUX)
			get_target_property(_dll_location PlVulkan::Loader IMPORTED_LOCATION)

			if(NOT _dll_location STREQUAL "")
				add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:PlVulkan::Loader> $<TARGET_FILE_DIR:${TARGET_NAME}>)
			endif()

			unset(_dll_location)
		endif()
	endif()
endfunction()

# #####################################
# ## pl_link_target_dxc(<target>)
# #####################################
function(pl_link_target_dxc TARGET_NAME)
	pl_requires_vulkan()

	find_package(PlVulkan REQUIRED)

	if(PLASMAVULKAN_FOUND)
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

	if(PLASMAVULKAN_FOUND)
		target_include_directories(${TARGET_NAME} PRIVATE "${PLASMA_VULKAN_DIR}/source/SPIRV-Reflect")
		target_sources(${TARGET_NAME} PRIVATE "${PLASMA_VULKAN_DIR}/source/SPIRV-Reflect/spirv_reflect.h")
		target_sources(${TARGET_NAME} PRIVATE "${PLASMA_VULKAN_DIR}/source/SPIRV-Reflect/spirv_reflect.c")
		source_group("SPIRV-Reflect" FILES "${PLASMA_VULKAN_DIR}/source/SPIRV-Reflect/spirv_reflect.h" "${PLASMA_VULKAN_DIR}/source/SPIRV-Reflect/spirv_reflect.c")
	endif()
endfunction()
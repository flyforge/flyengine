# #####################################
# ## pl_requires_renderer()
# #####################################

macro(pl_requires_renderer)
	# PLATFORM-TODO
	if(PL_CMAKE_PLATFORM_WINDOWS)
		pl_requires_d3d()
	else()
		pl_requires_vulkan()
	endif()
endmacro()

# #####################################
# ## pl_add_renderers(<target>)
# ## Add all required libraries and dependencies to the given target so it has access to all available renderers.
# #####################################
function(pl_add_renderers TARGET_NAME)
	# PLATFORM-TODO
	if(PL_BUILD_EXPERIMENTAL_VULKAN)
		target_link_libraries(${TARGET_NAME}
			PRIVATE
			RendererVulkan
		)

		add_dependencies(${TARGET_NAME}
			ShaderCompilerDXC
		)
	endif()

	if(PL_CMAKE_PLATFORM_WINDOWS)
		target_link_libraries(${TARGET_NAME}
			PRIVATE
			RendererDX11
		)
		pl_link_target_dx11(${TARGET_NAME})

		add_dependencies(${TARGET_NAME}
			ShaderCompilerHLSL
		)
	endif()
endfunction()
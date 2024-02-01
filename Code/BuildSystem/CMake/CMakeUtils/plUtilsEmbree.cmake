# #####################################
# ## Embree support
# #####################################

set(PL_BUILD_EMBREE OFF CACHE BOOL "Whether support for Intel Embree should be added")

# #####################################
# ## pl_requires_embree()
# #####################################
macro(pl_requires_embree)
	pl_requires(PL_CMAKE_PLATFORM_WINDOWS)
	pl_requires(PL_BUILD_EMBREE)
endmacro()

# #####################################
# ## pl_link_target_embree(<target>)
# #####################################
function(pl_link_target_embree TARGET_NAME)
	pl_requires_embree()

	find_package(PlEmbree REQUIRED)

	if(PLEMBREE_FOUND)
		target_link_libraries(${TARGET_NAME} PRIVATE PlEmbree::PlEmbree)

		target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_EMBREE_SUPPORT)

		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:PlEmbree::PlEmbree> $<TARGET_FILE_DIR:${TARGET_NAME}>
		)
	endif()
endfunction()

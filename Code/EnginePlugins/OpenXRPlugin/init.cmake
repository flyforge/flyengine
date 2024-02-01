######################################
### OpenXR support
######################################

set (PL_BUILD_OPENXR ON CACHE BOOL "Whether support for OpenXR should be added")

######################################
### pl_requires_openxr()
######################################

macro(pl_requires_openxr)

	pl_requires(PL_CMAKE_PLATFORM_WINDOWS)
	pl_requires(PL_BUILD_OPENXR)
	# While counter-intuitive, we need to find the package here so that the PUBLIC inherited
	# target_sources using generator expressions can be resolved in the dependant projects.
	find_package(plOpenXR REQUIRED)

endmacro()

######################################
### pl_link_target_openxr(<target>)
######################################

function(pl_link_target_openxr TARGET_NAME)

	pl_requires_openxr()

	find_package(plOpenXR REQUIRED)

	if (PLOPENXR_FOUND)
		target_link_libraries(${TARGET_NAME} PRIVATE plOpenXR::Loader)

        get_target_property(_dll_location plOpenXR::Loader IMPORTED_LOCATION)
		if (NOT _dll_location STREQUAL "")
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:plOpenXR::Loader> $<TARGET_FILE_DIR:${TARGET_NAME}>)
		endif()
		unset(_dll_location)
        
        if (PL_CMAKE_PLATFORM_WINDOWS_DESKTOP AND PL_CMAKE_ARCHITECTURE_64BIT)
			target_link_libraries(${TARGET_NAME} PRIVATE plOpenXR::Remoting)

			# Copy INTERFACE_SOURCES to the output folder.
			get_target_property(REMOTING_ASSETS plOpenXR::Remoting INTERFACE_SOURCES)
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${REMOTING_ASSETS} $<TARGET_FILE_DIR:${TARGET_NAME}>)
			set_property(SOURCE ${REMOTING_ASSETS} PROPERTY VS_DEPLOYMENT_CONTENT 1)
			set_property(SOURCE ${REMOTING_ASSETS} PROPERTY VS_DEPLOYMENT_LOCATION "")
			unset(REMOTING_ASSETS)

        endif()
		
		pl_uwp_add_import_to_sources(${TARGET_NAME} plOpenXR::Loader)

	endif()

endfunction()


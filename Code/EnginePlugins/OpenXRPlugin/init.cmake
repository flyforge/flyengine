######################################
### OpenXR support
######################################

set (PLASMA_BUILD_OPENXR OFF CACHE BOOL "Whether support for OpenXR should be added")

######################################
### pl_requires_openxr()
######################################

macro(pl_requires_openxr)

	pl_requires_windows()
	pl_requires(PLASMA_BUILD_OPENXR)
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

	if (PLASMAOPENXR_FOUND)
		target_link_libraries(${TARGET_NAME} PRIVATE plOpenXR::Loader)

		get_target_property(_dll_location plOpenXR::Loader IMPORTED_LOCATION)
		if (NOT _dll_location STREQUAL "")
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:plOpenXR::Loader> $<TARGET_FILE_DIR:${TARGET_NAME}>)
		endif()

		if (PLASMA_CMAKE_PLATFORM_WINDOWS_DESKTOP AND PLASMA_CMAKE_ARCHITECTURE_64BIT)
			# This will add the remoting .targets file.
			target_link_libraries(${TARGET_NAME} PRIVATE $<TARGET_FILE:plOpenXR::Remoting>)
		endif()
		unset(_dll_location)
		pl_uwp_add_import_to_sources(${TARGET_NAME} plOpenXR::Loader)

	endif()

endfunction()

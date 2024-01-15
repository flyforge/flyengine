# #####################################
# ## pl_include_plExport()
# #####################################

macro(pl_include_plExport)
	# Create a modified version of the plExport.cmake file,
	# where the absolute paths to the original locations are replaced
	# with the absolute paths to this installation
	pl_get_export_location(EXP_FILE)
	set(IMP_FILE "${CMAKE_BINARY_DIR}/plExport.cmake")
	set(EXPINFO_FILE "${PLASMA_OUTPUT_DIRECTORY_DLL}/plExportInfo.cmake")

	# read the file that contains the original paths
	include(${EXPINFO_FILE})

	# read the plExport file into a string
	file(READ ${EXP_FILE} IMP_CONTENT)

	# replace the original paths with our paths
	string(REPLACE ${EXPINP_OUTPUT_DIRECTORY_DLL} ${PLASMA_OUTPUT_DIRECTORY_DLL} IMP_CONTENT "${IMP_CONTENT}")
	string(REPLACE ${EXPINP_OUTPUT_DIRECTORY_LIB} ${PLASMA_OUTPUT_DIRECTORY_LIB} IMP_CONTENT "${IMP_CONTENT}")
	string(REPLACE ${EXPINP_SOURCE_DIR} ${PLASMA_SDK_DIR} IMP_CONTENT "${IMP_CONTENT}")

	# write the modified plExport file to disk
	file(WRITE ${IMP_FILE} "${IMP_CONTENT}")

	# include the modified file, so that the CMake targets become known
	include(${IMP_FILE})
endmacro()

# #####################################
# ## pl_configure_external_project()
# #####################################
macro(pl_configure_external_project)
	file(RELATIVE_PATH PLASMA_SUBMODULE_PREFIX_PATH ${CMAKE_SOURCE_DIR} ${PLASMA_SDK_DIR})
	set_property(GLOBAL PROPERTY PLASMA_SUBMODULE_PREFIX_PATH ${PLASMA_SUBMODULE_PREFIX_PATH})

	if(PLASMA_SUBMODULE_PREFIX_PATH STREQUAL "")
		set(PLASMA_SUBMODULE_MODE FALSE)
	else()
		set(PLASMA_SUBMODULE_MODE FALSE)
	endif()

	set_property(GLOBAL PROPERTY PLASMA_SUBMODULE_MODE ${PLASMA_SUBMODULE_MODE})

	pl_build_filter_init()

	pl_set_build_types()
endmacro()
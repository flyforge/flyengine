# find the folder in which Embree is located

# early out, if this target has been created before
if(TARGET PlEmbree::PlEmbree)
	return()
endif()

find_path(PLASMA_EMBREE_DIR include/embree3/rtcore.h
	PATHS
	${PLASMA_ROOT}/Code/ThirdParty/embree
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(EMBREE_LIB_PATH "${PLASMA_EMBREE_DIR}/vc141win64")
else()
	set(EMBREE_LIB_PATH "${PLASMA_EMBREE_DIR}/vc141win32")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PlEmbree DEFAULT_MSG PLASMA_EMBREE_DIR)

if(NOT(PLASMA_EMBREE_DIR STREQUAL "PLASMA_EMBREE_DIR-NOTFOUND") AND NOT(PLASMA_EMBREE_DIR STREQUAL ""))
	set(PLASMAEMBREE_FOUND TRUE)
endif()

if(PLASMAEMBREE_FOUND)
	add_library(PlEmbree::PlEmbree SHARED IMPORTED)
	set_target_properties(PlEmbree::PlEmbree PROPERTIES IMPORTED_LOCATION "${EMBREE_LIB_PATH}/embree3.dll")
	set_target_properties(PlEmbree::PlEmbree PROPERTIES IMPORTED_IMPLIB "${EMBREE_LIB_PATH}/embree3.lib")
	set_target_properties(PlEmbree::PlEmbree PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PLASMA_EMBREE_DIR}/include")
endif()

mark_as_advanced(FORCE PLASMA_EMBREE_DIR)

unset(EMBREE_LIB_PATH)

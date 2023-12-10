pl_cmake_init()

pl_requires_fmod()

if (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	# UWP builds

	if (PLASMA_FMOD_USE_CUSTOM_INSTALLATION)
		set(FMOD_SEARCH_PATH "[HKEY_CURRENT_USER\\Software\\FMOD\ Studio\ API\ Universal\ Windows\ Platform]")
	else()
		set(FMOD_SEARCH_PATH "${CMAKE_SOURCE_DIR}/Code/ThirdParty/fmod/windows-uwp")
	endif()
		
	set(FMOD_DLL_SUFFIX ".dll")
	set(FMOD_LIB_SUFFIX ".lib")
		
else()
	# Desktop Windows builds
		
	if (PLASMA_FMOD_USE_CUSTOM_INSTALLATION)
		set(FMOD_SEARCH_PATH "[HKEY_CURRENT_USER\\Software\\FMOD\ Studio\ API\ Windows]")
	else()
		set(FMOD_SEARCH_PATH "${CMAKE_SOURCE_DIR}/Code/ThirdParty/fmod/windows")
	endif()

	set(FMOD_DLL_SUFFIX ".dll")
	set(FMOD_LIB_SUFFIX "_vc.lib")

endif()

if (NOT PLASMA_FMOD_SDK_LOCATION OR PLASMA_FMOD_SDK_LOCATION STREQUAL "")
	set(PLASMA_FMOD_SDK_LOCATION "PLASMA_FMOD_SDK_LOCATION-NOTFOUND")
endif()

find_path(PLASMA_FMOD_SDK_LOCATION api/core/inc/fmod.h PATHS ${FMOD_SEARCH_PATH})

message(STATUS "PLASMA_FMOD_SDK_LOCATION is ${PLASMA_FMOD_SDK_LOCATION}")

set (FMOD_DIR_STUDIO "${PLASMA_FMOD_SDK_LOCATION}/api/studio")
set (FMOD_DIR_FSBANK "${PLASMA_FMOD_SDK_LOCATION}/api/fsbank")
set (FMOD_DIR_CORE "${PLASMA_FMOD_SDK_LOCATION}/api/core")

if (PLASMA_CMAKE_ARCHITECTURE_ARM)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		message(FATAL_ERROR "Fmod does not support arm64, or support needs to be added to Plasma once binaries become available")
	else()
		set(FMOD_LIB_ARCH "arm")
	endif()
else()
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(FMOD_LIB_ARCH "x64")
	else()
		set(FMOD_LIB_ARCH "x86")
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PlasmaFmod DEFAULT_MSG PLASMA_FMOD_SDK_LOCATION)

if (PLASMAFMOD_FOUND)

	set(FMOD_DEBUG_SUFFIX, "")
	
	# set this, if you want the 'logging' libraries
	#set(FMOD_DEBUG_SUFFIX, "L")

	add_library(plFmod::Core SHARED IMPORTED GLOBAL)
	set_target_properties(plFmod::Core PROPERTIES IMPORTED_LOCATION "${FMOD_DIR_CORE}/lib/${FMOD_LIB_ARCH}/fmod${FMOD_DLL_SUFFIX}")
	set_target_properties(plFmod::Core PROPERTIES IMPORTED_LOCATION_DEBUG "${FMOD_DIR_CORE}/lib/${FMOD_LIB_ARCH}/fmod${FMOD_DEBUG_SUFFIX}${FMOD_DLL_SUFFIX}")
	set_target_properties(plFmod::Core PROPERTIES IMPORTED_IMPLIB "${FMOD_DIR_CORE}/lib/${FMOD_LIB_ARCH}/fmod${FMOD_LIB_SUFFIX}")
	set_target_properties(plFmod::Core PROPERTIES IMPORTED_IMPLIB_DEBUG "${FMOD_DIR_CORE}/lib/${FMOD_LIB_ARCH}/fmod${FMOD_DEBUG_SUFFIX}${FMOD_LIB_SUFFIX}")
	set_target_properties(plFmod::Core PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR_CORE}/inc")
	pl_uwp_mark_import_as_content(plFmod::Core)

	add_library(plFmod::FsBank SHARED IMPORTED GLOBAL)
	set_target_properties(plFmod::FsBank PROPERTIES IMPORTED_LOCATION "${FMOD_DIR_FSBANK}/lib/${FMOD_LIB_ARCH}/fsbank${FMOD_DLL_SUFFIX}")
	set_target_properties(plFmod::FsBank PROPERTIES IMPORTED_LOCATION_DEBUG "${FMOD_DIR_FSBANK}/lib/${FMOD_LIB_ARCH}/fsbank${FMOD_DEBUG_SUFFIX}${FMOD_DLL_SUFFIX}")
	set_target_properties(plFmod::FsBank PROPERTIES IMPORTED_IMPLIB "${FMOD_DIR_FSBANK}/lib/${FMOD_LIB_ARCH}/fsbank${FMOD_LIB_SUFFIX}")
	set_target_properties(plFmod::FsBank PROPERTIES IMPORTED_IMPLIB_DEBUG "${FMOD_DIR_FSBANK}/lib/${FMOD_LIB_ARCH}/fsbank${FMOD_DEBUG_SUFFIX}${FMOD_LIB_SUFFIX}")
	set_target_properties(plFmod::FsBank PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR_FSBANK}/inc")
	pl_uwp_mark_import_as_content(plFmod::FsBank)

	add_library(plFmod::Studio SHARED IMPORTED GLOBAL)
	set_target_properties(plFmod::Studio PROPERTIES IMPORTED_LOCATION "${FMOD_DIR_STUDIO}/lib/${FMOD_LIB_ARCH}/fmodstudio${FMOD_DLL_SUFFIX}")
	set_target_properties(plFmod::Studio PROPERTIES IMPORTED_LOCATION_DEBUG "${FMOD_DIR_STUDIO}/lib/${FMOD_LIB_ARCH}/fmodstudio${FMOD_DEBUG_SUFFIX}${FMOD_DLL_SUFFIX}")
	set_target_properties(plFmod::Studio PROPERTIES IMPORTED_IMPLIB "${FMOD_DIR_STUDIO}/lib/${FMOD_LIB_ARCH}/fmodstudio${FMOD_LIB_SUFFIX}")
	set_target_properties(plFmod::Studio PROPERTIES IMPORTED_IMPLIB_DEBUG "${FMOD_DIR_STUDIO}/lib/${FMOD_LIB_ARCH}/fmodstudio${FMOD_DEBUG_SUFFIX}${FMOD_LIB_SUFFIX}")
	set_target_properties(plFmod::Studio PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR_STUDIO}/inc")
	target_link_libraries(plFmod::Studio INTERFACE plFmod::Core)
	pl_uwp_mark_import_as_content(plFmod::Studio)

endif()

mark_as_advanced(FORCE PLASMA_FMOD_SDK_LOCATION)
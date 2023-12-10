# #####################################
# ## pl_link_target_dx11(<target>)
# #####################################

function(pl_link_target_dx11 TARGET_NAME)
	pl_requires_d3d()

	get_property(PLASMA_DX11_LIBRARY GLOBAL PROPERTY PLASMA_DX11_LIBRARY)

	# only execute find_package once
	if(NOT PLASMA_DX11_LIBRARY)
		find_package(DirectX11)

		if(DirectX11_FOUND)
			set_property(GLOBAL PROPERTY PLASMA_DX11_LIBRARY ${DirectX11_LIBRARY})
			set_property(GLOBAL PROPERTY PLASMA_DX11_LIBRARIES ${DirectX11_D3D11_LIBRARIES})
		endif()
	endif()

	get_property(PLASMA_DX11_LIBRARY GLOBAL PROPERTY PLASMA_DX11_LIBRARY)
	get_property(PLASMA_DX11_LIBRARIES GLOBAL PROPERTY PLASMA_DX11_LIBRARIES)

	target_link_libraries(${TARGET_NAME}
		PRIVATE
		${PLASMA_DX11_LIBRARIES})

	if(PLASMA_CMAKE_ARCHITECTURE_ARM)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(DX11_COPY_DLLS_BIT "arm64")
		else()
			set(DX11_COPY_DLLS_BIT "arm")
		endif()
	else()
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(DX11_COPY_DLLS_BIT "x64")
		else()
			set(DX11_COPY_DLLS_BIT "x86")
		endif()
	endif()

	# arm dll is not provide in the windows SDK.
	if(NOT PLASMA_CMAKE_ARCHITECTURE_ARM)
		if(${PLASMA_DX11_LIBRARY} MATCHES "/8\\.0/")
			set(DX11_COPY_DLLS_WINSDKVERSION "8.0")
			set(DX11_COPY_DLLS_DLL_VERSION "46")
		elseif(${PLASMA_DX11_LIBRARY} MATCHES "/8\\.1/")
			set(DX11_COPY_DLLS_WINSDKVERSION "8.1")
			set(DX11_COPY_DLLS_DLL_VERSION "47")
		elseif(${PLASMA_DX11_LIBRARY} MATCHES "/10/")
			set(DX11_COPY_DLLS_WINSDKVERSION "10")
			set(DX11_COPY_DLLS_DLL_VERSION "47")
		endif()
	endif()

	if(${DX11_COPY_DLLS_WINSDKVERSION})
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"%ProgramFiles(x86)%/Windows Kits/${DX11_COPY_DLLS_WINSDKVERSION}/Redist/D3D/${DX11_COPY_DLLS_BIT}/d3dcompiler_${DX11_COPY_DLLS_DLL_VERSION}.dll"
			$<TARGET_FILE_DIR:${TARGET_NAME}>
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		)
	endif()
endfunction()

# #####################################
# ## pl_requires_d3d()
# #####################################
macro(pl_requires_d3d)
	pl_requires_windows()
endmacro()
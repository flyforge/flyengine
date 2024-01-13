set(PLASMA_D3D12_AGILITY_SDK_DIR "PLASMA_D3D12_AGILITY_SDK_DIR-NOTFOUND" CACHE PATH "Directory of D3D12 Agility SDK")
set(PLASMA_D3D12_AGILITY_SDK_INCLUDE_DIR "PLASMA_D3D12_AGILITY_SDK_INCLUDE_DIR-NOTFOUND" CACHE PATH "Directory of D3D12 Agliity SDK Includes")
set(PLASMA_BUILD_EXPERIMENTAL_D3D12_SUPPORT ON CACHE BOOL "Add support for D3D12. PC/Xbox")

mark_as_advanced(FORCE PLASMA_D3D12_AGILITY_SDK_DIR)
mark_as_advanced(FORCE PLASMA_D3D12_AGILITY_SDK_INCLUDE_DIR)
mark_as_advanced(FORCE PLASMA_BUILD_EXPERIMENTAL_D3D12_SUPPORT)

# NOTE: This shouldnt be fixed Aglity Version. Find a way to "adjust this."
set(PLASMA_D3D12_AGILITY_SDK_PACKAGE_PATH "${CMAKE_BINARY_DIR}/packages/Microsoft.Direct3D.D3D12.1.711.3/build/native")
set(PLASMA_D3D12_AGILITY_SDK_PACKAGE_PATH_INCLUDE "${PLASMA_D3D12_AGILITY_SDK_PACKAGE_PATH}/include")

macro(pl_requires_d3d12)
    pl_requires_d3d()
    pl_requires(PLASMA_BUILD_EXPERIMENTAL_D3D12_SUPPORT)
endmacro()

function(pl_export_target_dx12 TARGET_NAME)

    pl_requires_d3d12()
    # Install D3D12 Aglity SDK for the latest sdk.
    pl_nuget_init()

    message("D3D12 SDK DLL PATH: ${PLASMA_D3D12_AGILITY_SDK_PACKAGE_PATH}")
    # Path where d3d12 dlls will end up on build.
    # NOTE: Should we allow the user to change this?
    set(PLASMA_D3D12_RESOURCES ${CMAKE_BINARY_DIR}/x64)

    if(NOT EXISTS ${PLASMA_D3D12_RESOURCES})
        file(MAKE_DIRECTORY ${PLASMA_D3D12_RESOURCES})
    endif()

    target_include_directories(${TARGET_NAME} PUBLIC ${PLASMA_D3D12_AGILITY_SDK_PACKAGE_PATH_INCLUDE})

    execute_process(COMMAND ${NUGET} restore ${CMAKE_SOURCE_DIR}/${PLASMA_CMAKE_RELPATH}/CMakeUtils/packages.config -PackagesDirectory ${CMAKE_BINARY_DIR}/packages
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

#    add_custom_command(TARGET ${TARGET_NAME}
#            PRE_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PLASMA_D3D12_AGILITY_SDK_PACKAGE_PATH}/bin/x64/D3D12Core.dll ${PLASMA_D3D12_RESOURCES}
#            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PLASMA_D3D12_AGILITY_SDK_PACKAGE_PATH}/bin/x64/d3d12SDKLayers.dll ${PLASMA_D3D12_RESOURCES}
#            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#    )

    target_include_directories(${PROJECT_NAME} PUBLIC ${PLASMA_D3D12_AGILITY_SDK_PACKAGE_PATH})
endfunction()

# #####################################
# ## pl_link_target_dx12(<target>)
# #####################################
function(pl_link_target_dx12 TARGET_NAME)
    # pl_export_target_dx12(${TARGET_NAME})
    target_link_libraries(${TARGET_NAME}
            PRIVATE
            RendererDX12
    )
endfunction()
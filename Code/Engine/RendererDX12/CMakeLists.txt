pl_cmake_init()

pl_requires_d3d12()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(LIBRARY ${PROJECT_NAME})

pl_export_target_dx12(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME}
        PRIVATE
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        Foundation
        RendererFoundation
)

if(PLASMA_3RDPARTY_GLFW_SUPPORT)
    target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
endif()
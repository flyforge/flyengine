pl_cmake_init()

pl_requires_amplitude()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(LIBRARY ${PROJECT_NAME} NO_UNITY)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Core
    RendererCore
    AudioSystemPlugin
)

pl_link_target_amplitude(${PROJECT_NAME})

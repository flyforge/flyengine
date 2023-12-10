pl_cmake_init()

pl_requires_one_of(PLASMA_CMAKE_PLATFORM_WINDOWS_DESKTOP PLASMA_CMAKE_PLATFORM_LINUX)
pl_requires_renderer()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(APPLICATION ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

pl_add_renderers(${PROJECT_NAME})

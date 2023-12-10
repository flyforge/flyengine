pl_cmake_init()

pl_requires(PLASMA_CMAKE_PLATFORM_WINDOWS_DESKTOP)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(APPLICATION ${PROJECT_NAME})
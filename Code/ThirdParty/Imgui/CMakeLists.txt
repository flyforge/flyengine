pl_cmake_init()



pl_requires(PLASMA_3RDPARTY_IMGUI_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(SHARED_LIBRARY ${PROJECT_NAME} NO_PLASMA_PREFIX)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_IMGUI_SUPPORT)


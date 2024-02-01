pl_cmake_init()

pl_requires(PL_3RDPARTY_JOLT_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(SHARED_LIBRARY ${PROJECT_NAME} NO_PL_PREFIX)
target_compile_definitions(${PROJECT_NAME} PUBLIC JPH_SHARED_LIBRARY)
target_compile_definitions(${PROJECT_NAME} PRIVATE JPH_BUILD_SHARED_LIBRARY)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_JOLT_SUPPORT JPH_DEBUG_RENDERER)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})


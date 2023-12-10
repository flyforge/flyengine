pl_cmake_init()

pl_requires(PLASMA_3RDPARTY_ZSTD_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(SHARED_LIBRARY ${PROJECT_NAME} NO_PLASMA_PREFIX)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_ZSTD_SUPPORT)

target_include_directories(${PROJECT_NAME} PRIVATE common compress decompress)

if (PLASMA_COMPILE_ENGINE_AS_DLL AND PLASMA_CMAKE_PLATFORM_WINDOWS)

  target_compile_definitions(${PROJECT_NAME} PRIVATE ZSTD_DLL_EXPORT=1)
  target_compile_definitions(${PROJECT_NAME} PUBLIC ZSTD_DLL_IMPORT=1)

endif()

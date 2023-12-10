pl_cmake_init()

pl_requires(PLASMA_3RDPARTY_KRAUT_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_PLASMA_PREFIX)

#if (PLASMA_COMPILE_ENGINE_AS_DLL)
#  target_compile_definitions(${PROJECT_NAME} PUBLIC AE_COMPILE_ENGINE_AS_DLL)
#endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE BUILDSYSTEM_BUILDING_KRAUTGENERATOR_LIB BUILDSYSTEM_BUILDING_KRAUTFOUNDATION_LIB)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

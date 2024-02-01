pl_cmake_init()



pl_requires(PL_3RDPARTY_OZZ_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(LIBRARY ${PROJECT_NAME} NO_PL_PREFIX ALL_SYMBOLS_VISIBLE)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_OZZ_SUPPORT)

target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_BASE_LIB)
target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_ANIMATION_LIB)
target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_GEOMETRY_LIB)
target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_ANIMOFFLINE_LIB)
target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_ANIMATIONTOOLS_LIB)
target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_ANIMATIONFBX_LIB)
target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_OPTIONS_LIB)

if(PL_CMAKE_COMPILER_MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /wd4661)
endif()

if(PL_CMAKE_PLATFORM_WINDOWS)
  if (PL_COMPILE_ENGINE_AS_DLL)
    target_compile_definitions(${PROJECT_NAME} PUBLIC OZZ_USE_DYNAMIC_LINKING)
  endif()
endif()

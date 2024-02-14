pl_cmake_init()

pl_requires_optick()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(SHARED_LIBRARY ${PROJECT_NAME} NO_PL_PREFIX)

set_target_properties(${PROJECT_NAME}
    PROPERTIES 
        PUBLIC_HEADER "${CMAKE_CURRENT_LIST_DIR}/optick.h;${CMAKE_CURRENT_LIST_DIR}/optick.config.h"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE OPTICK_EXPORTS=1)

target_compile_definitions(${PROJECT_NAME} PUBLIC
  USE_OPTICK=1
  OPTICK_USE_VULKAN=0
  OPTICK_USE_D3D12=0
  OPTICK_BUILD_GUI_APP=0
  OPTICK_ENABLE_GPU_VULKAN=0
  OPTICK_ENABLE_GPU_D3D12=0
)

if(MSVC)
  # temporary solution to unblock C++17 users
  target_compile_definitions(${PROJECT_NAME} PRIVATE _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
endif()
pl_cmake_init()

pl_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE

)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ToolsFoundation
  GameEngine
  Utilities
)

if(PL_3RDPARTY_GLFW_SUPPORT AND PL_CMAKE_PLATFORM_LINUX)
  target_link_libraries(${PROJECT_NAME} PRIVATE xcb)
endif()

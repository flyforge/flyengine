pl_cmake_init()

# currently the requirements are only met on Windows (ie. file iteration)
pl_requires(PL_CMAKE_PLATFORM_WINDOWS)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(APPLICATION ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Foundation
)

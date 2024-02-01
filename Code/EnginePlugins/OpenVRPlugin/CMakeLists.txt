pl_cmake_init()


pl_requires(PL_BUILD_OPENVR)
pl_requires(PL_CMAKE_PLATFORM_WINDOWS)

AddOpenVR()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  GameEngine
)

CopyOpenVRDlls()

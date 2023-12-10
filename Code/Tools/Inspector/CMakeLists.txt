pl_cmake_init()

pl_requires_qt()

pl_requires(PLASMA_3RDPARTY_ADS_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(APPLICATION ${PROJECT_NAME})

pl_make_winmain_executable(${PROJECT_NAME})

pl_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Core
  GuiFoundation
  ads
)

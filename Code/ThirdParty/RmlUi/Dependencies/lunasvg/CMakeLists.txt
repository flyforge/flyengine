pl_cmake_init()

pl_requires_rmlui()

set (PROJECT_NAME "lunasvg")

pl_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_UNITY NO_PLASMA_PREFIX)

# override the default IDE folder
pl_set_project_ide_folder(${PROJECT_NAME} "${PLASMA_ROOT}/Code/ThirdParty/lunasvg")

target_include_directories(${PROJECT_NAME}
  PRIVATE
  "3rdparty/plutovg"
  "3rdparty/software"

  PUBLIC
  "Include"
)

if(PLASMA_CMAKE_PLATFORM_WINDOWS)
  target_compile_options(${PROJECT_NAME} PRIVATE /wd4456 /wd4458 /wd4701)
endif()

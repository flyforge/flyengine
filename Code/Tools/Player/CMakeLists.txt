pl_cmake_init()

pl_requires_renderer()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(APPLICATION ${PROJECT_NAME})

pl_make_winmain_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  GameEngine
)

if (PL_CMAKE_PLATFORM_WINDOWS_UWP)
  # Due to app sandboxing we need to explicitly name required plugins for UWP.
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
    ParticlePlugin
    InspectorPlugin
    TypeScriptPlugin
  )

  if (PL_BUILD_PHYSX)
    target_link_libraries(${PROJECT_NAME} PUBLIC PhysXPlugin)
  endif()

  if (PL_BUILD_FMOD)
    target_link_libraries(${PROJECT_NAME} PUBLIC FmodPlugin)
  endif()

  if (PL_BUILD_OPENXR)
    find_package(plOpenXR REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenXRPlugin)
  endif()

endif()

pl_add_renderers(${PROJECT_NAME})

add_dependencies(${PROJECT_NAME}
  InspectorPlugin
)

# set all external projects as runtime dependencies of this application
get_property(EXTERNAL_PROJECTS GLOBAL PROPERTY "EXTERNAL_PROJECTS")
if(EXTERNAL_PROJECTS)
	message(STATUS "External Projects: ${EXTERNAL_PROJECTS}")
	add_dependencies(${PROJECT_NAME} ${EXTERNAL_PROJECTS})
endif()

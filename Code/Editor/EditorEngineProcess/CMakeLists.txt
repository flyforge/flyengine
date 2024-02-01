pl_cmake_init()

pl_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(APPLICATION ${PROJECT_NAME})

pl_make_winmain_executable(${PROJECT_NAME})

pl_add_renderers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  EditorEngineProcessFramework
)

# For SetProcessDpiAwareness
if (PL_CMAKE_PLATFORM_WINDOWS_DESKTOP)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Shcore.lib
  )
endif()

# For plugins to work in UWP builds, they must be direct link dependencies, otherwise they are not packaged with the app.
if (PL_CMAKE_PLATFORM_WINDOWS_UWP)

  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    InspectorPlugin
  )

  if (PL_BUILD_FMOD)

    target_link_libraries(${PROJECT_NAME}
      PRIVATE
      FmodPlugin
    )

  endif()

  if (PL_BUILD_PHYSX)

    target_link_libraries(${PROJECT_NAME}
      PRIVATE
      PhysXPlugin
      PhysXCooking
    )

  endif()
endif()

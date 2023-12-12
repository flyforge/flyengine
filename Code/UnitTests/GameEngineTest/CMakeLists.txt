pl_cmake_init()



pl_requires_d3d()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(APPLICATION ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  GameEngine
  RendererDX11
  Utilities
  ParticlePlugin
)

if (PLASMA_3RDPARTY_DUKTAPE_SUPPORT)

  target_link_libraries(${PROJECT_NAME}
    PUBLIC
    TypeScriptPlugin
  )

endif()

if (PLASMA_CMAKE_PLATFORM_WINDOWS_UWP)
  # Due to app sandboxing we need to explcitly name required plugins for UWP.
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
    InspectorPlugin
  )

  if (PLASMA_BUILD_FMOD)
    target_link_libraries(${PROJECT_NAME} PUBLIC FmodPlugin)
  endif()
endif()


pl_link_target_dx11(${PROJECT_NAME})

pl_ci_add_test(${PROJECT_NAME} NEEDS_HW_ACCESS)

add_dependencies(${PROJECT_NAME}
  ShaderCompilerHLSL
)


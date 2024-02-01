pl_cmake_init()


pl_requires_openxr()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

if (NOT PL_OPENXR_PREVIEW_DIR STREQUAL "")
	target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_OPENXR_PREVIEW_SUPPORT)
endif()

if (PL_CMAKE_PLATFORM_WINDOWS_DESKTOP AND PL_CMAKE_ARCHITECTURE_64BIT)
	target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT)
endif()
    
pl_link_target_openxr(${PROJECT_NAME})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/OpenXR.plPluginBundle" $<TARGET_FILE_DIR:${PROJECT_NAME}>
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	  )

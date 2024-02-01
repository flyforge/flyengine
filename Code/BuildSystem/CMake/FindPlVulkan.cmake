# find the folder into which the Vulkan SDK has been installed

# early out, if this target has been created before
if((TARGET PlVulkan::Loader) AND(TARGET PlVulkan::DXC))
	return()
endif()

set(PL_VULKAN_DIR $ENV{VULKAN_SDK} CACHE PATH "Directory of the Vulkan SDK")

pl_pull_compiler_and_architecture_vars()
pl_pull_config_vars()

get_property(PL_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY PL_SUBMODULE_PREFIX_PATH)

if (COMMAND pl_platformhook_find_vulkan)
	pl_platformhook_find_vulkan()
else()
	message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
endif()

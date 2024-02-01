#pragma once

#ifdef BUILDSYSTEM_ENABLE_GLFW_SUPPORT
#  define PL_SUPPORTS_GLFW PL_ON
#else
#  define PL_SUPPORTS_GLFW PL_OFF
#endif
